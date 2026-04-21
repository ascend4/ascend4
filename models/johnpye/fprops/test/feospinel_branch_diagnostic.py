#!/usr/bin/env python3
"""
Low-temperature Fe|spinel branch diagnostic.

This script compares three condensed-side constructions:

1. current reduced Fe|spinel helper from feoh_hydrogen_boundary.py
2. a refined solve of that same reduced Fe-only spinel model
3. stoichiometric Fe|Fe3O4

It then projects each oxygen potential onto both H2/H2O and CO/CO2
using the aligned gas basis and compares against the traced BG/Spreitzer
Fe|spinel branches.
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from feo_hidayat_validation import spinel_phase_g, g_fe_bcc, g_fe_fcc
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co
from feoh_hydrogen_boundary import (
    default_runner,
    gas_ratio_logs,
    normalize_gas_source,
    oxygen_potential_at_fe_magnetite_boundary,
    oxygen_potential_at_fe_spinel_boundary,
    query_mu0,
)

R = 8.31446261815324


def stable_fe_label(T: float) -> str:
    return "bcc" if g_fe_bcc(T) <= g_fe_fcc(T) else "fcc"


def stable_fe_g(T: float) -> float:
    return g_fe_bcc(T) if stable_fe_label(T) == "bcc" else g_fe_fcc(T)


def oxygen_potential_at_fe_spinel_boundary_refined(T: float) -> tuple[str, float, float, float]:
    """
    Same reduced Fe-only spinel thermodynamics as the production helper,
    but with a tighter local search over the two composition coordinates.
    """
    gfe = stable_fe_g(T)
    best = math.inf
    best_a = 0.0
    best_b = 0.5
    a_lo = 0.0
    a_hi = 1.0

    for outer in range(6):
        steps_a = 160 if outer < 2 else 240
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
            bmax = 0.5 * (1.0 - a)
            if bmax <= 0.0:
                continue
            if math.isfinite(best_a) and abs(a - best_a) < 0.15:
                b_center = min(max(best_b, 0.0), bmax)
                b_span = 0.12 if outer < 2 else 0.05
                b_lo = max(0.0, b_center - b_span)
                b_hi = min(bmax, b_center + b_span)
            else:
                b_lo = 0.0
                b_hi = bmax
            steps_b = 160 if outer < 2 else 240
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.0025, 0.10 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)

    return stable_fe_label(T), best, best_a, best_b


def god_from_log10_ratio(log10_ratio: float) -> float:
    ratio = 10.0 ** log10_ratio
    return ratio / (1.0 + ratio)


def co_log10_ratio(mu_co: float, mu_co2: float, lam_o: float, T: float) -> float:
    return (lam_o - (mu_co2 - mu_co)) / (R * T * math.log(10.0))


def main() -> int:
    ap = argparse.ArgumentParser(description="Diagnostic compare for the low-T Fe|spinel branch.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Aligned gas source for H2/H2O and CO/CO2.")
    ap.add_argument("--temps-c", default="350,450,550", help="Comma-separated temperatures in Celsius.")
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 2

    gas_source = normalize_gas_source(args.gas_source)
    temps_c = [float(tok.strip()) for tok in args.temps_c.split(",") if tok.strip()]
    h2_spec = H2_SPECS["h2-fe-spinel"]
    co_spec = CO_SPECS["co-fe-spinel"]

    print("Fe|spinel branch diagnostic")
    print(f"gas source: {gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'H2 BG':>10} {'H2 cur':>10} {'H2 ref':>10} {'H2 sto':>10} "
        f"{'CO BG':>10} {'CO cur':>10} {'CO ref':>10} {'CO sto':>10}"
    )

    for tc in temps_c:
        tk = tc + 273.15
        mu_h = query_mu0(args.runner, gas_source, tk, ["hydrogen", "water"])
        mu_c = query_mu0(args.runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])

        _phase_cur, lam_cur = oxygen_potential_at_fe_spinel_boundary(tk)
        _phase_ref, lam_ref, a_ref, b_ref = oxygen_potential_at_fe_spinel_boundary_refined(tk)
        _phase_sto, lam_sto = oxygen_potential_at_fe_magnetite_boundary(tk)

        log_h2_cur, _ = gas_ratio_logs(mu_h["hydrogen"], mu_h["water"], lam_cur, tk)
        log_h2_ref, _ = gas_ratio_logs(mu_h["hydrogen"], mu_h["water"], lam_ref, tk)
        log_h2_sto, _ = gas_ratio_logs(mu_h["hydrogen"], mu_h["water"], lam_sto, tk)

        log_co_cur = co_log10_ratio(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_cur, tk)
        log_co_ref = co_log10_ratio(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_ref, tk)
        log_co_sto = co_log10_ratio(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_sto, tk)

        h2_bg = fit_h2(h2_spec, tc)
        co_bg = fit_co(co_spec, tc)

        print(
            f"{tc:6.0f} "
            f"{h2_bg if h2_bg is not None else float('nan'):10.6f} "
            f"{god_from_log10_ratio(log_h2_cur):10.6f} "
            f"{god_from_log10_ratio(log_h2_ref):10.6f} "
            f"{god_from_log10_ratio(log_h2_sto):10.6f} "
            f"{co_bg if co_bg is not None else float('nan'):10.6f} "
            f"{god_from_log10_ratio(log_co_cur):10.6f} "
            f"{god_from_log10_ratio(log_co_ref):10.6f} "
            f"{god_from_log10_ratio(log_co_sto):10.6f}"
        )
        print(
            f"       refined search state: a={a_ref:.6f}, b={b_ref:.6f}, "
            f"lambda delta(current-refined)={lam_cur - lam_ref:.3f} J/mol O"
        )

    print()
    print("Interpretation")
    print("- If `current` and `refined` match closely, the residual slope is not a search-resolution artifact.")
    print("- If `sto` is noticeably better than both reduced-spinel curves, the reduced CEF itself is the likely culprit.")
    print("- If `sto` is also poor, the issue is broader than the reduced spinel composition search.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
