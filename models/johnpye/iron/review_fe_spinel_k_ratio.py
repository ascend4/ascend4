#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


THIS_DIR = Path(__file__).resolve().parent
FPROPS_TEST_DIR = THIS_DIR.parent / "fprops" / "test"
sys.path.insert(0, str(FPROPS_TEST_DIR))
sys.path.insert(0, str(THIS_DIR))

from feoh_baur_glaessner_compare import FIT_SPECS  # type: ignore
from feoh_hydrogen_boundary import (  # type: ignore
    default_runner,
    gas_ratio_logs,
    oxygen_potential_at_fe_spinel_boundary,
    query_mu0,
)
from regenerate_k_ratio_eq import (  # type: ignore
    CubicFitResult,
    FitResult,
    QuadraticFitResult,
    ThermoFitResult,
    eval_cubic_fit,
    eval_linear_fit,
    eval_quadratic_fit,
    eval_thermo_fit,
    fit_ln_k,
    fit_ln_k_cubic,
    fit_ln_k_quadratic,
    fit_ln_k_thermo,
)


GAS_SOURCE = "helmholtz+ref0:"
DEFAULT_TEMPS_C = "300,350,400,450,500,550,600,650,700"
BG_SPEC_KEY = "h2-fe-spinel"


@dataclass(frozen=True)
class DataPoint:
    temperature_c: float
    temperature_k: float
    k_fe_spinel: float


def parse_temps_c(text: str) -> list[float]:
    temps: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            temps.append(float(tok))
    if not temps:
        raise ValueError("no temperatures supplied")
    return temps


def ratio_from_log10(log10_h2o_h2: float) -> float:
    return 10.0 ** log10_h2o_h2


def k_ratio_fe_spinel(runner: Path, temperature_k: float) -> float:
    _phase, lam_o = oxygen_potential_at_fe_spinel_boundary(temperature_k)
    mu = query_mu0(runner, GAS_SOURCE, temperature_k, ["hydrogen", "water"])
    log10_h2o_h2, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, temperature_k)
    return ratio_from_log10(log10_h2o_h2)


def bg_points_as_ratio() -> tuple[list[float], list[float]]:
    spec = FIT_SPECS[BG_SPEC_KEY]
    temps_c = [temp_c for _god, temp_c in spec.points]
    ratios = [god / (1.0 - god) for god, _temp_c in spec.points]
    return temps_c, ratios


def generate_rows(runner: Path, temps_c: list[float]) -> list[DataPoint]:
    rows: list[DataPoint] = []
    for temp_c in temps_c:
        temp_k = temp_c + 273.15
        rows.append(
            DataPoint(
                temperature_c=temp_c,
                temperature_k=temp_k,
                k_fe_spinel=k_ratio_fe_spinel(runner, temp_k),
            )
        )
    return rows


def fit_all(rows: list[DataPoint]) -> tuple[FitResult, QuadraticFitResult, CubicFitResult, ThermoFitResult]:
    temps_k = [row.temperature_k for row in rows]
    values = [row.k_fe_spinel for row in rows]

    linear = fit_ln_k(temps_k, values)
    quadratic = fit_ln_k_quadratic(temps_k, values)
    cubic = fit_ln_k_cubic(temps_k, values)
    thermo = fit_ln_k_thermo(temps_k, values)

    return (
        FitResult("fe_spinel", linear.a, linear.b, linear.max_pct_err),
        QuadraticFitResult("fe_spinel", quadratic.c0, quadratic.c1, quadratic.c2, quadratic.max_pct_err),
        CubicFitResult("fe_spinel", cubic.c0, cubic.c1, cubic.c2, cubic.c3, cubic.max_pct_err),
        ThermoFitResult("fe_spinel", thermo.a, thermo.b_over_t, thermo.c_ln_t, thermo.d_t, thermo.max_pct_err),
    )


def plot_review(
    outfile: Path,
    rows: list[DataPoint],
    linear: FitResult,
    quadratic: QuadraticFitResult,
    cubic: CubicFitResult,
    thermo: ThermoFitResult,
) -> None:
    temps_c = [row.temperature_c for row in rows]
    temps_k = [row.temperature_k for row in rows]
    values = [row.k_fe_spinel for row in rows]
    dense_c = [min(temps_c) + (max(temps_c) - min(temps_c)) * i / 500.0 for i in range(501)]
    dense_k = [temp_c + 273.15 for temp_c in dense_c]

    bg_temps_c, bg_ratios = bg_points_as_ratio()

    fig, (ax_curve, ax_err) = plt.subplots(
        2,
        1,
        figsize=(10.8, 8.6),
        sharex=True,
        gridspec_kw={"height_ratios": [2.5, 1.2]},
        constrained_layout=True,
    )

    ax_curve.scatter(
        temps_c,
        values,
        color="#6a3d9a",
        marker="o",
        s=34,
        edgecolors="white",
        linewidths=0.5,
        zorder=3,
        label="FPROPS Fe|spinel points",
    )
    ax_curve.scatter(
        bg_temps_c,
        bg_ratios,
        color="black",
        marker="x",
        s=22,
        linewidths=0.9,
        alpha=0.8,
        label="BG points converted from GOD",
    )
    ax_curve.plot(dense_c, [eval_linear_fit(linear, t) for t in dense_k], color="#6a3d9a", lw=1.7, label="Linear in 1/T")
    ax_curve.plot(dense_c, [eval_quadratic_fit(quadratic, t) for t in dense_k], color="#6a3d9a", lw=1.2, ls="--", label="Quadratic in 1/T")
    ax_curve.plot(dense_c, [eval_cubic_fit(cubic, t) for t in dense_k], color="#6a3d9a", lw=1.2, ls=":", label="Cubic in 1/T")
    ax_curve.plot(dense_c, [eval_thermo_fit(thermo, t) for t in dense_k], color="#6a3d9a", lw=1.2, ls="-.", label="A + B/T + C ln(T) + D T")
    ax_curve.set_yscale("log")
    ax_curve.set_ylabel("K = p(H2O) / p(H2)")
    ax_curve.set_title("Direct Fe|spinel boundary: FPROPS, BG, and compact fits")
    ax_curve.grid(True, which="both", alpha=0.3)
    ax_curve.legend(loc="upper left", fontsize=8, ncol=2)

    err_linear = [(eval_linear_fit(linear, t) / v - 1.0) * 100.0 for t, v in zip(temps_k, values)]
    err_quadratic = [(eval_quadratic_fit(quadratic, t) / v - 1.0) * 100.0 for t, v in zip(temps_k, values)]
    err_cubic = [(eval_cubic_fit(cubic, t) / v - 1.0) * 100.0 for t, v in zip(temps_k, values)]
    err_thermo = [(eval_thermo_fit(thermo, t) / v - 1.0) * 100.0 for t, v in zip(temps_k, values)]

    ax_err.axhline(0.0, color="0.35", lw=0.9)
    ax_err.plot(temps_c, err_linear, color="#6a3d9a", lw=1.6, marker="o", ms=4, label="Linear in 1/T")
    ax_err.plot(temps_c, err_quadratic, color="#6a3d9a", lw=1.1, ls="--", marker="x", ms=4, label="Quadratic in 1/T")
    ax_err.plot(temps_c, err_cubic, color="#6a3d9a", lw=1.1, ls=":", marker="s", ms=3.5, label="Cubic in 1/T")
    ax_err.plot(temps_c, err_thermo, color="#6a3d9a", lw=1.1, ls="-.", marker="^", ms=3.5, label="A + B/T + C ln(T) + D T")
    ax_err.set_ylabel("Fit error [%]")
    ax_err.set_xlabel("Temperature [C]")
    ax_err.grid(True, alpha=0.3)
    ax_err.text(
        0.01,
        0.97,
        (
            f"max |err| linear {linear.max_pct_err:.2f}%, "
            f"quad {quadratic.max_pct_err:.2f}%, "
            f"cubic {cubic.max_pct_err:.2f}%, "
            f"thermo {thermo.max_pct_err:.2f}%"
        ),
        transform=ax_err.transAxes,
        ha="left",
        va="top",
        fontsize=10,
    )

    fig.savefig(outfile, dpi=180, bbox_inches="tight")
    plt.close(fig)


def write_json(
    outfile: Path,
    rows: list[DataPoint],
    linear: FitResult,
    quadratic: QuadraticFitResult,
    cubic: CubicFitResult,
    thermo: ThermoFitResult,
) -> None:
    payload = {
        "rows": [asdict(row) for row in rows],
        "linear_fit": asdict(linear),
        "quadratic_fit": asdict(quadratic),
        "cubic_fit": asdict(cubic),
        "thermo_fit": asdict(thermo),
        "bg_spec_key": BG_SPEC_KEY,
    }
    outfile.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Review direct Fe|spinel K-ratio boundary and compact fits.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--temps-c", default=DEFAULT_TEMPS_C, help="Comma-separated temperature grid in C.")
    ap.add_argument("--plot-out", type=Path, help="Output PNG path.")
    ap.add_argument("--json-out", type=Path, help="Output JSON path.")
    args = ap.parse_args()

    rows = generate_rows(args.runner, parse_temps_c(args.temps_c))
    linear, quadratic, cubic, thermo = fit_all(rows)

    print("Direct Fe|spinel K(T) review")
    for row in rows:
        print(f"{row.temperature_c:7.2f} C  {row.k_fe_spinel:.12g}")
    print()
    print(
        f"linear:   A={linear.a:.16g}, B={linear.b:.16g}, max_pct_err={linear.max_pct_err:.6f}"
    )
    print(
        f"quadratic c0={quadratic.c0:.16g}, c1={quadratic.c1:.16g}, "
        f"c2={quadratic.c2:.16g}, max_pct_err={quadratic.max_pct_err:.6f}"
    )
    print(
        f"cubic     c0={cubic.c0:.16g}, c1={cubic.c1:.16g}, c2={cubic.c2:.16g}, "
        f"c3={cubic.c3:.16g}, max_pct_err={cubic.max_pct_err:.6f}"
    )
    print(
        f"thermo    A={thermo.a:.16g}, B={thermo.b_over_t:.16g}, C={thermo.c_ln_t:.16g}, "
        f"D={thermo.d_t:.16g}, max_pct_err={thermo.max_pct_err:.6f}"
    )

    if args.plot_out:
        args.plot_out.parent.mkdir(parents=True, exist_ok=True)
        plot_review(args.plot_out, rows, linear, quadratic, cubic, thermo)
        print(f"wrote plot: {args.plot_out}")

    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        write_json(args.json_out, rows, linear, quadratic, cubic, thermo)
        print(f"wrote json: {args.json_out}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
