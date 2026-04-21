#!/usr/bin/env python3
"""
Direct Fe|wustite|CO/CO2 coexistence check against the BG/Spreitzer trace.

This script avoids the oxygen-potential shortcut by solving the mixed
Fe-O-C equilibrium directly for a fixed T,P and a scan of bulk oxygen totals.
It then inspects points where metallic Fe and wustite coexist and reports the
resulting gas oxidation degree:

    GOD = xCO2 / (xCO + xCO2)

If the direct-equilibrium GOD is flat across the coexistence window, that
plateau can be compared against:
  - the BG/Spreitzer traced Fe|wustite curve, and
  - the current oxygen-potential-based Fe|wustite CO harness.
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from feoc_baur_glaessner_compare import FIT_SPECS, fit_god_at_temp, cached_model_god_at_temp

CASE = "feoc_fe_wustite"
SOURCE_MAP = (
    "Fe_bcc=hidayat_2015;"
    "Fe_fcc=hidayat_2015;"
    "Wus_FeO=hidayat_2015;"
    "Wus_FeO1p5=hidayat_2015;"
    "carbonmonoxide=reaktoro_clone_supcrt98;"
    "carbondioxide=reaktoro_clone_supcrt98"
)
P0 = 1e5


def parse_csv_floats(text: str) -> list[float]:
    vals = [float(tok.strip()) for tok in text.split(",") if tok.strip()]
    if not vals:
        raise ValueError("no values provided")
    return vals


def frange(start: float, stop: float, step: float) -> list[float]:
    vals: list[float] = []
    x = start
    while x <= stop + 1e-12:
        vals.append(round(x, 12))
        x += step
    if not vals or vals[-1] < stop - 1e-9:
        vals.append(stop)
    return vals


def default_runner() -> Path:
    return THIS_DIR / "eqm_case_runner"


def default_mu0_runner() -> Path:
    return THIS_DIR / "eqm_mu0_runner"


def run_case(runner: Path, tk: float, p: float, algorithm: str, b_fe: float, b_c: float, b_o: float) -> dict:
    proc = subprocess.run(
        [
            str(runner),
            CASE,
            f"{tk:.17g}",
            f"{p:.17g}",
            algorithm,
            SOURCE_MAP,
            f"{b_fe:.17g},{b_c:.17g},{b_o:.17g}",
        ],
        text=True,
        capture_output=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"runner failed ({proc.returncode}) at T={tk:.2f} K, bO={b_o:.6g}\n{proc.stderr}"
        )
    lines = [line for line in proc.stdout.splitlines() if line.strip()]
    if not lines:
        raise RuntimeError(f"runner returned no output at T={tk:.2f} K, bO={b_o:.6g}")
    return json.loads(lines[-1])


def coexistence_metrics(data: dict) -> tuple[float, float, float]:
    species = data["species"]
    n = data["n"]
    if n is None:
        return math.nan, math.nan, math.nan
    idx = {name: i for i, name in enumerate(species)}
    n_fe = n[idx["Fe_bcc"]] + n[idx["Fe_fcc"]]
    n_wus = n[idx["Wus_FeO"]] + n[idx["Wus_FeO1p5"]]
    n_co = n[idx["carbonmonoxide"]]
    n_co2 = n[idx["carbondioxide"]]
    god = n_co2 / (n_co + n_co2) if (n_co + n_co2) > 0.0 else math.nan
    return n_fe, n_wus, god


def is_ok_status(status: int) -> bool:
    return status in (0, 1, 6)


def main() -> int:
    ap = argparse.ArgumentParser(description="Direct Fe|wustite|CO/CO2 coexistence audit.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_case_runner.")
    ap.add_argument("--mu0-runner", type=Path, default=default_mu0_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--temps-c", default="650,750,850,950", help="Comma-separated temperatures in Celsius.")
    ap.add_argument("--pressure", type=float, default=P0)
    ap.add_argument("--algorithm", default="auto")
    ap.add_argument("--b-fe", type=float, default=1.0)
    ap.add_argument("--b-c", type=float, default=1.0)
    ap.add_argument("--o-min", type=float, default=1.2)
    ap.add_argument("--o-max", type=float, default=2.0)
    ap.add_argument("--o-step", type=float, default=0.05)
    ap.add_argument("--fe-tol", type=float, default=1e-6)
    ap.add_argument("--wustite-tol", type=float, default=1e-6)
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 2
    if not args.mu0_runner.exists():
        print(f"mu0 runner not found: {args.mu0_runner}", file=sys.stderr)
        return 2

    temps_c = parse_csv_floats(args.temps_c)
    b_oxygen = frange(args.o_min, args.o_max, args.o_step)
    bg_spec = FIT_SPECS["co-fe-wustite"]

    print("Direct Fe|wustite|CO/CO2 coexistence audit")
    print(f"runner: {args.runner}")
    print(f"source: {SOURCE_MAP}")
    print(f"bulk totals scanned: Fe={args.b_fe:g}, C={args.b_c:g}, O=[{args.o_min:g}, {args.o_max:g}] step {args.o_step:g}")
    print()
    print(
        f"{'T[C]':>6} {'Ncoex':>6} {'GOD direct':>12} {'spread':>12} "
        f"{'GOD lambda':>12} {'GOD BG':>12} {'d(dir-lam)':>12} {'d(dir-BG)':>12}"
    )

    for tc in temps_c:
        tk = tc + 273.15
        coexist_gods: list[float] = []
        for bo in b_oxygen:
            data = run_case(args.runner, tk, args.pressure, args.algorithm, args.b_fe, args.b_c, bo)
            if not is_ok_status(data["status"]):
                continue
            n_fe, n_wus, god = coexistence_metrics(data)
            if not (math.isfinite(n_fe) and math.isfinite(n_wus) and math.isfinite(god)):
                continue
            if n_fe > args.fe_tol and n_wus > args.wustite_tol:
                coexist_gods.append(god)

        god_lambda = cached_model_god_at_temp("fe-wustite", str(args.mu0_runner), "reaktoro_clone_supcrt98", tc)[0]
        god_bg = fit_god_at_temp(bg_spec, tc)

        if coexist_gods:
            god_direct = sum(coexist_gods) / len(coexist_gods)
            spread = max(coexist_gods) - min(coexist_gods)
            dd_lambda = god_direct - god_lambda
            dd_bg = god_direct - god_bg if god_bg is not None else math.nan
            print(
                f"{tc:6.0f} {len(coexist_gods):6d} {god_direct:12.6f} {spread:12.6f} "
                f"{god_lambda:12.6f} "
                f"{god_bg if god_bg is not None else float('nan'):12.6f} "
                f"{dd_lambda:12.6f} "
                f"{dd_bg if math.isfinite(dd_bg) else float('nan'):12.6f}"
            )
        else:
            print(
                f"{tc:6.0f} {0:6d} {'nan':>12} {'nan':>12} "
                f"{god_lambda:12.6f} "
                f"{god_bg if god_bg is not None else float('nan'):12.6f} "
                f"{'nan':>12} {'nan':>12}"
            )

    print()
    print("Interpretation")
    print("- `GOD direct` comes from full mixed Fe-O-C equilibrium solves, not from an inferred oxygen potential.")
    print("- Small `spread` means the gas ratio is effectively invariant across the Fe|wustite coexistence window.")
    print("- If `GOD direct` matches `GOD lambda`, the oxygen-potential construction is not the cause of any BG mismatch.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
