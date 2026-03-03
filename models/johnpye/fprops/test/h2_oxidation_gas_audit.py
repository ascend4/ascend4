#!/usr/bin/env python3
"""
Gas-only audit for:

    H2(g) + 0.5 O2(g) <-> H2O(g)

This isolates gas-source effects from condensed Fe-O thermodynamics by comparing
standard-state reaction thermodynamics across:

  - FPROPS `reaktoro_clone_supcrt98`
  - FPROPS `Moran and Shapiro`
  - direct Reaktoro `SupcrtDatabase("supcrt98")`

How to run in the local Reaktoro environment:

    eval "$(micromamba shell hook --shell bash)"
    micromamba activate
    python3 models/johnpye/fprops/test/h2_oxidation_gas_audit.py
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

R = 8.31446261815324
P0 = 1e5


def parse_temps_c(text: str) -> list[float]:
    out: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            out.append(float(tok))
    if not out:
        raise ValueError("no temperatures provided")
    return out


def default_runner() -> Path:
    return Path(__file__).resolve().parent / "eqm_mu0_runner"


def fprops_mu0(runner: Path, source: str, tk: float) -> tuple[float, float, float]:
    out = subprocess.check_output(
        [str(runner), source, f"{tk:.12g}", f"{P0:.12g}", "hydrogen", "oxygen", "water"],
        text=True,
    )
    data = json.loads(out)
    mu = data["mu0"]
    if len(mu) != 3 or any(v is None or not math.isfinite(float(v)) for v in mu):
        raise RuntimeError(f"Non-finite FPROPS mu0 for source {source} at T={tk:.2f} K")
    return float(mu[0]), float(mu[1]), float(mu[2])


def reaktoro_mu0(tk: float) -> tuple[float, float, float]:
    try:
        from reaktoro import SupcrtDatabase
    except Exception as e:
        raise RuntimeError(f"Failed to import reaktoro: {e}") from e
    db = SupcrtDatabase("supcrt98")
    tc = tk - 273.15
    pbar = P0 / 1e5
    vals = []
    for name in ("H2(g)", "O2(g)", "H2O(g)"):
        sp = db.species(name)
        pr = sp.props(tc, "C", pbar, "bar")
        vals.append(float(pr.G0))
    if any(not math.isfinite(v) for v in vals):
        raise RuntimeError(f"Non-finite Reaktoro mu0 at T={tk:.2f} K")
    return vals[0], vals[1], vals[2]


def dg_logk(mu_h2: float, mu_o2: float, mu_h2o: float, tk: float) -> tuple[float, float]:
    dg = mu_h2o - mu_h2 - 0.5 * mu_o2
    log10k = -dg / (R * tk * math.log(10.0))
    return dg, log10k


def main() -> int:
    ap = argparse.ArgumentParser(description="Gas-only H2/O2/H2O source audit.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000",
        help="Comma-separated temperatures in Celsius.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"Runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_c = parse_temps_c(args.temps_c)
    print("Gas-only audit: H2 + 0.5 O2 <-> H2O")
    print("Reference: direct Reaktoro SupcrtDatabase(\"supcrt98\")")
    print()
    print(
        f"{'T[C]':>6} "
        f"{'log10K clone':>13} {'log10K M&S':>13} {'log10K Rkt':>13} "
        f"{'dlogK(M&S-clone)':>18} {'dlogK(clone-Rkt)':>18} {'dlogK(M&S-Rkt)':>18}"
    )

    for tc in temps_c:
        tk = tc + 273.15
        mu_h2_c, mu_o2_c, mu_h2o_c = fprops_mu0(args.runner, "reaktoro_clone_supcrt98", tk)
        mu_h2_m, mu_o2_m, mu_h2o_m = fprops_mu0(args.runner, "Moran and Shapiro", tk)
        mu_h2_r, mu_o2_r, mu_h2o_r = reaktoro_mu0(tk)

        _dg_c, logk_c = dg_logk(mu_h2_c, mu_o2_c, mu_h2o_c, tk)
        _dg_m, logk_m = dg_logk(mu_h2_m, mu_o2_m, mu_h2o_m, tk)
        _dg_r, logk_r = dg_logk(mu_h2_r, mu_o2_r, mu_h2o_r, tk)

        print(
            f"{tc:6.0f} "
            f"{logk_c:13.6f} {logk_m:13.6f} {logk_r:13.6f} "
            f"{(logk_m-logk_c):18.6f} {(logk_c-logk_r):18.6f} {(logk_m-logk_r):18.6f}"
        )

    print()
    print("Interpretation")
    print("- `reaktoro_clone_supcrt98` should track direct Reaktoro closely.")
    print("- The M&S gas source quantifies how much gas thermodynamics alone can move the reduction lines.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
