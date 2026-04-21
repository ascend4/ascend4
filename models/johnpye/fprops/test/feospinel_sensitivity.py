#!/usr/bin/env python3
"""
Sensitivity study for the low-temperature Fe|spinel branch.

This script compares a few simple correction families against the traced
H2 and CO Fe|spinel branches:

1. current reduced Fe-only spinel model
2. affine correction to the Fe3O4 endmember Gibbs energy
3. magnetic-term retuning only

The goal is to determine whether the residual low-T slope can plausibly be
removed by a small correction inside the reduced model, or whether a fuller
spinel treatment is required.
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from feo_hidayat_validation import spinel_phase_g, g_fe_bcc, g_fe_fcc, hillert_jarl_gmag
from feoh_hydrogen_boundary import query_mu0, default_runner
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co

R = 8.31446261815324

# Candidate corrections found in ad hoc low-T scans.
AFFINE_DG_AE = -16984.331545915302
AFFINE_DG_AE_T = 18.769347422265838
MAG_TORD = 1100.0
MAG_BETA = 100.0
MAG_P = 0.24


def stable_fe_g(T: float) -> float:
    return g_fe_bcc(T) if g_fe_bcc(T) <= g_fe_fcc(T) else g_fe_fcc(T)


def minimize_lambda_fe_spinel(
    T: float,
    *,
    steps: int = 80,
    rounds: int = 4,
    dg_affine_a: float = 0.0,
    dg_affine_b: float = 0.0,
    mag_tord: float = 848.0,
    mag_beta: float = 44.54,
    mag_p: float = 0.28,
) -> tuple[float, float, float]:
    """
    Compute the reduced Fe|spinel boundary oxygen potential for a modified
    spinel Gibbs model.
    """
    g_fe = stable_fe_g(T)
    base_mag = hillert_jarl_gmag(T, 848.0, 44.54, 0.28)
    new_mag = hillert_jarl_gmag(T, mag_tord, mag_beta, mag_p)
    dmag = new_mag - base_mag

    best = math.inf
    best_a = 0.0
    best_b = 0.5
    a_lo = 0.0
    a_hi = 1.0

    for _ in range(rounds):
        span_a = a_hi - a_lo
        for ia in range(steps + 1):
            a = a_lo + span_a * ia / steps
            bmax = 0.5 * (1.0 - a)
            if bmax <= 0.0:
                continue
            if abs(a - best_a) < 0.2:
                b_center = min(max(best_b, 0.0), bmax)
                b_lo = max(0.0, b_center - 0.15)
                b_hi = min(bmax, b_center + 0.15)
            else:
                b_lo = 0.0
                b_hi = bmax
            for ib in range(steps + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g + dg_affine_a + dg_affine_b * T + dmag - g_fe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)

    return best, best_a, best_b


def god_from_log10_ratio(log10_ratio: float) -> float:
    ratio = 10.0 ** log10_ratio
    return ratio / (1.0 + ratio)


def log10_ratio(mu_reduced: float, mu_oxidized: float, lam_o: float, T: float) -> float:
    return (lam_o - (mu_oxidized - mu_reduced)) / (R * T * math.log(10.0))


def rms(vals: list[float]) -> float:
    return math.sqrt(sum(v * v for v in vals) / len(vals)) if vals else math.nan


def main() -> int:
    ap = argparse.ArgumentParser(description="Sensitivity study for low-T Fe|spinel residuals.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Aligned gas source.")
    ap.add_argument("--temps-c", default="350,450,550", help="Comma-separated temperatures in Celsius.")
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_c = [float(tok.strip()) for tok in args.temps_c.split(",") if tok.strip()]
    if not temps_c:
        print("no temperatures provided", file=sys.stderr)
        return 2

    h2_spec = H2_SPECS["h2-fe-spinel"]
    co_spec = CO_SPECS["co-fe-spinel"]

    variants = [
        ("current", {}),
        ("affine_g_ae", {"dg_affine_a": AFFINE_DG_AE, "dg_affine_b": AFFINE_DG_AE_T}),
        ("magnetic_only", {"mag_tord": MAG_TORD, "mag_beta": MAG_BETA, "mag_p": MAG_P}),
    ]

    print("Fe|spinel low-T sensitivity study")
    print(f"gas source: {args.gas_source}")
    print(
        "candidate affine correction: "
        f"dG_AE(T) = {AFFINE_DG_AE:.3f} + ({AFFINE_DG_AE_T:.6f}) T  [J/mol spinel]"
    )
    print(
        "candidate magnetic-only retune: "
        f"Tord={MAG_TORD:.3f} K, beta={MAG_BETA:.3f}, p={MAG_P:.3f}"
    )
    print()

    for name, params in variants:
        h2_resid: list[float] = []
        co_resid: list[float] = []
        print(name)
        print(
            f"{'T[C]':>6} {'H2 BG':>10} {'H2 model':>10} {'dH2':>10} "
            f"{'CO BG':>10} {'CO model':>10} {'dCO':>10} "
            f"{'a':>9} {'b':>9}"
        )
        for tc in temps_c:
            tk = tc + 273.15
            lam_o, a_best, b_best = minimize_lambda_fe_spinel(tk, **params)
            mu_h = query_mu0(args.runner, args.gas_source, tk, ["hydrogen", "water"])
            mu_c = query_mu0(args.runner, args.gas_source, tk, ["carbonmonoxide", "carbondioxide"])
            h2_model = god_from_log10_ratio(log10_ratio(mu_h["hydrogen"], mu_h["water"], lam_o, tk))
            co_model = god_from_log10_ratio(log10_ratio(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_o, tk))
            h2_bg = fit_h2(h2_spec, tc)
            co_bg = fit_co(co_spec, tc)
            dh2 = h2_model - h2_bg
            dco = co_model - co_bg
            h2_resid.append(dh2)
            co_resid.append(dco)
            print(
                f"{tc:6.0f} {h2_bg:10.6f} {h2_model:10.6f} {dh2:10.6f} "
                f"{co_bg:10.6f} {co_model:10.6f} {dco:10.6f} "
                f"{a_best:9.6f} {b_best:9.6f}"
            )
        print(
            f"RMS residuals: H2={rms(h2_resid):.6f}, CO={rms(co_resid):.6f}, "
            f"combined={math.sqrt((sum(v*v for v in h2_resid)+sum(v*v for v in co_resid))/(len(h2_resid)+len(co_resid))):.6f}"
        )
        print()

    print("Interpretation")
    print("- If `affine_g_ae` materially reduces both H2 and CO residuals, a low-order Fe3O4 endmember correction is a plausible patch.")
    print("- If `magnetic_only` performs similarly, the low-T slope may be mainly magnetic in origin.")
    print("- If neither is clearly satisfactory, the remaining issue likely sits in the reduced Fe-only spinel model form itself.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
