#!/usr/bin/env python3
"""
Compare FPROPS Ni/NiO/H2/H2O standard reaction thermodynamics against
reference values generated with Reaktoro (SUPCRT98).

Reaction:
    NiO(s) + H2(g) <-> Ni(s) + H2O(g)

Reference table source (user-provided):
    reaktoro1.py using SupcrtDatabase("supcrt98")
    Species: Nickel, Bunsenite, H2(g), H2O(g)

How to run:
    1) Build runner:
         scons models/johnpye/fprops/test/eqm_mu0_runner -j4
    2) Compare:
         python3 models/johnpye/fprops/test/ni_reaktoro_compare.py
    3) Strict CI-style exit on tolerance:
         python3 models/johnpye/fprops/test/ni_reaktoro_compare.py --strict \
             --dg-tol 5000 --log10k-tol 0.25
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

R = 8.31446261815324
P0 = 1e5


@dataclass(frozen=True)
class RefPoint:
    tc: float
    dg_j_per_mol: float
    k_ratio: float


REFERENCE = [
    RefPoint(400.0, -31849.5, 2.961e2),
    RefPoint(600.0, -37551.5, 1.764e2),
    RefPoint(800.0, -41528.6, 1.050e2),
    RefPoint(900.0, -42859.2, 8.096e1),
    RefPoint(1000.0, -43746.9, 6.235e1),
    RefPoint(1200.0, -44183.1, 3.686e1),
]


def default_runner() -> Path:
    return Path(__file__).resolve().parent / "eqm_mu0_runner"


def fprops_mu0(runner: Path, source: str, tk: float) -> tuple[float, float, float, float]:
    cmd = [
        str(runner),
        source,
        f"{tk:.12g}",
        f"{P0:.12g}",
        "Ni",
        "NiO",
        "hydrogen",
        "water",
    ]
    out = subprocess.check_output(cmd, text=True)
    data = json.loads(out)
    mu = data["mu0"]
    if any(v is None for v in mu):
        raise RuntimeError(f"Missing mu0 values at T={tk:.2f} K: {mu}")
    return float(mu[0]), float(mu[1]), float(mu[2]), float(mu[3])


def fprops_dg_k(runner: Path, source: str, tc: float) -> tuple[float, float]:
    tk = tc + 273.15
    mu_ni, mu_nio, mu_h2, mu_h2o = fprops_mu0(runner, source, tk)
    dg = (mu_ni + mu_h2o) - (mu_nio + mu_h2)
    k = math.exp(-dg / (R * tk))
    return dg, k


def main() -> int:
    ap = argparse.ArgumentParser(description="Ni/NiO Reaktoro vs FPROPS comparison.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--source", default="Moran and Shapiro", help="FPROPS source argument.")
    ap.add_argument("--strict", action="store_true", help="Return nonzero if tolerance exceeded.")
    ap.add_argument("--dg-tol", type=float, default=5000.0, help="|dG_FPROPS - dG_ref| tolerance [J/mol].")
    ap.add_argument(
        "--log10k-tol",
        type=float,
        default=0.25,
        help="|log10(K_FPROPS) - log10(K_ref)| tolerance.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"Runner not found: {args.runner}", file=sys.stderr)
        return 2

    print("NiO(s) + H2(g) <-> Ni(s) + H2O(g)")
    print(f"FPROPS source: {args.source}")
    print(f"{'T[C]':>6} {'dG_ref':>12} {'dG_fprops':>12} {'ddG':>12} {'K_ref':>11} {'K_fprops':>11} {'dlog10K':>10}")

    bad = 0
    for p in REFERENCE:
        dg, k = fprops_dg_k(args.runner, args.source, p.tc)
        ddg = dg - p.dg_j_per_mol
        dlogk = math.log10(k) - math.log10(p.k_ratio)
        print(
            f"{p.tc:6.0f} "
            f"{p.dg_j_per_mol:12.1f} "
            f"{dg:12.1f} "
            f"{ddg:12.1f} "
            f"{p.k_ratio:11.3e} "
            f"{k:11.3e} "
            f"{dlogk:10.3f}"
        )
        if abs(ddg) > args.dg_tol or abs(dlogk) > args.log10k_tol:
            bad += 1

    if args.strict and bad:
        print(
            f"\nFAILED: {bad}/{len(REFERENCE)} points exceeded tolerances "
            f"(dg_tol={args.dg_tol:g}, log10k_tol={args.log10k_tol:g})."
        )
        return 1

    if bad:
        print(
            f"\nNOTE: {bad}/{len(REFERENCE)} points exceed suggested tolerances. "
            "This is expected until source-data/reference-state alignment is finalized."
        )
    else:
        print("\nAll points within tolerances.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

