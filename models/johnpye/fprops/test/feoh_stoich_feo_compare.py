#!/usr/bin/env python3
"""
Compare the current Fe|wustite H2/H2O boundary against a simplified
stoichiometric Fe|FeO boundary using the placeholder const-cp solids.

This is a diagnostic only. It asks whether stripping out wustite
nonstoichiometry materially moves the hydrogen reduction line.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from feoh_hydrogen_boundary import (
    default_runner,
    gas_ratio_logs,
    oxygen_potential_at_fe_wustite_boundary,
    parse_temps_c,
    query_mu0,
)

R = 8.31446261815324
P0 = 1e5


def log10_ratio_stoich_fe_feo(runner: Path, gas_source: str, tc: float) -> float:
    tk = tc + 273.15
    gas = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
    solid = query_mu0(runner, "constcp:ellingham_placeholder", tk, ["Fe", "FeO"])
    dg = solid["Fe"] + gas["water"] - solid["FeO"] - gas["hydrogen"]
    return -dg / (R * tk * math.log(10.0))


def god_from_log10_ratio(log10_h2o_h2: float) -> float:
    ratio = 10.0 ** log10_h2o_h2
    return ratio / (1.0 + ratio)


def current_log10_ratio(runner: Path, gas_source: str, tc: float) -> tuple[float, float]:
    tk = tc + 273.15
    x_best, lam_o = oxygen_potential_at_fe_wustite_boundary(tk)
    mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
    log10_ratio, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, tk)
    return x_best, log10_ratio


def default_plot_file(gas_source: str) -> Path:
    gas = gas_source.lower().replace(" ", "_")
    return Path(__file__).resolve().parent / f"feoh_stoich_feo_compare_{gas}.png"


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare current Fe|wustite vs stoich Fe|FeO H2/H2O boundaries.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--gas-source",
        default="reaktoro_clone_supcrt98",
        help="Gas mu0 source for hydrogen and water.",
    )
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000,1100,1200",
        help="Comma-separated temperatures in Celsius.",
    )
    ap.add_argument(
        "--plot-file",
        type=Path,
        default=None,
        help="PNG output path. Defaults to a file beside this script.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        raise SystemExit(f"Runner not found: {args.runner}")

    temps_c = parse_temps_c(args.temps_c)
    plot_file = args.plot_file if args.plot_file is not None else default_plot_file(args.gas_source)

    print("Fe|H2/H2O boundary diagnostic")
    print(f"Gas source: {args.gas_source}")
    print("Condensed curves:")
    print("- current Hidayat Fe|wustite")
    print("- simplified constcp Fe|FeO(stoich)")
    print()
    print(
        f"{'T[C]':>6} {'x_wus':>10} {'log10 current':>15} {'log10 Fe/FeO':>15} "
        f"{'delta':>12} {'GOD current':>12} {'GOD Fe/FeO':>12}"
    )

    cur_logs = []
    stoich_logs = []
    xvals = []
    for tc in temps_c:
        x_best, log_cur = current_log10_ratio(args.runner, args.gas_source, tc)
        log_stoich = log10_ratio_stoich_fe_feo(args.runner, args.gas_source, tc)
        cur_logs.append(log_cur)
        stoich_logs.append(log_stoich)
        xvals.append(x_best)
        print(
            f"{tc:6.0f} {x_best:10.6f} {log_cur:15.6f} {log_stoich:15.6f} "
            f"{(log_stoich-log_cur):12.6f} {god_from_log10_ratio(log_cur):12.6f} "
            f"{god_from_log10_ratio(log_stoich):12.6f}"
        )

    fig, ax = plt.subplots(figsize=(7.2, 5.2), dpi=160)
    ax.plot([god_from_log10_ratio(v) for v in cur_logs], temps_c, color="#1f4e79", lw=2.2, ls="-", marker="o", label="FPROPS Fe|wustite")
    ax.plot([god_from_log10_ratio(v) for v in stoich_logs], temps_c, color="#b55200", lw=2.2, ls="--", marker="s", label="constcp Fe|FeO")
    ax.set_xlabel("GOD = p(H2O) / (p(H2) + p(H2O))")
    ax.set_ylabel("Temperature (C)")
    ax.set_title("Fe|H2/H2O boundary: current vs stoichiometric FeO")
    ax.grid(True, color="#d9d9d9", lw=0.7)
    ax.legend(loc="best", frameon=True)
    fig.tight_layout()
    plot_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(plot_file)
    plt.close(fig)
    print()
    print(f"Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
