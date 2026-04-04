#!/usr/bin/env python3
"""
Regenerate stepwise H2O/H2 equilibrium-ratio data for the TGA screen.

This helper reuses the existing Fe-O-H FPROPS diagnostics to compute:

1. hematite | magnetite:
      3 Fe2O3 + H2 <-> 2 Fe3O4 + H2O
2. magnetite | wustite:
      boundary-derived (p_H2O / p_H2)_eq from the current wustite|spinel model
3. wustite | iron:
      boundary-derived (p_H2O / p_H2)_eq from the current Fe|wustite model

It then compares several compact surrogates, including the thermo-shaped
form now used in the TGA models:

    ln(K_ratio_eq) = A + B / T[K] + C ln(T[K]) + D T[K]

for convenient use inside `models/johnpye/iron/tga.a4c`.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from dataclasses import asdict, dataclass
from pathlib import Path


THIS_DIR = Path(__file__).resolve().parent
FPROPS_TEST_DIR = THIS_DIR.parent / "fprops" / "test"
sys.path.insert(0, str(FPROPS_TEST_DIR))

from feoh_hydrogen_boundary import (  # type: ignore
    default_runner,
    gas_ratio_logs,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
    query_mu0,
)


R = 8.31446261815324
STEP_FIELDS = [
    ("k1_hematite_magnetite", "Step 1 Fe2O3 -> Fe3O4", "#c23b22"),
    ("k2_magnetite_wustite", "Step 2 Fe3O4 -> FeO", "#1f77b4"),
    ("k3_wustite_iron", "Step 3 FeO -> Fe", "#2ca02c"),
]


@dataclass(frozen=True)
class DataPoint:
    temperature_k: float
    k1_hematite_magnetite: float
    k2_magnetite_wustite: float
    k3_wustite_iron: float


@dataclass(frozen=True)
class FitResult:
    name: str
    a: float
    b: float
    max_pct_err: float


@dataclass(frozen=True)
class QuadraticFitResult:
    name: str
    c0: float
    c1: float
    c2: float
    max_pct_err: float


@dataclass(frozen=True)
class CubicFitResult:
    name: str
    c0: float
    c1: float
    c2: float
    c3: float
    max_pct_err: float


@dataclass(frozen=True)
class ThermoFitResult:
    name: str
    a: float
    b_over_t: float
    c_ln_t: float
    d_t: float
    max_pct_err: float


def parse_temps_c(text: str) -> list[float]:
    temps: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            temps.append(float(tok))
    if not temps:
        raise ValueError("no temperatures supplied")
    return temps


def k1_ratio_hematite_magnetite(
    runner: Path,
    source_mix: str,
    temperature_k: float,
) -> float:
    mu = query_mu0(
        runner,
        source_mix,
        temperature_k,
        ["Fe2O3", "Fe3O4", "hydrogen", "water"],
    )
    delta_g = (
        2.0 * mu["Fe3O4"]
        + mu["water"]
        - 3.0 * mu["Fe2O3"]
        - mu["hydrogen"]
    )
    return math.exp(-delta_g / (R * temperature_k))


def k_ratio_from_boundary(
    runner: Path,
    source_gas: str,
    temperature_k: float,
    boundary_fn,
) -> float:
    _x, lam_o = boundary_fn(temperature_k)
    mu = query_mu0(runner, source_gas, temperature_k, ["hydrogen", "water"])
    log10_h2o_h2, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, temperature_k)
    return 10.0 ** log10_h2o_h2


def fit_ln_k(temperatures_k: list[float], values: list[float]) -> FitResult:
    xs = [1.0 / t for t in temperatures_k]
    ys = [math.log(v) for v in values]
    n = len(xs)
    sx = sum(xs)
    sy = sum(ys)
    sxx = sum(x * x for x in xs)
    sxy = sum(x * y for x, y in zip(xs, ys))
    m = (n * sxy - sx * sy) / (n * sxx - sx * sx)
    c = (sy - m * sx) / n
    a = c
    b = -m
    max_pct_err = 0.0
    for temperature_k, value in zip(temperatures_k, values):
        fitted = math.exp(a - b / temperature_k)
        pct_err = abs((fitted / value - 1.0) * 100.0)
        max_pct_err = max(max_pct_err, pct_err)
    return FitResult(name="", a=a, b=b, max_pct_err=max_pct_err)


def solve_linear_system(matrix: list[list[float]], rhs: list[float]) -> list[float]:
    aug = [row[:] + [value] for row, value in zip(matrix, rhs)]
    n = len(matrix)
    for col in range(n):
        pivot_row = max(range(col, n), key=lambda row: abs(aug[row][col]))
        if abs(aug[pivot_row][col]) < 1e-20:
            raise ValueError("singular fit system")
        if pivot_row != col:
            aug[col], aug[pivot_row] = aug[pivot_row], aug[col]
        pivot = aug[col][col]
        for j in range(col, n + 1):
            aug[col][j] /= pivot
        for row in range(n):
            if row == col:
                continue
            factor = aug[row][col]
            for j in range(col, n + 1):
                aug[row][j] -= factor * aug[col][j]
    return [aug[row][n] for row in range(n)]


def fit_linear_basis(
    basis_rows: list[list[float]],
    values: list[float],
) -> list[float]:
    n_features = len(basis_rows[0])
    gram = [[0.0 for _ in range(n_features)] for _ in range(n_features)]
    rhs = [0.0 for _ in range(n_features)]
    for row, value in zip(basis_rows, values):
        for i in range(n_features):
            rhs[i] += row[i] * value
            for j in range(n_features):
                gram[i][j] += row[i] * row[j]
    return solve_linear_system(gram, rhs)


def eval_linear_fit(fit: FitResult, temperature_k: float) -> float:
    return math.exp(fit.a - fit.b / temperature_k)


def eval_quadratic_fit(fit: QuadraticFitResult, temperature_k: float) -> float:
    x = 1.0 / temperature_k
    return math.exp(fit.c0 + fit.c1 * x + fit.c2 * x * x)


def fit_ln_k_quadratic(temperatures_k: list[float], values: list[float]) -> QuadraticFitResult:
    xs = [1.0 / t for t in temperatures_k]
    ys = [math.log(v) for v in values]

    s0 = float(len(xs))
    s1 = sum(xs)
    s2 = sum(x * x for x in xs)
    s3 = sum(x * x * x for x in xs)
    s4 = sum(x * x * x * x for x in xs)
    t0 = sum(ys)
    t1 = sum(x * y for x, y in zip(xs, ys))
    t2 = sum(x * x * y for x, y in zip(xs, ys))

    c0, c1, c2 = solve_linear_system(
        [
            [s0, s1, s2],
            [s1, s2, s3],
            [s2, s3, s4],
        ],
        [t0, t1, t2],
    )

    max_pct_err = 0.0
    for temperature_k, value in zip(temperatures_k, values):
        fitted = math.exp(c0 + c1 / temperature_k + c2 / (temperature_k * temperature_k))
        pct_err = abs((fitted / value - 1.0) * 100.0)
        max_pct_err = max(max_pct_err, pct_err)
    return QuadraticFitResult(name="", c0=c0, c1=c1, c2=c2, max_pct_err=max_pct_err)


def eval_cubic_fit(fit: CubicFitResult, temperature_k: float) -> float:
    x = 1000.0 / temperature_k
    return math.exp(fit.c0 + fit.c1 * x + fit.c2 * x * x + fit.c3 * x * x * x)


def fit_ln_k_cubic(temperatures_k: list[float], values: list[float]) -> CubicFitResult:
    xs = [1000.0 / t for t in temperatures_k]
    ys = [math.log(v) for v in values]
    basis_rows = [[1.0, x, x * x, x * x * x] for x in xs]
    c0, c1, c2, c3 = fit_linear_basis(basis_rows, ys)

    max_pct_err = 0.0
    for temperature_k, value in zip(temperatures_k, values):
        x = 1000.0 / temperature_k
        fitted = math.exp(c0 + c1 * x + c2 * x * x + c3 * x * x * x)
        pct_err = abs((fitted / value - 1.0) * 100.0)
        max_pct_err = max(max_pct_err, pct_err)
    return CubicFitResult(name="", c0=c0, c1=c1, c2=c2, c3=c3, max_pct_err=max_pct_err)


def eval_thermo_fit(fit: ThermoFitResult, temperature_k: float) -> float:
    return math.exp(
        fit.a
        + fit.b_over_t / temperature_k
        + fit.c_ln_t * math.log(temperature_k)
        + fit.d_t * temperature_k
    )


def fit_ln_k_thermo(temperatures_k: list[float], values: list[float]) -> ThermoFitResult:
    ys = [math.log(v) for v in values]
    basis_rows = [[1.0, 1.0 / t, math.log(t), t] for t in temperatures_k]
    a, b_over_t, c_ln_t, d_t = fit_linear_basis(basis_rows, ys)

    max_pct_err = 0.0
    for temperature_k, value in zip(temperatures_k, values):
        fitted = math.exp(a + b_over_t / temperature_k + c_ln_t * math.log(temperature_k) + d_t * temperature_k)
        pct_err = abs((fitted / value - 1.0) * 100.0)
        max_pct_err = max(max_pct_err, pct_err)
    return ThermoFitResult(
        name="",
        a=a,
        b_over_t=b_over_t,
        c_ln_t=c_ln_t,
        d_t=d_t,
        max_pct_err=max_pct_err,
    )


def write_csv(path: Path, rows: list[DataPoint]) -> None:
    with path.open("w", newline="", encoding="ascii") as fp:
        writer = csv.writer(fp)
        writer.writerow(
            [
                "temperature_k",
                "k1_hematite_magnetite",
                "k2_magnetite_wustite",
                "k3_wustite_iron",
            ]
        )
        for row in rows:
            writer.writerow(
                [
                    f"{row.temperature_k:.2f}",
                    f"{row.k1_hematite_magnetite:.12g}",
                    f"{row.k2_magnetite_wustite:.12g}",
                    f"{row.k3_wustite_iron:.12g}",
                ]
            )


def write_json(
    path: Path,
    rows: list[DataPoint],
    fits: list[FitResult],
    quadratic_fits: list[QuadraticFitResult],
    cubic_fits: list[CubicFitResult],
    thermo_fits: list[ThermoFitResult],
    meta: dict[str, str],
) -> None:
    payload = {
        "meta": meta,
        "rows": [asdict(r) for r in rows],
        "linear_fits": [asdict(f) for f in fits],
        "quadratic_fits": [asdict(f) for f in quadratic_fits],
        "cubic_fits": [asdict(f) for f in cubic_fits],
        "thermo_fits": [asdict(f) for f in thermo_fits],
    }
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="ascii")


def plot_fit(
    path: Path,
    rows: list[DataPoint],
    fits: list[FitResult],
    quadratic_fits: list[QuadraticFitResult],
    cubic_fits: list[CubicFitResult],
    thermo_fits: list[ThermoFitResult],
) -> None:
    import matplotlib.pyplot as plt
    from matplotlib.lines import Line2D

    temps_k = [row.temperature_k for row in rows]
    temps_c = [t - 273.15 for t in temps_k]
    fit_by_name = {fit.name: fit for fit in fits}
    quad_fit_by_name = {fit.name: fit for fit in quadratic_fits}
    cubic_fit_by_name = {fit.name: fit for fit in cubic_fits}
    thermo_fit_by_name = {fit.name: fit for fit in thermo_fits}

    fig, (ax_curve, ax_err) = plt.subplots(
        2,
        1,
        figsize=(10.2, 7.8),
        sharex=True,
        gridspec_kw={"height_ratios": [3.2, 1.2]},
        constrained_layout=True,
    )

    dense_temps_k = [
        min(temps_k) + (max(temps_k) - min(temps_k)) * i / 400.0 for i in range(401)
    ]
    dense_temps_c = [t - 273.15 for t in dense_temps_k]

    for field_name, label, color in STEP_FIELDS:
        fit = fit_by_name[field_name]
        quad_fit = quad_fit_by_name[field_name]
        cubic_fit = cubic_fit_by_name[field_name]
        thermo_fit = thermo_fit_by_name[field_name]
        values = [getattr(row, field_name) for row in rows]
        dense_fit = [eval_linear_fit(fit, t) for t in dense_temps_k]
        dense_quad_fit = [eval_quadratic_fit(quad_fit, t) for t in dense_temps_k]
        dense_cubic_fit = [eval_cubic_fit(cubic_fit, t) for t in dense_temps_k]
        dense_thermo_fit = [eval_thermo_fit(thermo_fit, t) for t in dense_temps_k]
        pct_err = [
            (eval_linear_fit(fit, row.temperature_k) / getattr(row, field_name) - 1.0)
            * 100.0
            for row in rows
        ]
        pct_err_quad = [
            (eval_quadratic_fit(quad_fit, row.temperature_k) / getattr(row, field_name) - 1.0)
            * 100.0
            for row in rows
        ]
        pct_err_cubic = [
            (eval_cubic_fit(cubic_fit, row.temperature_k) / getattr(row, field_name) - 1.0)
            * 100.0
            for row in rows
        ]
        pct_err_thermo = [
            (eval_thermo_fit(thermo_fit, row.temperature_k) / getattr(row, field_name) - 1.0)
            * 100.0
            for row in rows
        ]

        ax_curve.plot(dense_temps_c, dense_fit, color=color, lw=2.2)
        ax_curve.plot(dense_temps_c, dense_quad_fit, color=color, lw=1.6, ls="--")
        ax_curve.plot(dense_temps_c, dense_cubic_fit, color=color, lw=1.2, ls=":")
        ax_curve.plot(dense_temps_c, dense_thermo_fit, color=color, lw=1.2, ls="-.")
        ax_curve.scatter(
            temps_c,
            values,
            color=color,
            s=34,
            marker="o",
            edgecolors="white",
            linewidths=0.5,
            zorder=3,
        )
        ax_err.plot(temps_c, pct_err, color=color, lw=1.6, marker="o", ms=4)
        ax_err.plot(temps_c, pct_err_quad, color=color, lw=1.2, ls="--", marker="x", ms=4)
        ax_err.plot(temps_c, pct_err_cubic, color=color, lw=1.1, ls=":", marker="s", ms=3.5)
        ax_err.plot(temps_c, pct_err_thermo, color=color, lw=1.1, ls="-.", marker="^", ms=3.5)

    ax_curve.set_yscale("log")
    ax_curve.set_ylabel("K_ratio_eq = (p_H2O / p_H2)_eq")
    ax_curve.set_title("TGA stepwise K(T): four-way fit comparison against FPROPS points")
    ax_curve.grid(True, which="both", alpha=0.3)

    style_handles = [
        Line2D([0], [0], color="0.2", lw=2.2, ls="-", label="Linear in 1/T"),
        Line2D([0], [0], color="0.2", lw=1.6, ls="--", label="Quadratic in 1/T"),
        Line2D([0], [0], color="0.2", lw=1.2, ls=":", label="Cubic in 1000/T"),
        Line2D([0], [0], color="0.2", lw=1.2, ls="-.", label="A + B/T + C ln(T) + D T"),
        Line2D([0], [0], color="0.2", marker="o", lw=0, markersize=5, label="FPROPS points"),
    ]
    leg1 = ax_curve.legend(
        handles=style_handles,
        loc="upper left",
        fontsize=8,
        frameon=True,
        title="Curve Style",
        title_fontsize=9,
    )
    ax_curve.add_artist(leg1)

    color_lines = [f"{label}" for _field_name, label, _color in STEP_FIELDS]
    color_text = "\n".join(
        [f"{label}" for _field_name, label, _color in STEP_FIELDS]
    )
    ax_curve.text(
        0.985,
        0.98,
        color_text,
        transform=ax_curve.transAxes,
        ha="right",
        va="top",
        fontsize=8,
        bbox={"boxstyle": "round,pad=0.25", "facecolor": "white", "alpha": 0.85, "edgecolor": "0.7"},
        color="black",
    )
    for idx, (_field_name, label, color) in enumerate(STEP_FIELDS):
        ax_curve.text(
            0.975,
            0.95 - 0.06 * idx,
            label,
            transform=ax_curve.transAxes,
            ha="right",
            va="top",
            fontsize=8,
            color=color,
        )

    ax_err.set_xlabel("Temperature [C]")
    ax_err.set_ylabel("Fit error [%]")
    ax_err.grid(True, alpha=0.3)

    for idx, (_, label, color) in enumerate(STEP_FIELDS):
        fit = fit_by_name[STEP_FIELDS[idx][0]]
        quad_fit = quad_fit_by_name[STEP_FIELDS[idx][0]]
        cubic_fit = cubic_fit_by_name[STEP_FIELDS[idx][0]]
        thermo_fit = thermo_fit_by_name[STEP_FIELDS[idx][0]]
        ax_err.text(
            0.02,
            0.92 - 0.11 * idx,
            (
                f"{label}: lin {fit.max_pct_err:.2f}%, "
                f"quad {quad_fit.max_pct_err:.2f}%, "
                f"cubic {cubic_fit.max_pct_err:.2f}%, "
                f"thermo {thermo_fit.max_pct_err:.2f}%"
            ),
            color=color,
            transform=ax_err.transAxes,
            fontsize=9,
            ha="left",
            va="top",
        )

    fig.savefig(path, dpi=180)
    plt.close(fig)


def plot_residuals(
    path: Path,
    rows: list[DataPoint],
    fits: list[FitResult],
    quadratic_fits: list[QuadraticFitResult],
    cubic_fits: list[CubicFitResult],
    thermo_fits: list[ThermoFitResult],
) -> None:
    import matplotlib.pyplot as plt

    temps_c = [row.temperature_k - 273.15 for row in rows]
    fit_by_name = {fit.name: fit for fit in fits}
    quad_fit_by_name = {fit.name: fit for fit in quadratic_fits}
    cubic_fit_by_name = {fit.name: fit for fit in cubic_fits}
    thermo_fit_by_name = {fit.name: fit for fit in thermo_fits}

    fig, axes = plt.subplots(3, 1, figsize=(8.6, 8.4), sharex=True, constrained_layout=True)

    for ax, (field_name, label, color) in zip(axes, STEP_FIELDS):
        fit = fit_by_name[field_name]
        quad_fit = quad_fit_by_name[field_name]
        cubic_fit = cubic_fit_by_name[field_name]
        thermo_fit = thermo_fit_by_name[field_name]
        values = [getattr(row, field_name) for row in rows]
        pct_err_linear = [
            (eval_linear_fit(fit, row.temperature_k) / value - 1.0) * 100.0
            for row, value in zip(rows, values)
        ]
        pct_err_quad = [
            (eval_quadratic_fit(quad_fit, row.temperature_k) / value - 1.0) * 100.0
            for row, value in zip(rows, values)
        ]
        pct_err_cubic = [
            (eval_cubic_fit(cubic_fit, row.temperature_k) / value - 1.0) * 100.0
            for row, value in zip(rows, values)
        ]
        pct_err_thermo = [
            (eval_thermo_fit(thermo_fit, row.temperature_k) / value - 1.0) * 100.0
            for row, value in zip(rows, values)
        ]

        ax.axhline(0.0, color="0.35", lw=0.9)
        ax.plot(temps_c, pct_err_linear, color=color, lw=1.6, marker="o", ms=4, label="Linear in 1/T")
        ax.plot(temps_c, pct_err_quad, color=color, lw=1.3, ls="--", marker="x", ms=4, label="Quadratic in 1/T")
        ax.plot(temps_c, pct_err_cubic, color=color, lw=1.1, ls=":", marker="s", ms=3.5, label="Cubic in 1/T")
        ax.plot(temps_c, pct_err_thermo, color=color, lw=1.1, ls="-.", marker="^", ms=3.5, label="A + B/T + C lnT + D T")
        ax.set_ylabel("Error [%]")
        ax.set_title(
            (
                f"{label}: "
                f"lin {fit.max_pct_err:.2f}%, "
                f"quad {quad_fit.max_pct_err:.2f}%, "
                f"cubic {cubic_fit.max_pct_err:.2f}%, "
                f"thermo {thermo_fit.max_pct_err:.2f}%"
            )
        )
        ax.grid(True, alpha=0.3)
        ax.legend(loc="best", fontsize=8)

    axes[-1].set_xlabel("Temperature [C]")
    fig.suptitle("TGA K(T) residuals: four-way fit comparison", fontsize=14)
    fig.savefig(path, dpi=180)
    plt.close(fig)


def main() -> int:
    ap = argparse.ArgumentParser(description="Regenerate K_ratio_eq(T) data for the iron TGA screen.")
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000,1100,1200",
        help="Comma-separated temperatures in Celsius.",
    )
    ap.add_argument(
        "--gas-source",
        default="helmholtz+ref0:",
        help="Gas mu0 source for hydrogen and water.",
    )
    ap.add_argument(
        "--hematite-source",
        default="Fe2O3=hidayat_2015;Fe3O4=hidayat_2015;*=helmholtz+ref0:",
        help="Mixed mu0 source string for the hematite/magnetite step.",
    )
    ap.add_argument(
        "--runner",
        type=Path,
        default=default_runner(),
        help="Path to eqm_mu0_runner.",
    )
    ap.add_argument(
        "--csv-out",
        type=Path,
        help="Optional CSV output path for the raw K_ratio_eq(T) data.",
    )
    ap.add_argument(
        "--json-out",
        type=Path,
        help="Optional JSON output path for the raw data and fit summary.",
    )
    ap.add_argument(
        "--plot-out",
        type=Path,
        help="Optional plot output path for raw FPROPS points, fitted curves, and pointwise fit error.",
    )
    ap.add_argument(
        "--residual-plot-out",
        type=Path,
        help="Optional residual-plot output path comparing linear and quadratic fit errors.",
    )
    args = ap.parse_args()

    temperatures_k = [tc + 273.15 for tc in parse_temps_c(args.temps_c)]
    rows: list[DataPoint] = []
    for temperature_k in temperatures_k:
        print(f"Temperature {temperature_k}...");
        rows.append(
            DataPoint(
                temperature_k=temperature_k,
                k1_hematite_magnetite=k1_ratio_hematite_magnetite(
                    args.runner, args.hematite_source, temperature_k
                ),
                k2_magnetite_wustite=k_ratio_from_boundary(
                    args.runner,
                    args.gas_source,
                    temperature_k,
                    oxygen_potential_at_wustite_spinel_boundary,
                ),
                k3_wustite_iron=k_ratio_from_boundary(
                    args.runner,
                    args.gas_source,
                    temperature_k,
                    oxygen_potential_at_fe_wustite_boundary,
                ),
            )
        )

    fit1 = fit_ln_k(temperatures_k, [r.k1_hematite_magnetite for r in rows])
    fit2 = fit_ln_k(temperatures_k, [r.k2_magnetite_wustite for r in rows])
    fit3 = fit_ln_k(temperatures_k, [r.k3_wustite_iron for r in rows])
    fit1 = FitResult("k1_hematite_magnetite", fit1.a, fit1.b, fit1.max_pct_err)
    fit2 = FitResult("k2_magnetite_wustite", fit2.a, fit2.b, fit2.max_pct_err)
    fit3 = FitResult("k3_wustite_iron", fit3.a, fit3.b, fit3.max_pct_err)
    fits = [fit1, fit2, fit3]

    qfit1 = fit_ln_k_quadratic(temperatures_k, [r.k1_hematite_magnetite for r in rows])
    qfit2 = fit_ln_k_quadratic(temperatures_k, [r.k2_magnetite_wustite for r in rows])
    qfit3 = fit_ln_k_quadratic(temperatures_k, [r.k3_wustite_iron for r in rows])
    qfit1 = QuadraticFitResult("k1_hematite_magnetite", qfit1.c0, qfit1.c1, qfit1.c2, qfit1.max_pct_err)
    qfit2 = QuadraticFitResult("k2_magnetite_wustite", qfit2.c0, qfit2.c1, qfit2.c2, qfit2.max_pct_err)
    qfit3 = QuadraticFitResult("k3_wustite_iron", qfit3.c0, qfit3.c1, qfit3.c2, qfit3.max_pct_err)
    quadratic_fits = [qfit1, qfit2, qfit3]

    cfit1 = fit_ln_k_cubic(temperatures_k, [r.k1_hematite_magnetite for r in rows])
    cfit2 = fit_ln_k_cubic(temperatures_k, [r.k2_magnetite_wustite for r in rows])
    cfit3 = fit_ln_k_cubic(temperatures_k, [r.k3_wustite_iron for r in rows])
    cfit1 = CubicFitResult("k1_hematite_magnetite", cfit1.c0, cfit1.c1, cfit1.c2, cfit1.c3, cfit1.max_pct_err)
    cfit2 = CubicFitResult("k2_magnetite_wustite", cfit2.c0, cfit2.c1, cfit2.c2, cfit2.c3, cfit2.max_pct_err)
    cfit3 = CubicFitResult("k3_wustite_iron", cfit3.c0, cfit3.c1, cfit3.c2, cfit3.c3, cfit3.max_pct_err)
    cubic_fits = [cfit1, cfit2, cfit3]

    tfit1 = fit_ln_k_thermo(temperatures_k, [r.k1_hematite_magnetite for r in rows])
    tfit2 = fit_ln_k_thermo(temperatures_k, [r.k2_magnetite_wustite for r in rows])
    tfit3 = fit_ln_k_thermo(temperatures_k, [r.k3_wustite_iron for r in rows])
    tfit1 = ThermoFitResult("k1_hematite_magnetite", tfit1.a, tfit1.b_over_t, tfit1.c_ln_t, tfit1.d_t, tfit1.max_pct_err)
    tfit2 = ThermoFitResult("k2_magnetite_wustite", tfit2.a, tfit2.b_over_t, tfit2.c_ln_t, tfit2.d_t, tfit2.max_pct_err)
    tfit3 = ThermoFitResult("k3_wustite_iron", tfit3.a, tfit3.b_over_t, tfit3.c_ln_t, tfit3.d_t, tfit3.max_pct_err)
    thermo_fits = [tfit1, tfit2, tfit3]

    print("K_ratio_eq(T) data for models/johnpye/iron/tga.a4c")
    print(f"runner = {args.runner}")
    print(f"gas source = {args.gas_source}")
    print(f"hematite source = {args.hematite_source}")
    print()
    print("T_K,K1_hematite_magnetite,K2_magnetite_wustite,K3_wustite_iron")
    for row in rows:
        print(
            f"{row.temperature_k:.2f},"
            f"{row.k1_hematite_magnetite:.12g},"
            f"{row.k2_magnetite_wustite:.12g},"
            f"{row.k3_wustite_iron:.12g}"
        )

    print()
    print("ASCEND coefficient block for thermo-shaped fit")
    print("(* regenerated by models/johnpye/iron/regenerate_k_ratio_eq.py *)")
    for idx, fit in enumerate(thermo_fits, start=1):
        print(f"K_eq_fit_A[{idx}] := {fit.a:.16g};")
    for idx, fit in enumerate(thermo_fits, start=1):
        print(f"K_eq_fit_B[{idx}] := {fit.b_over_t:.16g};")
    for idx, fit in enumerate(thermo_fits, start=1):
        print(f"K_eq_fit_C[{idx}] := {fit.c_ln_t:.16g};")
    for idx, fit in enumerate(thermo_fits, start=1):
        print(f"K_eq_fit_D[{idx}] := {fit.d_t:.16g};")

    print()
    print("Fit summary")
    for fit in fits:
        print(
            f"{fit.name}: "
            f"A={fit.a:.16g}, "
            f"B={fit.b:.16g}, "
            f"max_pct_err={fit.max_pct_err:.6f}"
        )

    print()
    print("Quadratic fit summary in x = 1/T[K]")
    for fit in quadratic_fits:
        print(
            f"{fit.name}: "
            f"ln(K)=c0 + c1*x + c2*x^2, "
            f"c0={fit.c0:.16g}, "
            f"c1={fit.c1:.16g}, "
            f"c2={fit.c2:.16g}, "
            f"max_pct_err={fit.max_pct_err:.6f}"
        )

    print()
    print("Cubic fit summary in x = 1/T[K]")
    for fit in cubic_fits:
        print(
            f"{fit.name}: "
            f"ln(K)=c0 + c1*x + c2*x^2 + c3*x^3, "
            f"c0={fit.c0:.16g}, "
            f"c1={fit.c1:.16g}, "
            f"c2={fit.c2:.16g}, "
            f"c3={fit.c3:.16g}, "
            f"max_pct_err={fit.max_pct_err:.6f}"
        )

    print()
    print("Thermo-shaped fit summary")
    for fit in thermo_fits:
        print(
            f"{fit.name}: "
            f"ln(K)=A + B/T + C ln(T) + D T, "
            f"A={fit.a:.16g}, "
            f"B={fit.b_over_t:.16g}, "
            f"C={fit.c_ln_t:.16g}, "
            f"D={fit.d_t:.16g}, "
            f"max_pct_err={fit.max_pct_err:.6f}"
        )

    if args.csv_out:
        write_csv(args.csv_out, rows)
        print()
        print(f"wrote CSV: {args.csv_out}")

    if args.json_out:
        meta = {
            "runner": str(args.runner),
            "gas_source": args.gas_source,
            "hematite_source": args.hematite_source,
        }
        write_json(args.json_out, rows, fits, quadratic_fits, cubic_fits, thermo_fits, meta)
        print(f"wrote JSON: {args.json_out}")

    if args.plot_out:
        plot_fit(args.plot_out, rows, fits, quadratic_fits, cubic_fits, thermo_fits)
        print(f"wrote plot: {args.plot_out}")

    if args.residual_plot_out:
        plot_residuals(args.residual_plot_out, rows, fits, quadratic_fits, cubic_fits, thermo_fits)
        print(f"wrote residual plot: {args.residual_plot_out}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
