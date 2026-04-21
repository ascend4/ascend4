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
    oxygen_potential_at_fe_spinel_boundary_bg_tuned,
    oxygen_potential_at_fe_spinel_boundary_lambda_fit,
    oxygen_potential_at_fe_spinel_boundary_mmc1_guess,
    oxygen_potential_at_fe_spinel_boundary_mmc1_selective_best,
    oxygen_potential_at_fe_spinel_boundary_mmc1_tapered_fit,
    oxygen_potential_at_fe_spinel_boundary,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary_state_mmc1_guess,
    oxygen_potential_at_wustite_spinel_boundary_state_mmc1_tapered_fit,
    oxygen_potential_at_wustite_spinel_boundary,
    trace_wustite_spinel_boundary_mmc1_guess,
    trace_wustite_spinel_boundary_mmc1_selective_best,
    trace_wustite_spinel_boundary_mmc1_tapered_fit,
    trace_wustite_spinel_boundary,
    parse_temps_c,
    query_mu0,
)


@dataclass(frozen=True)
class FitSpec:
    name: str
    boundary: str | None
    fit_kind: str
    coeffs: tuple[float, float, float, float, float]
    points: tuple[tuple[float, float], ...]
    xmin: float
    xmax: float
    ymin_c: float
    ymax_c: float
    note: str


FIT_SPECS: dict[str, FitSpec] = {
    "h2-fe-wustite": FitSpec(
        name="H2 Fe|wustite",
        boundary="fe-wustite",
        fit_kind="raw_points",
        coeffs=(0.0, 0.0, 0.0, 0.0, 0.0),
        points=(
            (0.23456245187, 570.019013451),
            (0.245296282085, 592.590131422),
            (0.255653986024, 614.912837142),
            (0.265820081712, 636.619322895),
            (0.276332434119, 661.081479321),
            (0.28587264008, 682.801879077),
            (0.295194336274, 704.894663517),
            (0.304440542997, 728.550763493),
            (0.314278233378, 751.960141983),
            (0.322901794238, 775.194442504),
            (0.332080050299, 798.659339747),
            (0.34091136509, 823.547651351),
            (0.348323683793, 845.580742473),
            (0.356207135385, 868.888850295),
            (0.363887111598, 892.091017732),
            (0.371545484269, 917.140024584),
            (0.379672257281, 940.778255429),
            (0.386656009969, 964.517824917),
            (0.392734556921, 986.338851943),
            (0.396007774564, 999.490234859),
        ),
        xmin=0.23456245187,
        xmax=0.396007774564,
        ymin_c=570.019013451,
        ymax_c=999.490234859,
        note="User-supplied hand-traced H2 Fe|wustite line.",
    ),
    "h2-wustite-spinel": FitSpec(
        name="H2 wustite|spinel",
        boundary="wustite-spinel",
        fit_kind="raw_points",
        coeffs=(0.0, 0.0, 0.0, 0.0, 0.0),
        points=(
            (0.239636951896, 570.802435201),
            (0.26050174532, 580.54667932),
            (0.280873172483, 589.174170906),
            (0.302575011377, 599.15855048),
            (0.323005844537, 607.571969383),
            (0.343902933586, 616.598976108),
            (0.365361486289, 626.038166261),
            (0.386057891025, 634.520983141),
            (0.407381453732, 643.547840821),
            (0.428479295225, 652.703107093),
            (0.449059606332, 661.619647942),
            (0.470615227345, 670.566281858),
            (0.49083018369, 679.879098617),
            (0.512137360845, 689.332544171),
            (0.5333888167, 698.63992637),
            (0.553100275071, 707.819009833),
            (0.574465809385, 717.92777879),
            (0.595291048461, 728.43389926),
            (0.615035358174, 739.036771584),
            (0.635904031524, 749.995835088),
            (0.655768343041, 761.207343924),
            (0.675471200887, 773.088962713),
            (0.695367585689, 784.940587177),
            (0.714406754152, 797.953058136),
            (0.733389609597, 811.66502176),
            (0.752528006667, 824.830642117),
            (0.770755480745, 840.545929068),
            (0.788963418297, 856.556431896),
            (0.80572786869, 872.585699551),
            (0.821795065664, 889.906116431),
            (0.837937983872, 909.781941876),
            (0.85288725963, 928.918132929),
            (0.866386490957, 949.369068968),
            (0.880422170032, 973.543407948),
            (0.890140238673, 991.257119644),
            (0.894130342583, 999.467304699),
        ),
        xmin=0.239636951896,
        xmax=0.894130342583,
        ymin_c=570.802435201,
        ymax_c=999.467304699,
        note="User-supplied hand-traced H2 wustite|spinel line.",
    ),
    "h2-fe-spinel": FitSpec(
        name="H2 Fe|spinel",
        boundary="fe-spinel",
        fit_kind="raw_points",
        coeffs=(0.0, 0.0, 0.0, 0.0, 0.0),
        points=(
            (0.0324563391631, 300.168143858),
            (0.0426429152512, 324.117602543),
            (0.0543387848168, 347.496404941),
            (0.065007540708, 366.150699826),
            (0.0789270759127, 388.298687389),
            (0.0934618420789, 409.099758867),
            (0.106420951095, 426.12554569),
            (0.122031864011, 445.592220804),
            (0.136824638483, 462.83294041),
            (0.153775819851, 481.58547164),
            (0.168845620361, 498.137780549),
            (0.185316158674, 515.570661982),
            (0.202668265865, 532.581218262),
            (0.219718975701, 549.656298345),
            (0.235228173231, 564.783638887),
            (0.239096097533, 569.079556973),
        ),
        xmin=0.0324563391631,
        xmax=0.239096097533,
        ymin_c=300.168143858,
        ymax_c=569.079556973,
        note="User-supplied hand-traced H2 Fe|spinel line.",
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
    "fe-spinel": {
        "color": "#2b7a0b",
        "label": "Fe|spinel",
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
    if spec.fit_kind == "raw_points":
        for (x0, y0), (x1, y1) in zip(spec.points, spec.points[1:]):
            if y0 <= target_c <= y1:
                if y1 == y0:
                    return x0
                frac = (target_c - y0) / (y1 - y0)
                return x0 + frac * (x1 - x0)
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


def boundary_fn(name: str, spinel_variant: str = "current"):
    if spinel_variant == "mmc1_selective_best":
        spinel_variant = "hidayat_adj1"
    if name == "fe-wustite":
        return oxygen_potential_at_fe_wustite_boundary
    if name == "wustite-spinel":
        if spinel_variant == "mmc1_guess":
            return None
        if spinel_variant == "mmc1_tapered_fit":
            return None
        if spinel_variant == "hidayat_adj1":
            return None
        return oxygen_potential_at_wustite_spinel_boundary
    if name == "fe-spinel":
        if spinel_variant == "bg_tuned":
            return oxygen_potential_at_fe_spinel_boundary_bg_tuned
        if spinel_variant == "lambda_fit":
            return oxygen_potential_at_fe_spinel_boundary_lambda_fit
        if spinel_variant == "mmc1_guess":
            return oxygen_potential_at_fe_spinel_boundary_mmc1_guess
        if spinel_variant == "mmc1_tapered_fit":
            return oxygen_potential_at_fe_spinel_boundary_mmc1_tapered_fit
        if spinel_variant == "hidayat_adj1":
            return oxygen_potential_at_fe_spinel_boundary_mmc1_selective_best
        return oxygen_potential_at_fe_spinel_boundary
    raise KeyError(name)


def batch_model_god_at_temps(
    boundary_name: str,
    runner: Path,
    gas_source: str,
    temps_c: list[float],
    spinel_variant: str = "current",
) -> dict[float, tuple[float, float]]:
    if spinel_variant == "mmc1_selective_best":
        spinel_variant = "hidayat_adj1"
    if boundary_name != "wustite-spinel":
        return {
            tc: cached_model_god_at_temp(boundary_name, str(runner), gas_source, tc, spinel_variant)
            for tc in temps_c
        }

    temps_k = [tc + 273.15 for tc in temps_c]
    if spinel_variant == "mmc1_guess":
        states = trace_wustite_spinel_boundary_mmc1_guess(temps_k)
    elif spinel_variant == "mmc1_tapered_fit":
        states = trace_wustite_spinel_boundary_mmc1_tapered_fit(temps_k)
    elif spinel_variant == "hidayat_adj1":
        states = trace_wustite_spinel_boundary_mmc1_selective_best(temps_k)
    else:
        states = trace_wustite_spinel_boundary(temps_k)
    out: dict[float, tuple[float, float]] = {}
    for tc, (_x_best, lam_o, _a, _b, _v, _resid) in zip(temps_c, states):
        tk = tc + 273.15
        mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_model, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, tk)
        out[tc] = (god_from_log10_ratio(log10_model), log10_model)
    return out


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
def cached_model_god_at_temp(boundary_name: str, runner_str: str, gas_source: str, tc: float,
        spinel_variant: str = "current") -> tuple[float, float]:
    fn = boundary_fn(boundary_name, spinel_variant)
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


def default_plot_file(spec: FitSpec, gas_source: str, spinel_variant: str = "current") -> Path:
    stem = spec.name.lower().replace("|", "-").replace(" ", "_")
    gas = (
        gas_source.lower()
        .replace(" ", "_")
        .replace("+", "_plus_")
        .replace(":", "")
    )
    suffix = "" if spinel_variant == "current" else f"_{spinel_variant}"
    return Path(__file__).resolve().parent / f"bg_compare_{stem}_{gas}{suffix}.png"


def default_all_plot_file(gas_source: str, spinel_variant: str = "current") -> Path:
    gas = (
        gas_source.lower()
        .replace(" ", "_")
        .replace("+", "_plus_")
        .replace(":", "")
    )
    suffix = "" if spinel_variant == "current" else f"_{spinel_variant}"
    return Path(__file__).resolve().parent / f"bg_compare_all_h2_{gas}{suffix}.png"


def write_plot(
    spec: FitSpec,
    runner: Path,
    gas_source: str,
    sample_temps_c: list[float],
    plot_file: Path,
    plot_model_step_c: float,
    spinel_variant: str,
) -> None:
    style_key = spec.boundary if spec.boundary is not None else "fe-spinel"
    style = PLOT_STYLE[style_key]

    if spec.fit_kind == "raw_points":
        fit_xs = [x for x, _y in spec.points]
        fit_ys = [y for _x, y in spec.points]
    elif spec.fit_kind == "poly_t_of_god":
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
        plot_tmin = spec.ymin_c
        plot_tmax = spec.ymax_c
        model_temps = frange(plot_tmin, plot_tmax, plot_model_step_c)
        model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
        model_gods = [model_series[tc][0] for tc in model_temps]

    sample_fit_x = []
    sample_fit_y = []
    sample_model_x = []
    sample_model_y = []
    sample_model_series = {}
    if spec.boundary is not None:
        valid_sample_temps = [tc for tc in sample_temps_c if spec.ymin_c <= tc <= spec.ymax_c]
        sample_model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
    for tc in sample_temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        if fit_god is not None:
            sample_fit_x.append(fit_god)
            sample_fit_y.append(tc)
        if spec.boundary is not None and tc in sample_model_series:
            model_god, _ = sample_model_series[tc]
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
    spinel_variant: str,
) -> None:
    fig, ax = plt.subplots(figsize=(7.6, 5.6), dpi=160)

    xmax = 0.0
    ymin = min(sample_temps_c)
    ymax = max(sample_temps_c)

    for spec in specs:
        style_key = spec.boundary if spec.boundary is not None else "fe-spinel"
        style = PLOT_STYLE[style_key]

        if spec.fit_kind == "raw_points":
            fit_xs = [x for x, _y in spec.points]
            fit_ys = [y for _x, y in spec.points]
        elif spec.fit_kind == "poly_t_of_god":
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
            model_temps = frange(spec.ymin_c, spec.ymax_c, plot_model_step_c)
            model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
            model_gods = [model_series[tc][0] for tc in model_temps]

        sample_fit_x = []
        sample_fit_y = []
        sample_model_x = []
        sample_model_y = []
        valid_sample_temps = [tc for tc in sample_temps_c if spec.boundary is not None and spec.ymin_c <= tc <= spec.ymax_c]
        sample_model_series = (
            batch_model_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
            if spec.boundary is not None
            else {}
        )
        for tc in sample_temps_c:
            fit_god = fit_god_at_temp(spec, tc)
            if fit_god is not None:
                sample_fit_x.append(fit_god)
                sample_fit_y.append(tc)
            if spec.boundary is not None and tc in sample_model_series:
                model_god, _ = sample_model_series[tc]
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
        default="helmholtz+ref0:",
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
    ap.add_argument(
        "--spinel-variant",
        choices=("current", "bg_tuned", "lambda_fit", "mmc1_guess", "mmc1_tapered_fit", "hidayat_adj1", "mmc1_selective_best"),
        default="current",
        help="Fe|spinel branch variant to use in the comparison.",
    )
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)

    if not args.runner.exists():
        print(f"Runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_c = parse_temps_c(args.temps_c)
    if args.fit == "all-h2-current":
        specs = [FIT_SPECS["h2-fe-wustite"], FIT_SPECS["h2-wustite-spinel"], FIT_SPECS["h2-fe-spinel"]]
        plot_file = args.plot_file if args.plot_file is not None else default_all_plot_file(args.gas_source, args.spinel_variant)
        write_all_plot(specs, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c, args.spinel_variant)
        print("Baur-Glaessner comparison: all current H2 fits")
        print(f"Gas source: {args.gas_source}")
        print(f"Fe|spinel variant: {args.spinel_variant}")
        print(f"Wrote PNG plot to {plot_file}")
        print("Included curves:")
        for spec in specs:
            print(f"- {spec.name}")
        return 0

    spec = FIT_SPECS[args.fit]
    plot_file = args.plot_file if args.plot_file is not None else default_plot_file(spec, args.gas_source, args.spinel_variant)

    print(f"Baur-Glaessner comparison: {spec.name}")
    print(spec.note)
    print(f"Fit x-range: {spec.xmin:.9f} to {spec.xmax:.9f}")
    print(f"Fit T-range: {spec.ymin_c:.3f} to {spec.ymax_c:.3f} C")
    print(f"Gas source: {args.gas_source}")
    print(f"Fe|spinel variant: {args.spinel_variant}")
    print()
    print(
        f"{'T[C]':>6} {'GOD_fit':>12} {'GOD_model':>12} {'dGOD':>12} "
        f"{'log10 fit':>12} {'log10 model':>12} {'dlog10':>12} {'status':>10}"
    )

    sum_sq = 0.0
    ncomp = 0
    max_abs_dlog = 0.0

    model_series = batch_model_god_at_temps(spec.boundary, args.runner, args.gas_source, temps_c, args.spinel_variant)

    for tc in temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        model_god, log10_model = model_series[tc]

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
    write_plot(spec, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c, args.spinel_variant)
    print(f"- Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
