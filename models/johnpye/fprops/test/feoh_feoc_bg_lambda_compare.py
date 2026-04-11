#!/usr/bin/env python3
"""
Compare the traced H2/H2O and CO/CO2 Baur-Glaessner / Spreitzer curves on
the same oxygen-potential basis, without FPROPS in the loop.

This isolates the cross-gas part of the discrepancy:

    delta_lambda_bg = lambda_BG(CO) - lambda_BG(H2)

for the corresponding Fe-O branch. If this is non-zero, then some part of the
H2-vs-CO mismatch sits in the traced gas-side datasets or their interpretation,
not in the condensed-phase model.
"""

from __future__ import annotations

import argparse
import math
import os
from dataclasses import dataclass
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from feo_hidayat_validation import R
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2_god
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co_god
from feoh_hydrogen_boundary import default_runner, normalize_gas_source, parse_temps_c, query_mu0

THIS_DIR = Path(__file__).resolve().parent


@dataclass(frozen=True)
class BranchPair:
    name: str
    h2_spec: str
    co_spec: str
    default_temps_c: tuple[float, ...]


BRANCHES: dict[str, BranchPair] = {
    "fe-wustite": BranchPair(
        "Fe|wustite",
        "h2-fe-wustite",
        "co-fe-wustite",
        (600.0, 650.0, 700.0, 750.0, 800.0, 850.0, 900.0, 950.0),
    ),
    "wustite-spinel": BranchPair(
        "wustite|spinel",
        "h2-wustite-spinel",
        "co-wustite-spinel",
        (600.0, 650.0, 700.0, 750.0, 800.0, 850.0, 900.0, 950.0),
    ),
    "fe-spinel": BranchPair(
        "Fe|spinel",
        "h2-fe-spinel",
        "co-fe-spinel",
        (350.0, 400.0, 450.0, 500.0, 550.0),
    ),
}


def lambda_from_h2_god(runner: Path, gas_source: str, tc: float, god: float) -> float:
    tk = tc + 273.15
    mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
    ratio = god / (1.0 - god)
    return mu["water"] - mu["hydrogen"] + R * tk * math.log(ratio)


def lambda_from_co_god(runner: Path, gas_source: str, tc: float, god: float) -> float:
    tk = tc + 273.15
    mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
    ratio = god / (1.0 - god)
    return mu["carbondioxide"] - mu["carbonmonoxide"] + R * tk * math.log(ratio)


def rms(vals: list[float]) -> float:
    if not vals:
        return math.nan
    return math.sqrt(sum(v * v for v in vals) / len(vals))


def rows_for_branch(
    branch: BranchPair,
    runner: Path,
    gas_source: str,
    temps_c: list[float],
) -> list[dict[str, float]]:
    rows: list[dict[str, float]] = []
    for tc in temps_c:
        god_h2 = fit_h2_god(H2_SPECS[branch.h2_spec], tc)
        god_co = fit_co_god(CO_SPECS[branch.co_spec], tc)
        if god_h2 is None or god_co is None:
            continue
        lam_h2 = lambda_from_h2_god(runner, gas_source, tc, god_h2)
        lam_co = lambda_from_co_god(runner, gas_source, tc, god_co)
        rows.append(
            {
                "tc": tc,
                "god_h2": god_h2,
                "god_co": god_co,
                "lam_h2": lam_h2,
                "lam_co": lam_co,
                "delta_lambda_bg": lam_co - lam_h2,
            }
        )
    return rows


def default_plot_file(gas_source: str) -> Path:
    gas = gas_source.lower().replace(" ", "_").replace("+", "_plus_").replace(":", "")
    return THIS_DIR / f"bg_lambda_crossgas_compare_{gas}.png"


def write_plot(
    branch_rows: list[tuple[BranchPair, list[dict[str, float]]]],
    plot_file: Path,
) -> None:
    fig, axes = plt.subplots(3, 1, figsize=(8.2, 10.0), sharex=False)
    colors = {
        "Fe|wustite": "#1f4e79",
        "wustite|spinel": "#b55200",
        "Fe|spinel": "#2b7a0b",
    }
    for ax, (branch, rows) in zip(axes, branch_rows):
        temps = [r["tc"] for r in rows]
        lam_h2 = [r["lam_h2"] / 1000.0 for r in rows]
        lam_co = [r["lam_co"] / 1000.0 for r in rows]
        dlam = [r["delta_lambda_bg"] / 1000.0 for r in rows]
        color = colors[branch.name]
        ax.plot(temps, lam_h2, color=color, lw=1.8, marker="o", ms=4, label="BG H2")
        ax.plot(temps, lam_co, color="#7a1f5c", lw=1.6, marker="s", ms=4, label="BG CO")
        ax2 = ax.twinx()
        ax2.plot(temps, dlam, color="#444444", lw=1.2, ls="--", marker="x", ms=4, label="CO - H2")
        ax.set_title(branch.name)
        ax.set_ylabel(r"$\lambda_O$ [kJ/mol O]")
        ax2.set_ylabel(r"$\Delta \lambda_{BG}$ [kJ/mol O]")
        ax.grid(True, alpha=0.25)
        lines = ax.get_lines() + ax2.get_lines()
        labels = [ln.get_label() for ln in lines]
        ax.legend(lines, labels, loc="best")
    axes[-1].set_xlabel("Temperature [C]")
    fig.tight_layout()
    fig.savefig(plot_file, dpi=160)
    plt.close(fig)


def print_branch(branch: BranchPair, rows: list[dict[str, float]]) -> None:
    print(branch.name)
    print(
        f"{'T[C]':>6} {'GOD_H2':>10} {'GOD_CO':>10} "
        f"{'lambda_H2':>14} {'lambda_CO':>14} {'CO-H2':>12}"
    )
    for r in rows:
        print(
            f"{r['tc']:6.0f} {r['god_h2']:10.6f} {r['god_co']:10.6f} "
            f"{r['lam_h2']/1000.0:14.6f} {r['lam_co']/1000.0:14.6f} "
            f"{r['delta_lambda_bg']/1000.0:12.6f}"
        )
    vals = [r["delta_lambda_bg"] / 1000.0 for r in rows]
    if vals:
        print(f"RMS delta lambda_BG (CO - H2): {rms(vals):.6f} kJ/mol O")
    print()


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare traced H2 and CO BG curves on a common oxygen-potential basis.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source for H2/H2O and CO/CO2.")
    ap.add_argument(
        "--branches",
        default="fe-wustite,wustite-spinel,fe-spinel",
        help="Comma-separated branch names.",
    )
    ap.add_argument(
        "--temps-c",
        default=None,
        help="Optional comma-separated temperatures in Celsius applied to every selected branch.",
    )
    ap.add_argument("--plot-file", type=Path, default=None, help="Optional PNG output path.")
    args = ap.parse_args()

    if not args.runner.exists():
        raise SystemExit(f"runner not found: {args.runner}")

    gas_source = normalize_gas_source(args.gas_source)
    branch_names = [tok.strip() for tok in args.branches.split(",") if tok.strip()]
    selected = [BRANCHES[name] for name in branch_names]
    override_temps = parse_temps_c(args.temps_c) if args.temps_c else None

    branch_rows: list[tuple[BranchPair, list[dict[str, float]]]] = []
    print("Cross-gas BG comparison on common oxygen-potential basis")
    print(f"Gas source for lambda conversion: {gas_source}")
    print()
    for branch in selected:
        temps_c = list(override_temps) if override_temps is not None else list(branch.default_temps_c)
        rows = rows_for_branch(branch, args.runner, gas_source, temps_c)
        branch_rows.append((branch, rows))
        print_branch(branch, rows)

    plot_file = args.plot_file if args.plot_file is not None else default_plot_file(gas_source)
    write_plot(branch_rows, plot_file)
    print("Interpretation")
    print("- `delta_lambda_BG = lambda_BG(CO) - lambda_BG(H2)` is independent of FPROPS.")
    print("- Non-zero values here mean the traced H2 and CO BG curves do not imply the same oxygen potential for the corresponding oxide boundary.")
    print("- That isolates cross-gas / cross-trace inconsistency from the condensed-phase model mismatch.")
    print(f"- Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
