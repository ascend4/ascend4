#!/usr/bin/env python3
"""
Compare current FPROPS Tier 3 H2/H2O boundaries against fitted
Baur-Glaessner-style polynomial curves on a common GOD basis.

The published/fitted curves are represented as:

    T_C = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4

where x is taken to be the gas oxidation degree

    GOD = p(H2O) / (p(H2) + p(H2O))

This script inverts the fitted curve numerically over its stated x-range to
obtain x_fit(T), then compares it against the current FPROPS model x_model(T).
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from feoh_hydrogen_boundary import (
    default_runner,
    gas_ratio_logs,
    normalize_gas_source,
    oxygen_potential_at_fe_magnetite_boundary,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
    parse_temps_c,
    query_mu0,
)


@dataclass(frozen=True)
class FitSpec:
    name: str
    boundary: str | None
    fit_kind: str
    coeffs: tuple[float, float, float, float, float]
    xmin: float
    xmax: float
    ymin_c: float
    ymax_c: float
    note: str


FIT_SPECS: dict[str, FitSpec] = {
    "h2-fe-wustite": FitSpec(
        name="H2 Fe|wustite",
        boundary="fe-wustite",
        fit_kind="poly_t_of_god",
        coeffs=(4595.72, -27292.9, 78583.6, -111187.0, 61840.6),
        xmin=0.294362959456,
        xmax=0.494298957173,
        ymin_c=568.64160727,
        ymax_c=1000.16056878,
        note="Fityk polynomial from g3data2 points supplied by user.",
    ),
    "h2-wustite-spinel": FitSpec(
        name="H2 wustite|spinel",
        boundary="wustite-spinel",
        fit_kind="poly_t_of_god",
        coeffs=(2930.67, -16978.7, 43596.4, -48465.1, 20660.2),
        xmin=0.494629473168,
        xmax=0.830900778467,
        ymin_c=569.493308392,
        ymax_c=969.237753586,
        note="Fityk polynomial from g3data2 points supplied by user.",
    ),
    "h2-fe-magnetite": FitSpec(
        name="H2 Fe|magnetite",
        boundary="fe-magnetite",
        fit_kind="linear_god_of_t",
        coeffs=(0.493969, 1.40312e-06, 0.0, 0.0, 0.0),
        xmin=0.494146060491,
        xmax=0.495124245888,
        ymin_c=300.364196965,
        ymax_c=569.974675344,
        note="Near-vertical BG line fitted as GOD(T) from g3data2 points supplied by user.",
    ),
}

PLOT_STYLE: dict[str, dict[str, str]] = {
    "fe-wustite": {
        "color": "#1f4e79",
        "label": "Fe|wustite",
    },
    "wustite-spinel": {
        "color": "#b55200",
        "label": "wustite|spinel",
    },
    "fe-magnetite": {
        "color": "#2b7a0b",
        "label": "Fe|magnetite",
    },
}


def poly_eval(coeffs: tuple[float, ...], x: float) -> float:
    y = 0.0
    for c in reversed(coeffs):
        y = y * x + c
    return y


def bisect_root(coeffs: tuple[float, ...], target_c: float, xa: float, xb: float) -> float:
    fa = poly_eval(coeffs, xa) - target_c
    fb = poly_eval(coeffs, xb) - target_c
    if fa == 0.0:
        return xa
    if fb == 0.0:
        return xb
    if fa * fb > 0.0:
        raise ValueError("interval does not bracket a root")
    a, b = xa, xb
    for _ in range(80):
        m = 0.5 * (a + b)
        fm = poly_eval(coeffs, m) - target_c
        if abs(fm) < 1e-12 or abs(b - a) < 1e-12:
            return m
        if fa * fm <= 0.0:
            b = m
            fb = fm
        else:
            a = m
            fa = fm
    return 0.5 * (a + b)


def fit_god_at_temp(spec: FitSpec, target_c: float) -> float | None:
    if target_c < spec.ymin_c or target_c > spec.ymax_c:
        return None
    if spec.fit_kind == "linear_god_of_t":
        a0, a1, _a2, _a3, _a4 = spec.coeffs
        return a0 + a1 * target_c
    if spec.fit_kind != "poly_t_of_god":
        raise ValueError(f"unsupported fit kind {spec.fit_kind}")
    nscan = 2000
    xs = [spec.xmin + (spec.xmax - spec.xmin) * i / nscan for i in range(nscan + 1)]
    vals = [poly_eval(spec.coeffs, x) - target_c for x in xs]
    for i in range(nscan):
        va = vals[i]
        vb = vals[i + 1]
        if va == 0.0:
            return xs[i]
        if va * vb <= 0.0:
            return bisect_root(spec.coeffs, target_c, xs[i], xs[i + 1])
    return None


def god_from_log10_ratio(log10_h2o_h2: float) -> float:
    ratio = 10.0 ** log10_h2o_h2
    return ratio / (1.0 + ratio)


def log10_ratio_from_god(god: float) -> float:
    return math.log10(god / (1.0 - god))


def boundary_fn(name: str):
    if name == "fe-wustite":
        return oxygen_potential_at_fe_wustite_boundary
    if name == "wustite-spinel":
        return oxygen_potential_at_wustite_spinel_boundary
    if name == "fe-magnetite":
        return oxygen_potential_at_fe_magnetite_boundary
    raise KeyError(name)


def model_god_at_temp(
    fn,
    runner: Path,
    gas_source: str,
    tc: float,
) -> tuple[float, float]:
    tk = tc + 273.15
    first, lam_o = fn(tk)
    _unused = first
    mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
    log10_model, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, tk)
    return god_from_log10_ratio(log10_model), log10_model


@lru_cache(maxsize=None)
def cached_model_god_at_temp(boundary_name: str, runner_str: str, gas_source: str, tc: float) -> tuple[float, float]:
    fn = boundary_fn(boundary_name)
    return model_god_at_temp(fn, Path(runner_str), gas_source, tc)


def frange(start: float, stop: float, step: float) -> list[float]:
    if step <= 0.0:
        raise ValueError("step must be positive")
    vals: list[float] = []
    x = start
    while x <= stop + 1e-12:
        vals.append(x)
        x += step
    if not vals or vals[-1] < stop - 1e-9:
        vals.append(stop)
    return vals


def default_plot_file(spec: FitSpec, gas_source: str) -> Path:
    stem = spec.name.lower().replace("|", "-").replace(" ", "_")
    gas = (
        gas_source.lower()
        .replace(" ", "_")
        .replace("+", "_plus_")
        .replace(":", "")
    )
    return Path(__file__).resolve().parent / f"bg_compare_{stem}_{gas}.png"


def default_all_plot_file(gas_source: str) -> Path:
    gas = (
        gas_source.lower()
        .replace(" ", "_")
        .replace("+", "_plus_")
        .replace(":", "")
    )
    return Path(__file__).resolve().parent / f"bg_compare_all_h2_{gas}.png"


def write_plot(
    spec: FitSpec,
    runner: Path,
    gas_source: str,
    sample_temps_c: list[float],
    plot_file: Path,
    plot_model_step_c: float,
) -> None:
    style_key = spec.boundary if spec.boundary is not None else "fe-magnetite"
    style = PLOT_STYLE[style_key]

    if spec.fit_kind == "poly_t_of_god":
        fit_xs = frange(spec.xmin, spec.xmax, max((spec.xmax - spec.xmin) / 400.0, 1e-5))
        fit_ys = [poly_eval(spec.coeffs, x) for x in fit_xs]
    elif spec.fit_kind == "linear_god_of_t":
        fit_ys = frange(spec.ymin_c, spec.ymax_c, max((spec.ymax_c - spec.ymin_c) / 400.0, 0.5))
        a0, a1, _a2, _a3, _a4 = spec.coeffs
        fit_xs = [a0 + a1 * tc for tc in fit_ys]
    else:
        raise ValueError(f"unsupported fit kind {spec.fit_kind}")

    model_temps = []
    model_gods = []
    if spec.boundary is not None:
        fn = boundary_fn(spec.boundary)
        plot_tmin = max(spec.ymin_c, min(sample_temps_c))
        plot_tmax = min(spec.ymax_c, max(sample_temps_c))
        if plot_tmax <= plot_tmin:
            plot_tmin = spec.ymin_c
            plot_tmax = spec.ymax_c
        model_temps = frange(plot_tmin, plot_tmax, plot_model_step_c)
        model_gods = [model_god_at_temp(fn, runner, gas_source, tc)[0] for tc in model_temps]

    sample_fit_x = []
    sample_fit_y = []
    sample_model_x = []
    sample_model_y = []
    for tc in sample_temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        if fit_god is not None:
            sample_fit_x.append(fit_god)
            sample_fit_y.append(tc)
        if spec.boundary is not None:
            model_god, _ = model_god_at_temp(fn, runner, gas_source, tc)
            sample_model_x.append(model_god)
            sample_model_y.append(tc)

    fig, ax = plt.subplots(figsize=(7.2, 5.4), dpi=160)
    ax.plot(fit_xs, fit_ys, color=style["color"], lw=2.2, ls="-", label="Spreitzer/BG fit")
    if model_gods:
        ax.plot(model_gods, model_temps, color=style["color"], lw=2.2, ls=":", label="FPROPS")
    ax.scatter(sample_fit_x, sample_fit_y, color=style["color"], s=18, marker="o", zorder=3, label="Fit sample points")
    if sample_model_x:
        ax.scatter(sample_model_x, sample_model_y, color=style["color"], s=24, marker="x", zorder=3, label="FPROPS sample points")

    ax.set_title(f"{spec.name}: fit vs FPROPS")
    ax.set_xlabel("GOD = p(H2O) / (p(H2) + p(H2O))")
    ax.set_ylabel("Temperature (C)")
    ax.grid(True, color="#d9d9d9", lw=0.7)
    ax.set_xlim(0.0, max(spec.xmax * 1.05, max(sample_model_x, default=0.0) * 1.05))
    ymin = min(spec.ymin_c, min(sample_temps_c)) - 20.0
    ymax = max(spec.ymax_c, max(sample_temps_c)) + 20.0
    ax.set_ylim(ymin, ymax)
    ax.legend(loc="best", frameon=True)
    fig.tight_layout()
    plot_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(plot_file)
    plt.close(fig)


def write_all_plot(
    specs: list[FitSpec],
    runner: Path,
    gas_source: str,
    sample_temps_c: list[float],
    plot_file: Path,
    plot_model_step_c: float,
) -> None:
    fig, ax = plt.subplots(figsize=(7.6, 5.6), dpi=160)

    xmax = 0.0
    ymin = min(sample_temps_c)
    ymax = max(sample_temps_c)

    for spec in specs:
        style_key = spec.boundary if spec.boundary is not None else "fe-magnetite"
        style = PLOT_STYLE[style_key]

        if spec.fit_kind == "poly_t_of_god":
            fit_xs = frange(spec.xmin, spec.xmax, max((spec.xmax - spec.xmin) / 400.0, 1e-5))
            fit_ys = [poly_eval(spec.coeffs, x) for x in fit_xs]
        elif spec.fit_kind == "linear_god_of_t":
            fit_ys = frange(spec.ymin_c, spec.ymax_c, max((spec.ymax_c - spec.ymin_c) / 400.0, 0.5))
            a0, a1, _a2, _a3, _a4 = spec.coeffs
            fit_xs = [a0 + a1 * tc for tc in fit_ys]
        else:
            raise ValueError(f"unsupported fit kind {spec.fit_kind}")

        model_temps = []
        model_gods = []
        if spec.boundary is not None:
            model_temps = [tc for tc in sample_temps_c if spec.ymin_c <= tc <= spec.ymax_c]
            if not model_temps:
                model_temps = [
                    tc
                    for tc in frange(spec.ymin_c, spec.ymax_c, plot_model_step_c)
                    if spec.ymin_c <= tc <= spec.ymax_c
                ]
            model_gods = [
                cached_model_god_at_temp(spec.boundary, str(runner), gas_source, tc)[0]
                for tc in model_temps
            ]

        sample_fit_x = []
        sample_fit_y = []
        sample_model_x = []
        sample_model_y = []
        for tc in sample_temps_c:
            fit_god = fit_god_at_temp(spec, tc)
            if fit_god is not None:
                sample_fit_x.append(fit_god)
                sample_fit_y.append(tc)
            if spec.boundary is not None and spec.ymin_c <= tc <= spec.ymax_c:
                model_god, _ = cached_model_god_at_temp(spec.boundary, str(runner), gas_source, tc)
                sample_model_x.append(model_god)
                sample_model_y.append(tc)

        ax.plot(
            fit_xs,
            fit_ys,
            color=style["color"],
            lw=2.2,
            ls="-",
            label=f"Spreitzer {style['label']}",
        )
        if model_gods:
            ax.plot(
                model_gods,
                model_temps,
                color=style["color"],
                lw=2.2,
                ls=":",
                label=f"FPROPS {style['label']}",
            )
        ax.scatter(sample_fit_x, sample_fit_y, color=style["color"], s=16, marker="o", zorder=3)
        if sample_model_x:
            ax.scatter(sample_model_x, sample_model_y, color=style["color"], s=22, marker="x", zorder=3)

        xmax = max(xmax, spec.xmax, max(model_gods, default=0.0))
        ymin = min(ymin, spec.ymin_c)
        ymax = max(ymax, spec.ymax_c)

    ax.set_title("H2 Baur-Glaessner fits vs FPROPS")
    ax.set_xlabel("GOD = p(H2O) / (p(H2) + p(H2O))")
    ax.set_ylabel("Temperature (C)")
    ax.grid(True, color="#d9d9d9", lw=0.7)
    ax.set_xlim(0.0, xmax * 1.05)
    ax.set_ylim(ymin - 20.0, ymax + 20.0)
    ax.legend(loc="best", frameon=True)
    fig.tight_layout()
    plot_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(plot_file)
    plt.close(fig)


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare FPROPS Tier 3 boundary to a Baur-Glaessner fit.")
    ap.add_argument(
        "--fit",
        choices=sorted(list(FIT_SPECS.keys()) + ["all-h2-current"]),
        required=True,
        help="Named fitted boundary curve.",
    )
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--gas-source",
        default="reaktoro_clone_supcrt98",
        help="Gas mu0 source for hydrogen and water.",
    )
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000",
        help="Comma-separated temperatures in Celsius.",
    )
    ap.add_argument(
        "--plot-file",
        type=Path,
        default=None,
        help="PNG output path. Defaults to a file beside this script.",
    )
    ap.add_argument(
        "--plot-model-step-c",
        type=float,
        default=10.0,
        help="Temperature step in Celsius for the model curve in the PNG.",
    )
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)

    if not args.runner.exists():
        print(f"Runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_c = parse_temps_c(args.temps_c)
    if args.fit == "all-h2-current":
        specs = [FIT_SPECS["h2-fe-wustite"], FIT_SPECS["h2-wustite-spinel"], FIT_SPECS["h2-fe-magnetite"]]
        plot_file = args.plot_file if args.plot_file is not None else default_all_plot_file(args.gas_source)
        write_all_plot(specs, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c)
        print("Baur-Glaessner comparison: all current H2 fits")
        print(f"Gas source: {args.gas_source}")
        print(f"Wrote PNG plot to {plot_file}")
        print("Included curves:")
        for spec in specs:
            print(f"- {spec.name}")
        return 0

    spec = FIT_SPECS[args.fit]
    fn = boundary_fn(spec.boundary)
    plot_file = args.plot_file if args.plot_file is not None else default_plot_file(spec, args.gas_source)

    print(f"Baur-Glaessner comparison: {spec.name}")
    print(spec.note)
    print(f"Fit x-range: {spec.xmin:.9f} to {spec.xmax:.9f}")
    print(f"Fit T-range: {spec.ymin_c:.3f} to {spec.ymax_c:.3f} C")
    print(f"Gas source: {args.gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'GOD_fit':>12} {'GOD_model':>12} {'dGOD':>12} "
        f"{'log10 fit':>12} {'log10 model':>12} {'dlog10':>12} {'status':>10}"
    )

    sum_sq = 0.0
    ncomp = 0
    max_abs_dlog = 0.0

    for tc in temps_c:
        tk = tc + 273.15
        fit_god = fit_god_at_temp(spec, tc)
        model_god, log10_model = cached_model_god_at_temp(spec.boundary, str(args.runner), args.gas_source, tc)

        if fit_god is None:
            print(
                f"{tc:6.0f} {'-':>12} {model_god:12.6f} {'-':>12} "
                f"{'-':>12} {log10_model:12.6f} {'-':>12} {'out-of-fit':>10}"
            )
            continue

        log10_fit = log10_ratio_from_god(fit_god)
        dgod = model_god - fit_god
        dlog = log10_model - log10_fit
        sum_sq += dlog * dlog
        ncomp += 1
        max_abs_dlog = max(max_abs_dlog, abs(dlog))
        print(
            f"{tc:6.0f} {fit_god:12.6f} {model_god:12.6f} {dgod:12.6f} "
            f"{log10_fit:12.6f} {log10_model:12.6f} {dlog:12.6f} {'ok':>10}"
        )

    print()
    if ncomp:
        rms_dlog = math.sqrt(sum_sq / ncomp)
        print(f"Compared points: {ncomp}")
        print(f"RMS delta log10(H2O/H2): {rms_dlog:.6f}")
        print(f"Max abs delta log10(H2O/H2): {max_abs_dlog:.6f}")
    else:
        print("Compared points: 0")
    print()
    print("Interpretation")
    print("- `GOD` is assumed here to mean p(H2O) / (p(H2) + p(H2O)).")
    print("- Large dlog10 indicates a substantial shift in the reduction boundary.")
    print("- This comparison is only as good as that GOD-axis interpretation.")
    write_plot(spec, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c)
    print(f"- Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
