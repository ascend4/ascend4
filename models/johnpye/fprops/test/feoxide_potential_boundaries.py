#!/usr/bin/env python3
"""
Direct oxide-side oxygen-potential helpers for the pure-Fe Fe-O system.

This script currently exposes the reduced-spinels benchmark that is still
missing from the main validation story: the `spinel|Fe2O3` boundary.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from feo_hidayat_validation import g_fe2o3, g_fe3o4, minimize_spinel_grand_residual_degterov
from feoh_hydrogen_boundary import (
    default_runner,
    gas_ratio_logs,
    normalize_gas_source,
    parse_temps_c,
    query_mu0,
)
from feoc_baur_glaessner_compare import gas_ratio_logs as co_ratio_logs


def oxygen_potential_at_spinel_hematite_boundary(T: float) -> tuple[float, float, float]:
    """
    Compute the oxygen potential for the reduced Fe-only spinel / hematite
    boundary.

    For a trial oxygen potential lambda_O, hematite implies

        lambda_Fe = (g_Fe2O3 - 3 lambda_O) / 2

    and the reduced spinel phase is co-stable when the minimized grand
    potential

        min_{a,b} [ g_spinel(a,b) - lambda_Fe n_Fe(a,b) - 4 lambda_O ]

    reaches zero.
    """

    lam_lo = -350000.0
    lam_hi = -120000.0
    best_a = math.nan
    best_b = math.nan

    def signed_residual(lam_o: float, a_hint: float | None, b_hint: float | None) -> tuple[float, float, float]:
        lam_fe = (g_fe2o3(T) - 3.0 * lam_o) / 2.0
        resid, a_best, b_best, _v_best = minimize_spinel_grand_residual_degterov(
            T, lam_fe, lam_o, a_hint=a_hint, b_hint=b_hint
        )
        return resid, a_best, b_best

    r_lo, a_lo, b_lo = signed_residual(lam_lo, None, None)
    r_hi, a_hi, b_hi = signed_residual(lam_hi, None, None)
    if r_lo == 0.0:
        return lam_lo, a_lo, b_lo
    if r_hi == 0.0:
        return lam_hi, a_hi, b_hi
    if r_lo * r_hi > 0.0:
        raise RuntimeError(
            f"failed to bracket spinel|hematite boundary at T={T:.2f} K "
            f"(residuals {r_lo:.6g}, {r_hi:.6g})"
        )

    a_hint = a_lo
    b_hint = b_lo
    lo = lam_lo
    hi = lam_hi
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        r_mid, a_mid, b_mid = signed_residual(mid, a_hint, b_hint)
        if abs(r_mid) < 1e-6 or abs(hi - lo) < 1e-6:
            return mid, a_mid, b_mid
        if r_lo * r_mid <= 0.0:
            hi = mid
            r_hi = r_mid
            a_hi = a_mid
            b_hi = b_mid
        else:
            lo = mid
            r_lo = r_mid
            a_lo = a_mid
            b_lo = b_mid
        a_hint = a_mid
        b_hint = b_mid
    return 0.5 * (lo + hi), a_hint, b_hint


def print_table(runner: Path, gas_source: str, temps_c: list[float]) -> None:
    print("Reduced spinel|Fe2O3 boundary")
    print(f"Gas source: {gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'lambda_O [kJ/mol O]':>22} {'a_t(Fe2+)':>12} "
        f"{'b_o(Fe2+)':>12} {'log10(H2O/H2)':>15} {'log10(CO2/CO)':>15}"
    )
    for tc in temps_c:
        tk = tc + 273.15
        lam_o, a_best, b_best = oxygen_potential_at_spinel_hematite_boundary(tk)
        mu_h = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_h2o_h2, _ = gas_ratio_logs(mu_h["hydrogen"], mu_h["water"], lam_o, tk)
        mu_c = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_co2_co, _ = co_ratio_logs(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_o, tk)
        print(
            f"{tc:6.0f} {lam_o/1000.0:22.6f} {a_best:12.6f} "
            f"{b_best:12.6f} {log10_h2o_h2:15.6f} {log10_co2_co:15.6f}"
        )
    print()
    print("Interpretation")
    print("- This is the current reduced-spinels prediction for the oxidized spinel boundary.")
    print("- Once Fig. 10 / Fig. 11 data are traced, this table is the direct comparison point to use.")


def main() -> int:
    ap = argparse.ArgumentParser(description="Direct reduced spinel|Fe2O3 potential-boundary table.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source.")
    ap.add_argument("--temps-c", default="700,800,900,1000", help="Comma-separated temperatures in Celsius.")
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)
    temps_c = parse_temps_c(args.temps_c)
    print_table(args.runner, args.gas_source, temps_c)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
