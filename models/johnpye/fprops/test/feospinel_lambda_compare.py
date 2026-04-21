#!/usr/bin/env python3
"""
Compare the low-temperature Fe|spinel BG traces on a common oxygen-potential
basis.

The usual GOD plots can make the H2 and CO reduced-side errors look very
different because the transforms from lambda_O to GOD have very different
slopes. This script removes that distortion by:

- taking the traced H2 and CO Fe|spinel BG lines,
- converting them back to implied oxygen potential lambda_O(T), and
- comparing those directly against the current reduced-spinels model.

This is diagnostic only; it does not introduce a new thermodynamic model.
"""

from __future__ import annotations

import argparse
import math
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from feo_hidayat_validation import R
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2_god
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co_god
from feoh_hydrogen_boundary import (
    default_runner,
    normalize_gas_source,
    oxygen_potential_at_fe_spinel_boundary,
    oxygen_potential_at_fe_spinel_boundary_bg_tuned,
    parse_temps_c,
    query_mu0,
)

THIS_DIR = Path(__file__).resolve().parent


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


def fit_lambda_rows(runner: Path, gas_source: str, temps_c: list[float]) -> list[dict[str, float]]:
    rows: list[dict[str, float]] = []
    for tc in temps_c:
        tk = tc + 273.15
        _phase, lam_model = oxygen_potential_at_fe_spinel_boundary(tk)
        row: dict[str, float] = {"tc": tc, "lam_model": lam_model}

        god_h2 = fit_h2_god(H2_SPECS["h2-fe-spinel"], tc)
        if god_h2 is not None:
            row["god_h2_fit"] = god_h2
            row["lam_h2_fit"] = lambda_from_h2_god(runner, gas_source, tc, god_h2)
            row["dlam_h2"] = lam_model - row["lam_h2_fit"]

        god_co = fit_co_god(CO_SPECS["co-fe-spinel"], tc)
        if god_co is not None:
            row["god_co_fit"] = god_co
            row["lam_co_fit"] = lambda_from_co_god(runner, gas_source, tc, god_co)
            row["dlam_co"] = lam_model - row["lam_co_fit"]

        rows.append(row)
    return rows


def rms(values: list[float]) -> float:
    if not values:
        return math.nan
    return math.sqrt(sum(v * v for v in values) / len(values))


def default_plot_file(gas_source: str) -> Path:
    gas = gas_source.lower().replace(" ", "_").replace("+", "_plus_").replace(":", "")
    return THIS_DIR / f"feospinel_lambda_compare_{gas}.png"


def write_plot(rows: list[dict[str, float]], plot_file: Path) -> None:
    temps = [r["tc"] for r in rows]
    lam_model = [r["lam_model"] / 1000.0 for r in rows]

    fig, (ax0, ax1) = plt.subplots(2, 1, figsize=(8.0, 8.0), sharex=True)

    ax0.plot(temps, lam_model, color="#2b7a0b", lw=2.0, label="Model Fe|spinel")

    h2_rows = [r for r in rows if "lam_h2_fit" in r]
    if h2_rows:
        ax0.plot(
            [r["tc"] for r in h2_rows],
            [r["lam_h2_fit"] / 1000.0 for r in h2_rows],
            color="#1f4e79",
            marker="o",
            ms=4,
            lw=1.4,
            label="BG H2 Fe|spinel",
        )
        ax1.plot(
            [r["tc"] for r in h2_rows],
            [r["dlam_h2"] / 1000.0 for r in h2_rows],
            color="#1f4e79",
            marker="o",
            ms=4,
            lw=1.4,
            label="Model - BG (H2)",
        )

    co_rows = [r for r in rows if "lam_co_fit" in r]
    if co_rows:
        ax0.plot(
            [r["tc"] for r in co_rows],
            [r["lam_co_fit"] / 1000.0 for r in co_rows],
            color="#7a1f5c",
            marker="s",
            ms=4,
            lw=1.4,
            label="BG CO Fe|spinel",
        )
        ax1.plot(
            [r["tc"] for r in co_rows],
            [r["dlam_co"] / 1000.0 for r in co_rows],
            color="#7a1f5c",
            marker="s",
            ms=4,
            lw=1.4,
            label="Model - BG (CO)",
        )

    ax0.set_ylabel(r"$\lambda_O$ [kJ/mol O]")
    ax0.set_title("Fe|spinel Reduced-Side Comparison on Common Oxygen-Potential Basis")
    ax0.grid(True, alpha=0.25)
    ax0.legend(loc="best")

    ax1.axhline(0.0, color="#555555", lw=1.0, ls="--")
    ax1.set_xlabel("Temperature [C]")
    ax1.set_ylabel(r"$\Delta \lambda_O$ [kJ/mol O]")
    ax1.grid(True, alpha=0.25)
    ax1.legend(loc="best")

    fig.tight_layout()
    fig.savefig(plot_file, dpi=160)
    plt.close(fig)


def print_rows(rows: list[dict[str, float]]) -> None:
    print("Fe|spinel oxygen-potential comparison")
    print()
    print(
        f"{'T[C]':>6} {'lambda_model':>14} {'lambda_h2_fit':>14} {'dlam_h2':>12} "
        f"{'lambda_co_fit':>14} {'dlam_co':>12}"
    )
    for r in rows:
        def fmt(key: str) -> str:
            val = r.get(key)
            return f"{val/1000.0:14.6f}" if val is not None else f"{'-':>14}"

        print(
            f"{r['tc']:6.0f} {fmt('lam_model')} {fmt('lam_h2_fit')} {fmt('dlam_h2')} "
            f"{fmt('lam_co_fit')} {fmt('dlam_co')}"
        )
    print()

    h2_vals = [r["dlam_h2"] / 1000.0 for r in rows if "dlam_h2" in r]
    co_vals = [r["dlam_co"] / 1000.0 for r in rows if "dlam_co" in r]
    if h2_vals:
        print(f"H2 RMS delta lambda_O: {rms(h2_vals):.6f} kJ/mol O")
    if co_vals:
        print(f"CO RMS delta lambda_O: {rms(co_vals):.6f} kJ/mol O")
    print()
    print("Interpretation")
    print("- This puts the H2 and CO reduced-side BG traces onto the same thermodynamic scale.")
    print("- If the CO error looks much worse on GOD plots but only moderately worse here, the visual difference is mainly the gas-ratio transform.")
    print("- Hidayat and Degterov constrain the oxide-side spinel thermodynamics directly, but they do not provide an equally direct Fe|spinel gas-boundary trace.")


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare H2 and CO Fe|spinel BG traces on a common lambda_O basis.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source for H2/H2O and CO/CO2.")
    ap.add_argument("--temps-c", default="350,450,550", help="Comma-separated temperatures in Celsius.")
    ap.add_argument("--plot-file", type=Path, default=None, help="Optional output PNG path.")
    args = ap.parse_args()

    if not args.runner.exists():
        raise SystemExit(f"runner not found: {args.runner}")

    gas_source = normalize_gas_source(args.gas_source)
    temps_c = parse_temps_c(args.temps_c)
    rows = fit_lambda_rows(args.runner, gas_source, temps_c)
    print_rows(rows)

    plot_file = args.plot_file if args.plot_file is not None else default_plot_file(gas_source)
    write_plot(rows, plot_file)
    print(f"- Wrote PNG plot to {plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
