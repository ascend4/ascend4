#!/usr/bin/env python3
"""
Compare current FPROPS Fe-O-C boundaries against raw Baur-Glaessner /
Spreitzer-traced CO/CO2 curves on a common GOD basis.

Here:

    GOD = xCO2 / (xCO + xCO2)

following Spreitzer's definition of gas oxidation degree for the CO/CO2
subsystem.
"""

from __future__ import annotations

import argparse
import math
import os
import sys
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from feoh_hydrogen_boundary import (  # noqa: E402
    default_runner,
    normalize_gas_source,
    oxygen_potential_at_fe_spinel_boundary_bg_tuned,
    oxygen_potential_at_fe_spinel_boundary_lambda_fit,
    oxygen_potential_at_fe_spinel_boundary_mmc1_guess,
    oxygen_potential_at_fe_spinel_boundary_mmc1_selective_best,
    oxygen_potential_at_fe_spinel_boundary_mmc1_tapered_fit,
    oxygen_potential_at_fe_spinel_boundary,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
    trace_wustite_spinel_boundary_mmc1_guess,
    trace_wustite_spinel_boundary_mmc1_selective_best,
    trace_wustite_spinel_boundary_mmc1_tapered_fit,
    trace_wustite_spinel_boundary,
    parse_temps_c,
    query_mu0,
)
from fprops_phase_boundary import phase_boundary_lambda_o  # noqa: E402

R = 8.31446261815324


@dataclass(frozen=True)
class FitSpec:
    name: str
    boundary: str
    points: tuple[tuple[float, float], ...]
    xmin: float
    xmax: float
    ymin_c: float
    ymax_c: float
    note: str


def load_points(name: str) -> tuple[tuple[float, float], ...]:
    path = THIS_DIR / name
    pts: list[tuple[float, float]] = []
    with path.open("r", encoding="ascii") as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2:
                raise ValueError(f"bad data line in {path}: {line!r}")
            pts.append((float(parts[0]), float(parts[1])))
    if not pts:
        raise ValueError(f"no points loaded from {path}")
    pts.sort(key=lambda p: p[1])
    return tuple(pts)


def make_spec(name: str, boundary: str, filename: str, note: str) -> FitSpec:
    pts = load_points(filename)
    xs = [x for x, _y in pts]
    ys = [y for _x, y in pts]
    return FitSpec(
        name=name,
        boundary=boundary,
        points=pts,
        xmin=min(xs),
        xmax=max(xs),
        ymin_c=min(ys),
        ymax_c=max(ys),
        note=note,
    )


FIT_SPECS: dict[str, FitSpec] = {
    "co-fe-spinel": make_spec(
        "CO Fe|spinel",
        "fe-spinel",
        "spreitzer-baur-glassner-CO-fe-spin.dat",
        "User-supplied hand-traced CO/CO2 low-temperature branch via Spreitzer/BG; compared using the same Fe|spinel construction as the H2 harness.",
    ),
    "co-wustite-spinel": make_spec(
        "CO wustite|spinel",
        "wustite-spinel",
        "spreitzer-baur-glassner-CO-wust-spin.dat",
        "User-supplied hand-traced CO/CO2 upper branch via Spreitzer/BG; compared using the same wustite|spinel construction as the H2 harness.",
    ),
    "co-fe-wustite": make_spec(
        "CO Fe|wustite",
        "fe-wustite",
        "spreitzer-baur-glassner-CO-fe-wust.dat",
        "User-supplied hand-traced CO/CO2 Fe|wustite branch via Spreitzer/BG.",
    ),
}


PLOT_STYLE: dict[str, dict[str, str]] = {
    "fe-spinel": {"color": "#2b7a0b", "label": "Fe|spinel"},
    "wustite-spinel": {"color": "#b55200", "label": "wustite|spinel"},
    "fe-wustite": {"color": "#1f4e79", "label": "Fe|wustite"},
}


def fit_god_at_temp(spec: FitSpec, target_c: float) -> float | None:
    if target_c < spec.ymin_c or target_c > spec.ymax_c:
        return None
    for (x0, y0), (x1, y1) in zip(spec.points, spec.points[1:]):
        if y0 <= target_c <= y1:
            if y1 == y0:
                return x0
            frac = (target_c - y0) / (y1 - y0)
            return x0 + frac * (x1 - x0)
    return None


def god_from_log10_ratio(log10_co2_co: float) -> float:
    ratio = 10.0 ** log10_co2_co
    return ratio / (1.0 + ratio)


def log10_ratio_from_god(god: float) -> float:
    return math.log10(god / (1.0 - god))


def gas_ratio_logs(mu_co: float, mu_co2: float, lam_o: float, T: float) -> tuple[float, float]:
    ln_ratio = (lam_o - (mu_co2 - mu_co)) / (R * T)
    log10_co2_co = ln_ratio / math.log(10.0)
    log10_co_co2 = -log10_co2_co
    return log10_co2_co, log10_co_co2


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

    trace_temps_c = wustite_spinel_trace_temps_c(temps_c)
    temps_k = [tc + 273.15 for tc in trace_temps_c]
    if spinel_variant == "mmc1_guess":
        states = trace_wustite_spinel_boundary_mmc1_guess(temps_k)
    elif spinel_variant == "mmc1_tapered_fit":
        states = trace_wustite_spinel_boundary_mmc1_tapered_fit(temps_k)
    elif spinel_variant == "hidayat_adj1":
        states = trace_wustite_spinel_boundary_mmc1_selective_best(temps_k)
    else:
        states = trace_wustite_spinel_boundary(temps_k)
    traced: dict[float, tuple[float, float]] = {}
    for tc, (_x_best, lam_o, _a, _b, _v, _resid) in zip(trace_temps_c, states):
        tk = tc + 273.15
        mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_model, _ = gas_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
        traced[tc] = (god_from_log10_ratio(log10_model), log10_model)
    out: dict[float, tuple[float, float]] = {}
    for tc in temps_c:
        out[tc] = traced[tc]
    return out


def model_god_at_temp(fn, runner: Path, gas_source: str, tc: float) -> tuple[float, float]:
    tk = tc + 273.15
    first, lam_o = fn(tk)
    _unused = first
    mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
    log10_model, _ = gas_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
    return god_from_log10_ratio(log10_model), log10_model


@lru_cache(maxsize=None)
def cached_model_god_at_temp(boundary_name: str, runner_str: str, gas_source: str, tc: float,
        spinel_variant: str = "current") -> tuple[float, float]:
    fn = boundary_fn(boundary_name, spinel_variant)
    return model_god_at_temp(fn, Path(runner_str), gas_source, tc)


def wustite_spinel_trace_temps_c(temps_c: list[float], max_step_c: float = 10.0) -> list[float]:
    if not temps_c:
        return []
    targets = sorted({round(tc, 12) for tc in temps_c})
    start = min(targets[0], 570.0)
    stop = targets[-1]
    vals = set(targets)
    t = start
    while t <= stop + 1e-12:
        vals.add(round(t, 12))
        t += max_step_c
    return sorted(vals)


def phase_api_god_at_temp(
    boundary_name: str,
    runner: Path,
    gas_source: str,
    tc: float,
    spinel_variant: str = "current",
) -> tuple[float, float] | None:
    try:
        tk = tc + 273.15
        lam_o, _meta = phase_boundary_lambda_o(boundary_name, tk, spinel_variant)
        mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_model, _ = gas_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
        return god_from_log10_ratio(log10_model), log10_model
    except ValueError:
        return None


def batch_phase_api_god_at_temps(
    boundary_name: str,
    runner: Path,
    gas_source: str,
    temps_c: list[float],
    spinel_variant: str = "current",
) -> dict[float, tuple[float, float]]:
    out: dict[float, tuple[float, float]] = {}
    for tc in temps_c:
        value = phase_api_god_at_temp(boundary_name, runner, gas_source, tc, spinel_variant)
        if value is not None:
            out[tc] = value
    return out


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
    gas = gas_source.lower().replace(" ", "_").replace("+", "_plus_").replace(":", "")
    suffix = "" if spinel_variant == "current" else f"_{spinel_variant}"
    return THIS_DIR / f"bg_compare_{stem}_{gas}{suffix}.png"


def default_all_plot_file(gas_source: str, spinel_variant: str = "current") -> Path:
    gas = gas_source.lower().replace(" ", "_").replace("+", "_plus_").replace(":", "")
    suffix = "" if spinel_variant == "current" else f"_{spinel_variant}"
    return THIS_DIR / f"bg_compare_all_co_{gas}{suffix}.png"


def write_plot(
    spec: FitSpec,
    runner: Path,
    gas_source: str,
    sample_temps_c: list[float],
    plot_file: Path,
    plot_model_step_c: float,
    spinel_variant: str,
) -> None:
    style = PLOT_STYLE[spec.boundary]
    fit_xs = [x for x, _y in spec.points]
    fit_ys = [y for _x, y in spec.points]
    model_temps = frange(spec.ymin_c, spec.ymax_c, plot_model_step_c)
    model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
    model_gods = [model_series[tc][0] for tc in model_temps]
    phase_api_series = batch_phase_api_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
    phase_api_temps = [tc for tc in model_temps if tc in phase_api_series]
    phase_api_gods = [phase_api_series[tc][0] for tc in phase_api_temps]

    sample_fit_x = []
    sample_fit_y = []
    sample_model_x = []
    sample_model_y = []
    sample_phase_api_x = []
    sample_phase_api_y = []
    valid_sample_temps = [tc for tc in sample_temps_c if spec.ymin_c <= tc <= spec.ymax_c]
    sample_model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
    sample_phase_api_series = batch_phase_api_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
    for tc in sample_temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        if fit_god is not None:
            sample_fit_x.append(fit_god)
            sample_fit_y.append(tc)
        if tc in sample_model_series:
            model_god, _ = sample_model_series[tc]
            sample_model_x.append(model_god)
            sample_model_y.append(tc)
        if tc in sample_phase_api_series:
            phase_api_god, _ = sample_phase_api_series[tc]
            sample_phase_api_x.append(phase_api_god)
            sample_phase_api_y.append(tc)

    fig, ax = plt.subplots(figsize=(7.2, 5.4), dpi=160)
    ax.plot(fit_xs, fit_ys, color=style["color"], lw=2.2, ls="-", label="Spreitzer/BG points")
    ax.plot(model_gods, model_temps, color=style["color"], lw=2.2, ls=":", label="Python diagnostic")
    if phase_api_gods:
        ax.plot(phase_api_gods, phase_api_temps, color=style["color"], lw=1.8, ls="--", label="FPROPS phase API")
    ax.scatter(sample_fit_x, sample_fit_y, color=style["color"], s=18, marker="o", zorder=3, label="BG sample points")
    ax.scatter(sample_model_x, sample_model_y, color=style["color"], s=24, marker="x", zorder=3, label="Python sample points")
    if sample_phase_api_x:
        ax.scatter(sample_phase_api_x, sample_phase_api_y, color=style["color"], s=26, marker="+", zorder=3, label="Phase API sample points")

    ax.set_title(f"{spec.name}: BG vs FPROPS")
    ax.set_xlabel("GOD = xCO2 / (xCO + xCO2)")
    ax.set_ylabel("Temperature (C)")
    ax.grid(True, color="#d9d9d9", lw=0.7)
    ax.set_xlim(0.0, max(spec.xmax * 1.05, max(model_gods, default=0.0) * 1.05, max(phase_api_gods, default=0.0) * 1.05))
    ax.set_ylim(spec.ymin_c - 20.0, spec.ymax_c + 20.0)
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
    ymin = min(spec.ymin_c for spec in specs)
    ymax = max(spec.ymax_c for spec in specs)

    for spec in specs:
        style = PLOT_STYLE[spec.boundary]
        fit_xs = [x for x, _y in spec.points]
        fit_ys = [y for _x, y in spec.points]
        model_temps = frange(spec.ymin_c, spec.ymax_c, plot_model_step_c)
        model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
        model_gods = [model_series[tc][0] for tc in model_temps]
        phase_api_series = batch_phase_api_god_at_temps(spec.boundary, runner, gas_source, model_temps, spinel_variant)
        phase_api_temps = [tc for tc in model_temps if tc in phase_api_series]
        phase_api_gods = [phase_api_series[tc][0] for tc in phase_api_temps]

        sample_fit_x = []
        sample_fit_y = []
        sample_model_x = []
        sample_model_y = []
        sample_phase_api_x = []
        sample_phase_api_y = []
        valid_sample_temps = [tc for tc in sample_temps_c if spec.ymin_c <= tc <= spec.ymax_c]
        sample_model_series = batch_model_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
        sample_phase_api_series = batch_phase_api_god_at_temps(spec.boundary, runner, gas_source, valid_sample_temps, spinel_variant)
        for tc in sample_temps_c:
            fit_god = fit_god_at_temp(spec, tc)
            if fit_god is not None:
                sample_fit_x.append(fit_god)
                sample_fit_y.append(tc)
            if tc in sample_model_series:
                model_god, _ = sample_model_series[tc]
                sample_model_x.append(model_god)
                sample_model_y.append(tc)
            if tc in sample_phase_api_series:
                phase_api_god, _ = sample_phase_api_series[tc]
                sample_phase_api_x.append(phase_api_god)
                sample_phase_api_y.append(tc)

        ax.plot(fit_xs, fit_ys, color=style["color"], lw=2.2, ls="-", label=f"BG {style['label']}")
        ax.plot(model_gods, model_temps, color=style["color"], lw=2.2, ls=":", label=f"Python {style['label']}")
        if phase_api_gods:
            ax.plot(phase_api_gods, phase_api_temps, color=style["color"], lw=1.8, ls="--", label=f"Phase API {style['label']}")
        ax.scatter(sample_fit_x, sample_fit_y, color=style["color"], s=16, marker="o", zorder=3)
        ax.scatter(sample_model_x, sample_model_y, color=style["color"], s=22, marker="x", zorder=3)
        if sample_phase_api_x:
            ax.scatter(sample_phase_api_x, sample_phase_api_y, color=style["color"], s=24, marker="+", zorder=3)

        xmax = max(xmax, spec.xmax, max(model_gods, default=0.0), max(phase_api_gods, default=0.0))

    ax.set_title("CO Baur-Glaessner fits vs FPROPS")
    ax.set_xlabel("GOD = xCO2 / (xCO + xCO2)")
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
    ap = argparse.ArgumentParser(description="Compare FPROPS Fe-O-C boundaries to Spreitzer/BG CO fits.")
    ap.add_argument(
        "--fit",
        choices=sorted(list(FIT_SPECS.keys()) + ["all-co-current"]),
        required=True,
        help="Named fitted boundary curve.",
    )
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--gas-source",
        default="helmholtz+ref0:",
        help="Gas mu0 source for CO and CO2. The BG/Spreitzer comparison plots use the aligned helmholtz+ref0: basis by default.",
    )
    ap.add_argument(
        "--temps-c",
        default="350,450,550,650,750,850,950",
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
    if args.fit == "all-co-current":
        specs = [
            FIT_SPECS["co-fe-wustite"],
            FIT_SPECS["co-wustite-spinel"],
            FIT_SPECS["co-fe-spinel"],
        ]
        plot_file = args.plot_file if args.plot_file is not None else default_all_plot_file(args.gas_source, args.spinel_variant)
        write_all_plot(specs, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c, args.spinel_variant)
        print("Baur-Glaessner comparison: all current CO fits")
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
        f"{'T[C]':>6} {'GOD_fit':>12} {'GOD_py':>12} {'GOD_phase':>12} "
        f"{'log10 fit':>12} {'log10 py':>12} {'log10 phase':>12} {'phase-py':>12} {'status':>10}"
    )

    sum_sq = 0.0
    ncomp = 0
    max_abs_dlog = 0.0

    model_series = batch_model_god_at_temps(spec.boundary, args.runner, args.gas_source, temps_c, args.spinel_variant)
    phase_api_series = batch_phase_api_god_at_temps(spec.boundary, args.runner, args.gas_source, temps_c, args.spinel_variant)

    for tc in temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        model_god, log10_model = model_series[tc]
        phase_api = phase_api_series.get(tc)

        if fit_god is None:
            phase_god = phase_api[0] if phase_api is not None else math.nan
            phase_log = phase_api[1] if phase_api is not None else math.nan
            phase_delta = phase_log - log10_model if phase_api is not None else math.nan
            print(
                f"{tc:6.0f} {'-':>12} {model_god:12.6f} {phase_god:12.6f} "
                f"{'-':>12} {log10_model:12.6f} {phase_log:12.6f} {phase_delta:12.6f} {'out-of-fit':>10}"
            )
            continue

        log10_fit = log10_ratio_from_god(fit_god)
        dlog = log10_model - log10_fit
        phase_god = phase_api[0] if phase_api is not None else math.nan
        phase_log = phase_api[1] if phase_api is not None else math.nan
        phase_delta = phase_log - log10_model if phase_api is not None else math.nan
        sum_sq += dlog * dlog
        ncomp += 1
        max_abs_dlog = max(max_abs_dlog, abs(dlog))
        print(
            f"{tc:6.0f} {fit_god:12.6f} {model_god:12.6f} {phase_god:12.6f} "
            f"{log10_fit:12.6f} {log10_model:12.6f} {phase_log:12.6f} {phase_delta:12.6f} {'ok':>10}"
        )

    print()
    if ncomp:
        rms_dlog = math.sqrt(sum_sq / ncomp)
        print(f"Compared points: {ncomp}")
        print(f"RMS delta log10(CO2/CO): {rms_dlog:.6f}")
        print(f"Max abs delta log10(CO2/CO): {max_abs_dlog:.6f}")
    else:
        print("Compared points: 0")
    print()
    print("Interpretation")
    print("- `GOD` is taken as xCO2 / (xCO + xCO2), following Spreitzer.")
    print("- To stay consistent with the earlier H2/H2O BG work, the three CO branches are compared against the same Fe-O constructions: Fe|wustite, wustite|spinel, and Fe|spinel.")
    print("- `Python diagnostic` is the pre-existing boundary calculation; `phase API` is the new C phase-module route.")
    print("- Large dlog10 indicates a substantial shift in the reduction boundary.")
    write_plot(spec, args.runner, args.gas_source, temps_c, plot_file, args.plot_model_step_c, args.spinel_variant)
    print(f"- Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
