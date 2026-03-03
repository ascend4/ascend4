#!/usr/bin/env python3
"""
Internal consistency audit for the Fe|wustite|H2|H2O boundary.

This compares three ways of constructing the equilibrium gas ratio:

1. Oxygen-potential route:
      lambda_O = mu_H2O - mu_H2

2. Direct endmember reaction route:
      FeO + H2 <-> Fe + H2O

3. Direct whole-phase reaction route at the boundary composition x:
      FeO_(1+x/2) + (1+x/2) H2 <-> Fe + (1+x/2) H2O

If all three agree closely, then the current internal Fe|wustite boundary
construction is self-consistent, and the large mismatch to external
Baur-Glaessner fits must come from elsewhere.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from feo_hidayat_validation import (
    R,
    g0_feo,
    g0_feo1p5,
    gex_wustite,
    mu_a_wustite,
    mu_b_wustite,
)
from feoh_hydrogen_boundary import (
    default_runner,
    oxygen_potential_at_fe_wustite_boundary,
    parse_temps_c,
    query_mu0,
    stable_fe_g,
)


def g_wustite(T: float, x: float) -> float:
    xa = 1.0 - x
    xb = x
    return (
        xa * g0_feo(T)
        + xb * g0_feo1p5(T)
        + R * T * (xa * math.log(xa) + xb * math.log(xb))
        + gex_wustite(x)
    )


def log10_ratio_oxygen_potential(T: float, x: float, mu_h2: float, mu_h2o: float) -> float:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    return (lam_o - (mu_h2o - mu_h2)) / (R * T * math.log(10.0))


def log10_ratio_endmember_a(T: float, x: float, mu_h2: float, mu_h2o: float) -> float:
    mu_a = mu_a_wustite(T, x)
    gfe = stable_fe_g(T)
    return (mu_a - gfe - (mu_h2o - mu_h2)) / (R * T * math.log(10.0))


def log10_ratio_whole_phase(T: float, x: float, mu_h2: float, mu_h2o: float) -> float:
    gw = g_wustite(T, x)
    gfe = stable_fe_g(T)
    nu_h2o = 1.0 + 0.5 * x
    mu_o_from_phase = (gw - gfe) / nu_h2o
    return (mu_o_from_phase - (mu_h2o - mu_h2)) / (R * T * math.log(10.0))


def main() -> int:
    ap = argparse.ArgumentParser(description="Fe|wustite H2/H2O internal consistency audit.")
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
    args = ap.parse_args()

    temps_c = parse_temps_c(args.temps_c)
    print("Fe|wustite|H2|H2O internal consistency audit")
    print(f"Gas source: {args.gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'x_wus':>10} "
        f"{'log10 lamO':>13} {'log10 FeO':>13} {'log10 phase':>13} "
        f"{'d(FeO-lamO)':>14} {'d(phase-lamO)':>16}"
    )

    max_abs_d_a = 0.0
    max_abs_d_p = 0.0

    for tc in temps_c:
        tk = tc + 273.15
        x, _lam_o = oxygen_potential_at_fe_wustite_boundary(tk)
        mu = query_mu0(args.runner, args.gas_source, tk, ["hydrogen", "water"])
        mu_h2 = mu["hydrogen"]
        mu_h2o = mu["water"]
        log_lamo = log10_ratio_oxygen_potential(tk, x, mu_h2, mu_h2o)
        log_a = log10_ratio_endmember_a(tk, x, mu_h2, mu_h2o)
        log_p = log10_ratio_whole_phase(tk, x, mu_h2, mu_h2o)
        da = log_a - log_lamo
        dp = log_p - log_lamo
        max_abs_d_a = max(max_abs_d_a, abs(da))
        max_abs_d_p = max(max_abs_d_p, abs(dp))
        print(
            f"{tc:6.0f} {x:10.6f} "
            f"{log_lamo:13.6f} {log_a:13.6f} {log_p:13.6f} "
            f"{da:14.6e} {dp:16.6e}"
        )

    print()
    print(f"Max abs delta endmember-vs-lamO: {max_abs_d_a:.6e}")
    print(f"Max abs delta whole-phase-vs-lamO: {max_abs_d_p:.6e}")
    print()
    print("Interpretation")
    print("- Near-zero deltas mean the internal Fe|wustite boundary construction is self-consistent.")
    print("- Large external discrepancies must then come from basis alignment, gas/condensed dataset mixing, or diagram-axis interpretation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
