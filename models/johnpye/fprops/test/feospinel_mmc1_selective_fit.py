#!/usr/bin/env python3
"""
Search reduced-side selective magnetic tweaks on top of the mmc1_guess spinel.

This stays much closer to the assessed mmc1 magnetic structure than the earlier
global Gibbs corrections. The idea is to modify only the magnetic
contributions that are disproportionately active on the reduced spinel side:

- EA endmember magnetic contribution
- 234 / 235 excess magnetic interactions

Those weights are much larger on the reduced Fe|spinel / wustite|spinel
branches than on the oxidized spinel|Fe2O3 branch, so this is the smallest
mmc1-style experiment that might improve the BG reduced-side branches without
spoiling the hematite-side boundary.
"""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass

from feo_hidayat_validation import R, hillert_jarl_gmag, spinel_delta_ae, spinel_g_ae, spinel_i_ae, spinel_v_e
from feoh_hydrogen_boundary import default_runner, query_mu0
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co, gas_ratio_logs as co_ratio_logs
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2, gas_ratio_logs as h2_ratio_logs
from feospinel_correction_fit import (
    THIS_DIR,
    fe_spinel_lambda,
    load_points,
    log10_po2_from_lambda,
    oxygen_mu0,
    spinel_hematite_lambda,
    wustite_spinel_lambda,
)


@dataclass(frozen=True)
class Candidate:
    s_ea: float
    s_red: float
    rms_co_fs: float
    rms_co_ws: float
    rms_h2_fs: float
    rms_h2_ws: float
    rms_sh: float
    objective: float


def selective_phase_g_factory(s_ea: float, s_red: float):
    def fn(T: float, a: float, b: float):
        c = (a + 5.0 - 4.0 * b) / 6.0
        v = (1.0 - a - 2.0 * b) / 6.0
        if not (0.0 <= a <= 1.0 and 0.0 <= b <= 1.0 and 0.0 <= c <= 1.0 and 0.0 <= v <= 1.0):
            return math.nan, math.nan, math.nan, math.nan

        g_ae = spinel_g_ae(T)
        i_ae = spinel_i_ae(T)
        v_e = spinel_v_e(T)
        d_ae = spinel_delta_ae()

        g_ea = g_ae
        g_ee = g_ae + i_ae
        g_aa = g_ae - i_ae + d_ae
        g_ev = 5.0 / 7.0 * g_ae + v_e
        g_av = 5.0 / 7.0 * g_ae + v_e - i_ae + d_ae

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

        w_ae = yt_fe2 * yo_fe3
        w_ea = yt_fe3 * yo_fe2
        w_125 = yt_fe2 * yt_fe3 * yo_va
        w_134 = yt_fe2 * yo_fe2 * yo_fe3
        w_145 = yt_fe2 * yo_fe3 * yo_va
        w_234 = yt_fe3 * yo_fe2 * yo_fe3
        w_235 = yt_fe3 * yo_fe2 * yo_va

        tord = (
            848.0 * w_ae
            + s_ea * 424.0 * w_ea
            + 141.33333 * w_125
            + 2544.0 * w_134
            + 848.0 * w_145
            + s_red * 2544.0 * w_234
            - s_red * 5088.0 * w_235
        )
        beta = (
            44.54 * w_ae
            + s_ea * 22.27 * w_ea
            + 7.4233333 * w_125
            + 133.62 * w_134
            + 44.54 * w_145
            + s_red * 133.62 * w_234
            - s_red * 267.24 * w_235
        )
        gmag = hillert_jarl_gmag(T, tord, beta, 0.28) if (tord > 1e-12 and beta > 1e-12) else 0.0
        g = gmix - T * sconf + gmag
        n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
        return g, n_fe, yo_fe3, yo_va

    return fn


def rms(vals):
    return math.sqrt(sum(v * v for v in vals) / len(vals))


def evaluate_candidate(runner, gas_source: str, s_ea: float, s_red: float) -> Candidate:
    phase_g_fn = selective_phase_g_factory(s_ea, s_red)

    co_fs_errs = []
    for tc in [350.0, 400.0, 450.0, 500.0, 550.0]:
        tk = tc + 273.15
        lam_o = fe_spinel_lambda(tk, phase_g_fn)
        mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_ratio, _ = co_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
        god_model = (10.0**log10_ratio) / (1.0 + 10.0**log10_ratio)
        co_fs_errs.append(god_model - fit_co(CO_SPECS["co-fe-spinel"], tc))

    h2_fs_errs = []
    for tc in [350.0, 450.0, 550.0]:
        tk = tc + 273.15
        lam_o = fe_spinel_lambda(tk, phase_g_fn)
        mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_ratio, _ = h2_ratio_logs(mu["hydrogen"], mu["water"], lam_o, tk)
        god_model = (10.0**log10_ratio) / (1.0 + 10.0**log10_ratio)
        h2_fs_errs.append(god_model - fit_h2(H2_SPECS["h2-fe-spinel"], tc))

    co_ws_errs = []
    h2_ws_errs = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tc in [600.0, 700.0, 800.0, 900.0]:
        tk = tc + 273.15
        x_hint, lam_o, a_hint, b_hint, _v, _resid = wustite_spinel_lambda(tk, phase_g_fn, x_hint, a_hint, b_hint)
        mu_co = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_ratio_co, _ = co_ratio_logs(mu_co["carbonmonoxide"], mu_co["carbondioxide"], lam_o, tk)
        god_model_co = (10.0**log10_ratio_co) / (1.0 + 10.0**log10_ratio_co)
        co_ws_errs.append(god_model_co - fit_co(CO_SPECS["co-wustite-spinel"], tc))

        mu_h2 = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_ratio_h2, _ = h2_ratio_logs(mu_h2["hydrogen"], mu_h2["water"], lam_o, tk)
        god_model_h2 = (10.0**log10_ratio_h2) / (1.0 + 10.0**log10_ratio_h2)
        h2_ws_errs.append(god_model_h2 - fit_h2(H2_SPECS["h2-wustite-spinel"], tc))

    fig11_sh = load_points(THIS_DIR / "hidayat-2015-fig11-spin-Fe2O3.dat")
    sh_errs = []
    for invtemp, y_ref in sorted(fig11_sh.points, key=lambda p: 1000.0 / p[0]):
        tk = 1000.0 / invtemp
        lam_o, _a, _b = spinel_hematite_lambda(tk, phase_g_fn)
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        y_model = log10_po2_from_lambda(lam_o, mu_o2, tk)
        sh_errs.append(y_model - y_ref)

    rms_co_fs = rms(co_fs_errs)
    rms_co_ws = rms(co_ws_errs)
    rms_h2_fs = rms(h2_fs_errs)
    rms_h2_ws = rms(h2_ws_errs)
    rms_sh = rms(sh_errs)
    objective = rms_co_fs + rms_co_ws + rms_sh + 0.5 * (rms_h2_fs + rms_h2_ws)
    return Candidate(s_ea, s_red, rms_co_fs, rms_co_ws, rms_h2_fs, rms_h2_ws, rms_sh, objective)


def main() -> int:
    ap = argparse.ArgumentParser(description="Screen reduced-side selective mmc1 magnetic tweaks.")
    ap.add_argument("--runner", default=str(default_runner()))
    ap.add_argument("--gas-source", default="helmholtz+ref0:")
    ap.add_argument("--sea-values", default="0.7,0.85,1.0,1.15,1.3,1.5")
    ap.add_argument("--sred-values", default="0.7,0.85,1.0,1.15,1.3,1.5")
    ap.add_argument("--top-n", type=int, default=10)
    args = ap.parse_args()

    sea_values = [float(x.strip()) for x in args.sea_values.split(",") if x.strip()]
    sred_values = [float(x.strip()) for x in args.sred_values.split(",") if x.strip()]
    runner = args.runner
    gas_source = args.gas_source

    rows: list[Candidate] = []
    print("Selective mmc1 reduced-side magnetic screen")
    print(f"gas source: {gas_source}")
    print(f"s_ea grid: {sea_values}")
    print(f"s_red grid: {sred_values}")
    print()
    for s_ea in sea_values:
        for s_red in sred_values:
            cand = evaluate_candidate(runner, gas_source, s_ea, s_red)
            rows.append(cand)
            print(
                f"s_ea={s_ea:5.2f} s_red={s_red:5.2f} "
                f"obj={cand.objective:8.5f} "
                f"co_fs={cand.rms_co_fs:7.5f} co_ws={cand.rms_co_ws:7.5f} "
                f"h2_fs={cand.rms_h2_fs:7.5f} h2_ws={cand.rms_h2_ws:7.5f} "
                f"sh={cand.rms_sh:7.5f}"
            )
    print()
    rows.sort(key=lambda r: r.objective)
    print("Best candidates")
    for i, row in enumerate(rows[: args.top_n], start=1):
        print(
            f"{i}. s_ea={row.s_ea:.3f} s_red={row.s_red:.3f} "
            f"obj={row.objective:.6f} "
            f"co_fs={row.rms_co_fs:.6f} co_ws={row.rms_co_ws:.6f} "
            f"h2_fs={row.rms_h2_fs:.6f} h2_ws={row.rms_h2_ws:.6f} "
            f"sh={row.rms_sh:.6f}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
