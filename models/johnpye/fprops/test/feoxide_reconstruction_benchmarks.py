#!/usr/bin/env python3
"""
Benchmark harness for the pure-Fe oxide-side reconstruction work.

This is not a new thermodynamic model. It is a compact script that
summarizes the current baseline targets we need any reconstructed
oxide-side source family to satisfy or improve upon.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from feo_hidayat_validation import (
    R,
    atpct_o_from_x,
    g_fe_fcc,
    residual_fe_bcc_solution,
    residual_fe_fcc_solution,
    residual_fe_wustite,
    residual_wustite_magnetite,
    residual_wustite_spinel_degterov,
    spinel_phase_g,
    x_from_atpct_o,
)
from feoh_hydrogen_boundary import default_runner, normalize_gas_source, query_mu0
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2_god, batch_model_god_at_temps as h2_batch_god
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co_god, batch_model_god_at_temps as co_batch_god
from feoxide_potential_boundaries import oxygen_potential_at_spinel_hematite_boundary


def benchmark_solid_state() -> None:
    t1_c = 561.0
    tk1 = t1_c + 273.15
    atpct1 = 51.4
    x1 = x_from_atpct_o(atpct1)
    r_fe = residual_fe_wustite(tk1, x1)
    r_bcc_sol, y_bcc = residual_fe_bcc_solution(tk1, x1)
    r_mag = residual_wustite_magnetite(tk1, x1)
    r_spin, a_spin, b_spin, v_spin = residual_wustite_spinel_degterov(tk1, x1)

    print("Solid-state target: Hidayat Table 2 eutectoid")
    print(f"- target: {t1_c:.1f} C, wustite {atpct1:.3f} at% O (x = {x1:.6f})")
    print(f"- Fe|wustite residual: {r_fe/1000.0:+.3f} kJ/mol")
    print(f"- BCC_A2|wustite residual: {r_bcc_sol/1000.0:+.3f} kJ/mol")
    print(f"- wustite|magnetite residual: {r_mag/1000.0:+.3f} kJ/mol")
    print(f"- wustite|spinel residual: {r_spin/1000.0:+.3f} kJ/mol")
    print(
        "- best reduced-spinel site state at target point: "
        f"y_t(Fe2+)={a_spin:.4f}, y_o(Fe2+)={b_spin:.4f}, y_o(Va)={v_spin:.4f}"
    )
    print()

    t2_c = 912.0
    tk2 = t2_c + 273.15
    atpct2 = 51.3
    x2 = x_from_atpct_o(atpct2)
    r_bcc = residual_fe_wustite(tk2, x2)
    mu_fcc = g_fe_fcc(tk2)
    # residual_fe_wustite already uses bcc; compare fcc via solution helper too
    r_bcc_sol2, y_bcc2 = residual_fe_bcc_solution(tk2, x2)
    r_fcc_sol2, y_fcc2 = residual_fe_fcc_solution(tk2, x2)
    print("Solid-state target: Hidayat Table 2 alpha/gamma/wustite invariant")
    print(f"- target: {t2_c:.1f} C, wustite {atpct2:.3f} at% O (x = {x2:.6f})")
    print(f"- Fe(bcc)|wustite residual: {r_bcc/1000.0:+.3f} kJ/mol")
    print(f"- BCC_A2|wustite residual: {r_bcc_sol2/1000.0:+.3f} kJ/mol, y_O={y_bcc2:.6e}")
    print(f"- FCC_A1|wustite residual: {r_fcc_sol2/1000.0:+.3f} kJ/mol, y_O={y_fcc2:.6e}")
    print(f"- Fe(fcc) Gibbs at target T: {mu_fcc/1000.0:+.3f} kJ/mol")
    print()


def benchmark_hematite_side_invariant(runner: Path, gas_source: str) -> None:
    t_c = 1459.0
    tk = t_c + 273.15
    target_log10_po2 = 0.0
    target_spinel_atpct_o = 58.0

    lam_o, a_best, b_best = oxygen_potential_at_spinel_hematite_boundary(tk)
    mu_o2 = query_mu0(runner, gas_source, tk, ["oxygen"])["oxygen"]
    log10_po2 = (2.0 * lam_o - mu_o2) / (R * tk * math.log(10.0))
    _g, n_fe, _yo_fe3, _yo_va = spinel_phase_g(tk, a_best, b_best)
    atpct_o = 100.0 * 4.0 / (n_fe + 4.0)

    print("Hidayat Table 2 hematite-side invariant")
    print("- reaction: Magnetite + Gas (1 atm) -> Fe2O3")
    print(f"- target: {t_c:.1f} C")
    print(f"- target log10(pO2 / atm): {target_log10_po2:.4f}")
    print(f"- model  log10(pO2 / atm): {log10_po2:.4f}")
    print(f"- delta log10(pO2): {log10_po2 - target_log10_po2:+.4f}")
    print(f"- target spinel composition: {target_spinel_atpct_o:.3f} at% O")
    print(f"- model  spinel composition: {atpct_o:.3f} at% O")
    print(f"- delta spinel composition: {atpct_o - target_spinel_atpct_o:+.3f} at% O")
    print(
        "- best reduced-spinel site state at target point: "
        f"y_t(Fe2+)={a_best:.4f}, y_o(Fe2+)={b_best:.4f}"
    )
    print()


def benchmark_gas_side(runner: Path, gas_source: str, t_c: float) -> None:
    co_fit_fw = fit_co_god(CO_SPECS["co-fe-wustite"], t_c)
    co_fit_ws = fit_co_god(CO_SPECS["co-wustite-spinel"], t_c)
    co_fit_fs = fit_co_god(CO_SPECS["co-fe-spinel"], t_c)
    co_model_fw = co_batch_god("fe-wustite", runner, gas_source, [t_c]) [t_c][0]
    co_model_ws = co_batch_god("wustite-spinel", runner, gas_source, [t_c]) [t_c][0]
    co_model_fs = co_batch_god("fe-spinel", runner, gas_source, [t_c]) [t_c][0]

    print(f"Gas-side convergence checkpoint near {t_c:.1f} C (CO/CO2)")
    print(f"- BG Fe|wustite GOD:      {co_fit_fw:.6f}" if co_fit_fw is not None else "- BG Fe|wustite GOD:      n/a")
    print(f"- BG wustite|spinel GOD: {co_fit_ws:.6f}" if co_fit_ws is not None else "- BG wustite|spinel GOD: n/a")
    print(f"- BG Fe|spinel GOD:      {co_fit_fs:.6f}" if co_fit_fs is not None else "- BG Fe|spinel GOD:      n/a")
    print(f"- model Fe|wustite GOD:      {co_model_fw:.6f}")
    print(f"- model wustite|spinel GOD: {co_model_ws:.6f}")
    print(f"- model Fe|spinel GOD:      {co_model_fs:.6f}")
    print()

    h2_fit_fw = fit_h2_god(H2_SPECS["h2-fe-wustite"], t_c)
    h2_fit_ws = fit_h2_god(H2_SPECS["h2-wustite-spinel"], t_c)
    h2_fit_fs = fit_h2_god(H2_SPECS["h2-fe-spinel"], t_c)
    h2_model_fw = h2_batch_god("fe-wustite", runner, gas_source, [t_c]) [t_c][0]
    h2_model_ws = h2_batch_god("wustite-spinel", runner, gas_source, [t_c]) [t_c][0]
    h2_model_fs = h2_batch_god("fe-spinel", runner, gas_source, [t_c]) [t_c][0]

    print(f"Gas-side convergence checkpoint near {t_c:.1f} C (H2/H2O)")
    print(f"- BG Fe|wustite GOD:      {h2_fit_fw:.6f}" if h2_fit_fw is not None else "- BG Fe|wustite GOD:      n/a")
    print(f"- BG wustite|spinel GOD: {h2_fit_ws:.6f}" if h2_fit_ws is not None else "- BG wustite|spinel GOD: n/a")
    print(f"- BG Fe|spinel GOD:      {h2_fit_fs:.6f}" if h2_fit_fs is not None else "- BG Fe|spinel GOD:      n/a")
    print(f"- model Fe|wustite GOD:      {h2_model_fw:.6f}")
    print(f"- model wustite|spinel GOD: {h2_model_ws:.6f}")
    print(f"- model Fe|spinel GOD:      {h2_model_fs:.6f}")
    print()


def main() -> int:
    ap = argparse.ArgumentParser(description="Benchmark targets for pure-Fe oxide-side reconstruction.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source for gas-side checks.")
    ap.add_argument("--convergence-temp-c", type=float, default=570.0, help="Temperature for branch-convergence checkpoint.")
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)

    benchmark_solid_state()
    benchmark_hematite_side_invariant(args.runner, args.gas_source)
    benchmark_gas_side(args.runner, args.gas_source, args.convergence_temp_c)

    print("Interpretation")
    print("- Use these numbers as the baseline to beat before replacing the current mixed hidayat/degterov oxide package.")
    print("- The main goal is to improve the common spinel-side placement without degrading the already-good Fe|wustite branch.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
