#!/usr/bin/env python3
"""
Compare the reduced-spinels state selected on the Fe|spinel and
wustite|spinel branches.

This helps answer a practical question for low-temperature debugging:

if the two branches sit in very different parts of the reduced spinel
composition space, then a composition-selective Gibbs correction might be able
to move Fe|spinel without dragging wustite|spinel with it.

If, instead, the selected spinel states are already very similar near the
problem temperature range, then any smooth Gibbs correction inside the current
reduced model is likely to move both branches together.
"""

from __future__ import annotations

import argparse
import math

from feo_hidayat_validation import spinel_phase_g
from feoh_hydrogen_boundary import (
    oxygen_potential_at_fe_spinel_boundary,
    oxygen_potential_at_fe_spinel_boundary_lambda_fit,
    oxygen_potential_at_wustite_spinel_boundary_state,
    stable_fe_g,
)


def fe_spinel_state(T: float) -> tuple[float, float, float, float, float]:
    """
    Return the reduced-spinels state selected on the Fe|spinel branch.

    Output is:
        a, b, n_fe, y_o_fe3, y_o_va
    """
    gfe = stable_fe_g(T)
    best = math.inf
    best_a = 0.0
    best_b = 0.5
    best_state = (math.nan, math.nan, math.nan)
    a_lo = 0.0
    a_hi = 1.0

    for _ in range(4):
        span_a = a_hi - a_lo
        for ia in range(81):
            a = a_lo + span_a * ia / 80.0
            bmax = 0.5 * (1.0 - a)
            if bmax <= 0.0:
                continue
            if math.isfinite(best_a) and abs(a - best_a) < 0.2:
                b_center = min(max(best_b, 0.0), bmax)
                b_lo = max(0.0, b_center - 0.15)
                b_hi = min(bmax, b_center + 0.15)
            else:
                b_lo = 0.0
                b_hi = bmax
            for ib in range(81):
                b = b_lo + (b_hi - b_lo) * ib / 80.0
                g, n_fe, y_o_fe3, y_o_va = spinel_phase_g(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
                    best_state = (n_fe, y_o_fe3, y_o_va)
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)

    n_fe, y_o_fe3, y_o_va = best_state
    return best_a, best_b, n_fe, y_o_fe3, y_o_va


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare reduced spinel states on Fe|spinel and wustite|spinel branches.")
    ap.add_argument("--temps-c", default="350,450,550,560,570,700,900", help="Comma-separated temperatures in Celsius.")
    args = ap.parse_args()

    temps_c = [float(tok.strip()) for tok in args.temps_c.split(",") if tok.strip()]
    if not temps_c:
        raise SystemExit("no temperatures provided")

    print("Reduced-spinels branch state comparison")
    print()
    print(
        f"{'T[C]':>6} "
        f"{'a_fe|sp':>9} {'b_fe|sp':>9} {'yo3_fe|sp':>10} "
        f"{'a_w|sp':>9} {'b_w|sp':>9} {'yo3_w|sp':>10} "
        f"{'dlam_fit':>10}"
    )

    for tc in temps_c:
        tk = tc + 273.15
        a_fe, b_fe, _n_fe, y3_fe, _v_fe = fe_spinel_state(tk)
        _x_ws, _lam_ws, a_ws, b_ws, _v_ws, _res_ws = oxygen_potential_at_wustite_spinel_boundary_state(tk)
        _phase, lam_cur = oxygen_potential_at_fe_spinel_boundary(tk)
        _phase2, lam_fit = oxygen_potential_at_fe_spinel_boundary_lambda_fit(tk)
        print(
            f"{tc:6.0f} "
            f"{a_fe:9.5f} {b_fe:9.5f} {y3_fe:10.5f} "
            f"{a_ws:9.5f} {b_ws:9.5f} {(a_ws + 5.0 - 4.0 * b_ws) / 6.0:10.5f} "
            f"{(lam_fit - lam_cur) / 1000.0:10.5f}"
        )

    print()
    print("Interpretation")
    print("- If the Fe|spinel and wustite|spinel states are already very similar near 560 C, a smooth reduced-spinels Gibbs correction will likely move both branches together.")
    print("- In that case, the branch-only lambda fit is useful as a localization target, but not as evidence for an easy composition-selective repair inside the current reduced model.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
