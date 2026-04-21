#!/usr/bin/env python3
"""
Sensitivity study for the oxidized wustite|spinel branch.

This probes which reduced Fe-only spinel terms are most likely responsible
for the remaining BG mismatch on the upper branch, using the aligned
`helmholtz+ref0:` gas basis for both H2/H2O and CO/CO2.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from feo_hidayat_validation import (
    R,
    hillert_jarl_gmag,
    mu_a_wustite,
    mu_b_wustite,
    spinel_delta_ae,
    spinel_g_ae,
    spinel_i_ae,
    spinel_v_e,
)
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as h2_fit_god, log10_ratio_from_god as h2_log10
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as co_fit_god, log10_ratio_from_god as co_log10
from feoh_hydrogen_boundary import default_runner, gas_ratio_logs as h2_ratio_logs, normalize_gas_source, query_mu0
from feoc_baur_glaessner_compare import gas_ratio_logs as co_ratio_logs


def spinel_phase_g_modified(
    T: float,
    a: float,
    b: float,
    dg_ae_a: float = 0.0,
    dg_ae_b: float = 0.0,
    dv_e: float = 0.0,
    ddelta_eav: float = 0.0,
    magnetic_scale: float = 1.0,
) -> tuple[float, float, float, float]:
    c = (a + 5.0 - 4.0 * b) / 6.0
    v = (1.0 - a - 2.0 * b) / 6.0
    if not (0.0 <= a <= 1.0 and 0.0 <= b <= 1.0 and 0.0 <= c <= 1.0 and 0.0 <= v <= 1.0):
        return math.nan, math.nan, math.nan, math.nan

    g_ae = spinel_g_ae(T) + dg_ae_a + dg_ae_b * T
    i_ae = spinel_i_ae(T)
    v_e = spinel_v_e(T) + dv_e
    d_ae = spinel_delta_ae()

    g_ea = g_ae
    g_ee = g_ae + i_ae
    g_aa = g_ae - i_ae + d_ae
    g_ev = 5.0 / 7.0 * g_ae + v_e
    g_av = 5.0 / 7.0 * g_ae + v_e - i_ae + d_ae - ddelta_eav

    yt_fe2 = a
    yt_fe3 = 1.0 - a
    yo_fe2 = b
    yo_fe3 = c
    yo_va = v

    def ylogy(y: float) -> float:
        return 0.0 if y <= 0.0 else y * math.log(y)

    gmix = (
        yt_fe2 * yo_fe2 * g_aa
        + yt_fe2 * yo_fe3 * g_ae
        + yt_fe2 * yo_va * g_av
        + yt_fe3 * yo_fe2 * g_ea
        + yt_fe3 * yo_fe3 * g_ee
        + yt_fe3 * yo_va * g_ev
    )
    sconf = -R * (
        ylogy(yt_fe2)
        + ylogy(yt_fe3)
        + 2.0 * (ylogy(yo_fe2) + ylogy(yo_fe3) + ylogy(yo_va))
    )
    gmag = magnetic_scale * hillert_jarl_gmag(T, 848.0, 44.54, 0.28)
    g = gmix - T * sconf + gmag
    n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
    return g, n_fe, yo_fe3, yo_va


def minimize_modified_spinel_residual(
    T: float,
    lam_fe: float,
    lam_o: float,
    *,
    dg_ae_a: float = 0.0,
    dg_ae_b: float = 0.0,
    dv_e: float = 0.0,
    ddelta_eav: float = 0.0,
    magnetic_scale: float = 1.0,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    best = math.inf
    best_a = 0.0
    best_b = 0.5
    best_v = 0.0

    if a_hint is None or b_hint is None or not math.isfinite(a_hint) or not math.isfinite(b_hint):
        a_lo = 0.0
        a_hi = 1.0
        steps_a = 80
        steps_b = 80
        b_half = 0.15
    else:
        a_lo = max(0.0, a_hint - 0.12)
        a_hi = min(1.0, a_hint + 0.12)
        best_a = a_hint
        best_b = max(b_hint, 0.0)
        steps_a = 28
        steps_b = 28
        b_half = 0.08

    for _ in range(4):
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
            bmax = 0.5 * (1.0 - a)
            if bmax <= 0.0:
                continue
            if math.isfinite(best_a) and abs(a - best_a) < max(0.06, 0.35 * span_a):
                b_center = min(max(best_b, 0.0), bmax)
                b_lo = max(0.0, b_center - b_half)
                b_hi = min(bmax, b_center + b_half)
            else:
                b_lo = 0.0
                b_hi = bmax
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, yo_va = spinel_phase_g_modified(
                    T, a, b,
                    dg_ae_a=dg_ae_a,
                    dg_ae_b=dg_ae_b,
                    dv_e=dv_e,
                    ddelta_eav=ddelta_eav,
                    magnetic_scale=magnetic_scale,
                )
                if not math.isfinite(g):
                    continue
                resid = g - lam_fe * n_fe - 4.0 * lam_o
                if resid < best:
                    best = resid
                    best_a = a
                    best_b = b
                    best_v = yo_va
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
        b_half = max(0.01, 0.45 * b_half)
    return best, best_a, best_b, best_v


def trace_modified_wustite_spinel(
    temps_c: list[float],
    *,
    dg_ae_a: float = 0.0,
    dg_ae_b: float = 0.0,
    dv_e: float = 0.0,
    ddelta_eav: float = 0.0,
    magnetic_scale: float = 1.0,
) -> list[tuple[float, float, float, float]]:
    x_hint = None
    a_hint = None
    b_hint = None
    out: list[tuple[float, float, float, float]] = []
    for tc in temps_c:
        tk = tc + 273.15
        if x_hint is None:
            x_lo = 1e-4
            x_hi = 0.95
            steps_x = 80
        else:
            x_lo = max(1e-4, x_hint - 0.04)
            x_hi = min(0.95, x_hint + 0.04)
            steps_x = 28

        best_abs = math.inf
        best_x = x_lo
        best_a = a_hint if a_hint is not None else math.nan
        best_b = b_hint if b_hint is not None else math.nan
        best_lam_o = math.nan
        for _ in range(4):
            span_x = x_hi - x_lo
            a_seed = best_a
            b_seed = best_b
            for ix in range(steps_x + 1):
                x = x_lo + span_x * ix / steps_x
                mu_a = mu_a_wustite(tk, x)
                mu_b = mu_b_wustite(tk, x)
                lam_o = 2.0 * (mu_b - mu_a)
                lam_fe = 3.0 * mu_a - 2.0 * mu_b
                resid, a_cur, b_cur, _v = minimize_modified_spinel_residual(
                    tk, lam_fe, lam_o,
                    dg_ae_a=dg_ae_a,
                    dg_ae_b=dg_ae_b,
                    dv_e=dv_e,
                    ddelta_eav=ddelta_eav,
                    magnetic_scale=magnetic_scale,
                    a_hint=a_seed,
                    b_hint=b_seed,
                )
                a_seed = a_cur
                b_seed = b_cur
                if abs(resid) < best_abs:
                    best_abs = abs(resid)
                    best_x = x
                    best_a = a_cur
                    best_b = b_cur
                    best_lam_o = lam_o
            dx = max(5e-4, 0.15 * span_x)
            x_lo = max(1e-4, best_x - dx)
            x_hi = min(0.95, best_x + dx)
        out.append((best_x, best_lam_o, best_a, best_b))
        x_hint = best_x
        a_hint = best_a
        b_hint = best_b
    return out


def evaluate_variant(
    runner: Path,
    gas_source: str,
    temps_c: list[float],
    *,
    dg_ae_a: float = 0.0,
    dg_ae_b: float = 0.0,
    dv_e: float = 0.0,
    ddelta_eav: float = 0.0,
    magnetic_scale: float = 1.0,
) -> tuple[float, float]:
    trace = trace_modified_wustite_spinel(
        temps_c,
        dg_ae_a=dg_ae_a,
        dg_ae_b=dg_ae_b,
        dv_e=dv_e,
        ddelta_eav=ddelta_eav,
        magnetic_scale=magnetic_scale,
    )
    h2_spec = H2_SPECS["h2-wustite-spinel"]
    co_spec = CO_SPECS["co-wustite-spinel"]

    sum_sq_h2 = 0.0
    sum_sq_co = 0.0
    nh2 = 0
    nco = 0
    for tc, (_x_best, lam_o, _a, _b) in zip(temps_c, trace):
        tk = tc + 273.15
        fit_h2 = h2_fit_god(h2_spec, tc)
        fit_co = co_fit_god(co_spec, tc)
        mu_h = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_h2_model, _ = h2_ratio_logs(mu_h["hydrogen"], mu_h["water"], lam_o, tk)
        mu_c = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_co_model, _ = co_ratio_logs(mu_c["carbonmonoxide"], mu_c["carbondioxide"], lam_o, tk)
        if fit_h2 is not None:
            dlog_h2 = log10_h2_model - h2_log10(fit_h2)
            sum_sq_h2 += dlog_h2 * dlog_h2
            nh2 += 1
        if fit_co is not None:
            dlog_co = log10_co_model - co_log10(fit_co)
            sum_sq_co += dlog_co * dlog_co
            nco += 1
    rms_h2 = math.sqrt(sum_sq_h2 / nh2) if nh2 else math.nan
    rms_co = math.sqrt(sum_sq_co / nco) if nco else math.nan
    return rms_h2, rms_co


def main() -> int:
    ap = argparse.ArgumentParser(description="Sensitivity study for the wustite|spinel BG mismatch.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source for all gases.")
    ap.add_argument("--temps-c", default="650,750,850,950", help="Comma-separated temperatures in Celsius.")
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)
    temps_c = [float(tok) for tok in args.temps_c.split(",") if tok.strip()]

    variants = [
        ("current", dict()),
        ("fe-spinel affine", dict(dg_ae_a=-16984.331545915302, dg_ae_b=18.769347422265838)),
        ("delta_EAV +10000", dict(ddelta_eav=10000.0)),
        ("delta_EAV +20000", dict(ddelta_eav=20000.0)),
        ("V_E -10000", dict(dv_e=-10000.0)),
        ("V_E -20000", dict(dv_e=-20000.0)),
        ("magnetic x0.9", dict(magnetic_scale=0.9)),
    ]

    print("wustite|spinel sensitivity study")
    print(f"Gas source: {args.gas_source}")
    print(f"Temperatures [C]: {', '.join(f'{t:.0f}' for t in temps_c)}")
    print()
    print(f"{'variant':<18} {'RMS H2':>10} {'RMS CO':>10}")
    for label, params in variants:
        rms_h2, rms_co = evaluate_variant(args.runner, args.gas_source, temps_c, **params)
        print(f"{label:<18} {rms_h2:10.5f} {rms_co:10.5f}")
    print()
    print("Interpretation")
    print("- Strong improvement from `delta_EAV` or `V_E` points to vacancy-side spinel terms.")
    print("- Strong improvement from the affine `G_AE` shift would suggest the oxidized branch shares the same endmember issue as Fe|spinel.")
    print("- Weak sensitivity across all cases suggests the reduced Fe-only spinel form itself is the deeper limitation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
