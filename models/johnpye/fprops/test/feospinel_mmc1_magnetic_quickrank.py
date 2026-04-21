#!/usr/bin/env python3
"""
Quick ranking of plausible `mmc1.dat` spinel magnetic combination rules.

This is a lighter-weight companion to `feospinel_mmc1_magnetic_compare.py`.
It does not try to reproduce every traced point. Instead it samples a small
set of representative checkpoints so we can iterate on candidate magnetic
rules quickly.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from feospinel_mmc1_magnetic_compare import (
    VARIANTS,
    load_points,
    THIS_DIR,
    evaluate_1459c_invariant,
    evaluate_lowT_fe_spinel_bg,
    oxygen_mu0,
    oxygen_potential_at_spinel_hematite_boundary_variant,
    oxygen_potential_at_wustite_spinel_boundary_variant,
    log10_po2_from_lambda,
    fig10_mass_ratio_fe2o3_vs_feo,
    spinel_phase_g_variant,
)
from feoh_hydrogen_boundary import default_runner, normalize_gas_source


def sample_midpoint(points: tuple[tuple[float, float], ...]) -> tuple[float, float]:
    return points[len(points) // 2]


def rank_variant(runner: Path, gas_source: str, variant_name: str) -> dict[str, float]:
    variant = next(v for v in VARIANTS if v.name == variant_name)

    fig11_ws = load_points(THIS_DIR / "hidayat-2015-fig11-wust-spin.dat")
    fig11_sh = load_points(THIS_DIR / "hidayat-2015-fig11-spin-Fe2O3.dat")
    fig10_sh = load_points(THIS_DIR / "hidayat-2015-fig10-spin-Fe2O3.dat")

    inv_ws, y_ws_ref = sample_midpoint(fig11_ws.points)
    tk_ws = 1000.0 / inv_ws
    _x_ws, lam_ws, _a_ws, _b_ws = oxygen_potential_at_wustite_spinel_boundary_variant(tk_ws, variant)
    mu_o2_ws = oxygen_mu0(str(runner), gas_source, tk_ws)
    d_ws = log10_po2_from_lambda(lam_ws, mu_o2_ws, tk_ws) - y_ws_ref

    inv_sh, y_sh_ref = sample_midpoint(fig11_sh.points)
    tk_sh = 1000.0 / inv_sh
    lam_sh, a_sh, b_sh = oxygen_potential_at_spinel_hematite_boundary_variant(tk_sh, variant)
    mu_o2_sh = oxygen_mu0(str(runner), gas_source, tk_sh)
    d_sh = log10_po2_from_lambda(lam_sh, mu_o2_sh, tk_sh) - y_sh_ref

    x10_ref, tc10 = sample_midpoint(fig10_sh.points)
    tk10 = tc10 + 273.15
    _lam10, a10, b10 = oxygen_potential_at_spinel_hematite_boundary_variant(tk10, variant)
    _g10, nfe10, _yo3_10, _v10 = spinel_phase_g_variant(tk10, a10, b10, variant)
    d10 = fig10_mass_ratio_fe2o3_vs_feo(nfe10) - x10_ref

    log10_1459, atpct_1459 = evaluate_1459c_invariant(runner, gas_source, variant)
    h2_bg_rms, co_bg_rms = evaluate_lowT_fe_spinel_bg(runner, gas_source, variant, [350.0, 450.0, 550.0])

    score = math.sqrt(
        d_ws * d_ws
        + d_sh * d_sh
        + (25.0 * d10) * (25.0 * d10)
        + (0.5 * log10_1459) * (0.5 * log10_1459)
        + (h2_bg_rms * h2_bg_rms)
        + (co_bg_rms * co_bg_rms)
    )

    return {
        "d_fig11_ws": d_ws,
        "d_fig11_sh": d_sh,
        "d_fig10": d10,
        "log10_1459": log10_1459,
        "atpct_1459": atpct_1459,
        "h2_bg_rms": h2_bg_rms,
        "co_bg_rms": co_bg_rms,
        "score": score,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Quick-rank plausible mmc1 spinel magnetic combination rules.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas/O2 mu0 source.")
    ap.add_argument(
        "--variants",
        default="current,mmc1_weighted_tc_beta,mmc1_weighted_tc_logbeta,mmc1_norm_tc_beta,mmc1_tc_beta_plus_excess,mmc1_tc_logbeta_plus_excess,mmc1_norm_tc_beta_plus_excess",
        help="Comma-separated variant names to evaluate.",
    )
    args = ap.parse_args()
    gas_source = normalize_gas_source(args.gas_source)
    names = [tok.strip() for tok in args.variants.split(",") if tok.strip()]

    print("Quick rank of spinel magnetic candidates")
    print(f"gas source: {gas_source}")
    print()
    print(
        f"{'variant':36s} {'fig11_ws':>10} {'fig11_sh':>10} {'fig10':>10} "
        f"{'1459log':>10} {'1459at%':>10} {'H2bg':>10} {'CObg':>10} {'score':>10}"
    )
    for name in names:
        row = rank_variant(args.runner, gas_source, name)
        print(
            f"{name:36s} "
            f"{row['d_fig11_ws']:10.5f} {row['d_fig11_sh']:10.5f} {row['d_fig10']:10.5f} "
            f"{row['log10_1459']:10.5f} {row['atpct_1459']:10.3f} "
            f"{row['h2_bg_rms']:10.5f} {row['co_bg_rms']:10.5f} {row['score']:10.5f}"
        )
    print()
    print("Lower score is better for this quick screen.")
    print("This is only a ranking heuristic; use the full compare script before trusting a candidate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
