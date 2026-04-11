#!/usr/bin/env python3
"""
Compare the current reduced Fe-only spinel magnetic treatment against a
variant inferred from the Hidayat supplementary `mmc1.dat` export.

The key difference is:

- current model: one composition-independent spinel magnetic term
- mmc1 variant: endmember-weighted magnetic contribution, with distinct
  magnetic data for AE (`Fe3O4`) and EA (`Fe3O4[1-]`)

This stays on the Python diagnostic side only. It is intended to answer
whether the remaining Hidayat Fig. 10 mismatch is plausibly magnetic in
origin before any production C changes are made.
"""

from __future__ import annotations

import argparse
import math
import os
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

from feo_hidayat_validation import (
    R,
    g_fe2o3,
    g_fe_bcc,
    g_fe_fcc,
    hillert_jarl_gmag,
    mu_a_wustite,
    mu_b_wustite,
    spinel_delta_ae,
    spinel_g_ae,
    spinel_i_ae,
    spinel_v_e,
)
from feoh_hydrogen_boundary import default_runner, normalize_gas_source, query_mu0
from feoh_baur_glaessner_compare import FIT_SPECS as H2_SPECS, fit_god_at_temp as fit_h2
from feoc_baur_glaessner_compare import FIT_SPECS as CO_SPECS, fit_god_at_temp as fit_co

THIS_DIR = Path(__file__).resolve().parent
P0 = 1e5
M_FE = 55.845
M_O = 15.999
M_FEO = M_FE + M_O
M_FE2O3 = 2.0 * M_FE + 3.0 * M_O


@dataclass(frozen=True)
class PointSet:
    name: str
    points: tuple[tuple[float, float], ...]


@dataclass(frozen=True)
class Variant:
    name: str
    note: str


VARIANTS = (
    Variant("current", "single composition-independent spinel magnetic term"),
    Variant("mmc1_endmember_mag", "AE and EA endmember-weighted magnetic terms from mmc1.dat"),
    Variant("mmc1_weighted_tc_beta", "composition-dependent Tord/beta from AE and EA endmembers in mmc1.dat"),
    Variant("mmc1_weighted_tc_logbeta", "composition-dependent Tord with ln(beta+1) mixed from AE and EA endmembers"),
    Variant("mmc1_norm_tc_beta", "normalized composition-dependent Tord/beta from AE and EA endmembers"),
    Variant("mmc1_tc_beta_plus_excess", "composition-dependent Tord/beta with explicit excess magnetic interactions from mmc1.dat"),
    Variant("mmc1_tc_logbeta_plus_excess", "composition-dependent Tord with ln(beta+1) mixed and explicit excess magnetic interactions"),
    Variant("mmc1_norm_tc_beta_plus_excess", "normalized composition-dependent Tord/beta with explicit excess magnetic interactions"),
)


def load_points(path: Path) -> PointSet:
    pts: list[tuple[float, float]] = []
    with path.open("r", encoding="ascii") as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            a, b = line.split()[:2]
            pts.append((float(a), float(b)))
    return PointSet(path.stem, tuple(pts))


def stable_fe_g(T: float) -> float:
    return g_fe_bcc(T) if g_fe_bcc(T) <= g_fe_fcc(T) else g_fe_fcc(T)


def spinel_phase_g_variant(T: float, a: float, b: float, variant: Variant) -> tuple[float, float, float, float]:
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

    def beta_from_log(logbeta: float) -> float:
        if logbeta <= 0.0:
            return 0.0
        return math.exp(logbeta) - 1.0

    def normalized_pair(raw_tord: float, raw_beta: float, denom: float) -> tuple[float, float]:
        if denom <= 1e-12:
            return 0.0, 0.0
        return raw_tord / denom, raw_beta / denom

    if variant.name == "current":
        gmag = hillert_jarl_gmag(T, 848.0, 44.54, 0.28)
    elif variant.name == "mmc1_endmember_mag":
        # From mmc1.dat:
        # AE  -> Fe3O4      : Tord=848 K, beta=44.54
        # EA  -> Fe3O4[1-]  : Tord=424 K, beta=22.27
        gmag_ae = hillert_jarl_gmag(T, 848.0, 44.54, 0.28)
        gmag_ea = hillert_jarl_gmag(T, 424.0, 22.27, 0.28)
        gmag = w_ae * gmag_ae + w_ea * gmag_ea
    elif variant.name == "mmc1_weighted_tc_beta":
        tord = 848.0 * w_ae + 424.0 * w_ea
        beta = 44.54 * w_ae + 22.27 * w_ea
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    elif variant.name == "mmc1_weighted_tc_logbeta":
        tord = 848.0 * w_ae + 424.0 * w_ea
        logbeta = w_ae * math.log(44.54 + 1.0) + w_ea * math.log(22.27 + 1.0)
        beta = beta_from_log(logbeta)
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    elif variant.name == "mmc1_norm_tc_beta":
        denom = w_ae + w_ea
        tord, beta = normalized_pair(848.0 * w_ae + 424.0 * w_ea, 44.54 * w_ae + 22.27 * w_ea, denom)
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    elif variant.name == "mmc1_tc_beta_plus_excess":
        # From mmc1.dat:
        # constituent magnetic entries:
        #   AE: Fe3O4      -> Tord=848 K,  beta=44.54
        #   EA: Fe3O4[1-]  -> Tord=424 K,  beta=22.27
        # excess magnetic interactions (NOEXPR=1 => RK power zero):
        #   (1,2,5): +141.33333,   +7.4233333
        #   (1,3,4): +2544.0,      +133.62
        #   (1,4,5): +848.0,       +44.54
        #   (2,3,4): +2544.0,      +133.62
        #   (2,3,5): -5088.0,      -267.24
        tord = (
            848.0 * w_ae
            + 424.0 * w_ea
            + 141.33333 * (yt_fe2 * yt_fe3 * yo_va)
            + 2544.0 * (yt_fe2 * yo_fe2 * yo_fe3)
            + 848.0 * (yt_fe2 * yo_fe3 * yo_va)
            + 2544.0 * (yt_fe3 * yo_fe2 * yo_fe3)
            - 5088.0 * (yt_fe3 * yo_fe2 * yo_va)
        )
        beta = (
            44.54 * w_ae
            + 22.27 * w_ea
            + 7.4233333 * (yt_fe2 * yt_fe3 * yo_va)
            + 133.62 * (yt_fe2 * yo_fe2 * yo_fe3)
            + 44.54 * (yt_fe2 * yo_fe3 * yo_va)
            + 133.62 * (yt_fe3 * yo_fe2 * yo_fe3)
            - 267.24 * (yt_fe3 * yo_fe2 * yo_va)
        )
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    elif variant.name == "mmc1_tc_logbeta_plus_excess":
        tord = (
            848.0 * w_ae
            + 424.0 * w_ea
            + 141.33333 * w_125
            + 2544.0 * w_134
            + 848.0 * w_145
            + 2544.0 * w_234
            - 5088.0 * w_235
        )
        logbeta = (
            w_ae * math.log(44.54 + 1.0)
            + w_ea * math.log(22.27 + 1.0)
            + w_125 * math.log(7.4233333 + 1.0)
            + w_134 * math.log(133.62 + 1.0)
            + w_145 * math.log(44.54 + 1.0)
            + w_234 * math.log(133.62 + 1.0)
            - w_235 * math.log(267.24 + 1.0)
        )
        beta = beta_from_log(logbeta)
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    elif variant.name == "mmc1_norm_tc_beta_plus_excess":
        raw_tord = (
            848.0 * w_ae
            + 424.0 * w_ea
            + 141.33333 * w_125
            + 2544.0 * w_134
            + 848.0 * w_145
            + 2544.0 * w_234
            - 5088.0 * w_235
        )
        raw_beta = (
            44.54 * w_ae
            + 22.27 * w_ea
            + 7.4233333 * w_125
            + 133.62 * w_134
            + 44.54 * w_145
            + 133.62 * w_234
            - 267.24 * w_235
        )
        denom = w_ae + w_ea + w_125 + w_134 + w_145 + w_234 + w_235
        tord, beta = normalized_pair(raw_tord, raw_beta, denom)
        if tord > 1e-12 and beta > 1e-12:
            gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
        else:
            gmag = 0.0
    else:
        raise KeyError(variant.name)

    g = gmix - T * sconf + gmag
    n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
    return g, n_fe, yo_fe3, yo_va


def minimize_spinel_grand_residual_variant(
    T: float,
    lam_fe: float,
    lam_o: float,
    variant: Variant,
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
                g, n_fe, _yo_fe3, yo_va = spinel_phase_g_variant(T, a, b, variant)
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


def oxygen_potential_at_fe_spinel_boundary_variant(T: float, variant: Variant) -> tuple[float, float, float]:
    g_fe = stable_fe_g(T)
    best = math.inf
    best_a = 0.0
    best_b = 0.5
    a_lo = 0.0
    a_hi = 1.0
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            for ib in range(steps_a + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_a
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g_variant(T, a, b, variant)
                if not math.isfinite(g):
                    continue
                trial = (g - g_fe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return best, best_a, best_b


def oxygen_potential_at_wustite_spinel_boundary_variant(
    T: float,
    variant: Variant,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    if x_hint is None or not math.isfinite(x_hint):
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
            mu_a = mu_a_wustite(T, x)
            mu_b = mu_b_wustite(T, x)
            lam_o = 2.0 * (mu_b - mu_a)
            lam_fe = 3.0 * mu_a - 2.0 * mu_b
            resid, a_cur, b_cur, _v = minimize_spinel_grand_residual_variant(
                T, lam_fe, lam_o, variant, a_hint=a_seed, b_hint=b_seed
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
    return best_x, best_lam_o, best_a, best_b


def oxygen_potential_at_spinel_hematite_boundary_variant(T: float, variant: Variant) -> tuple[float, float, float]:
    lam_lo = -350000.0
    lam_hi = -120000.0

    def signed_residual(lam_o: float, a_hint: float | None, b_hint: float | None) -> tuple[float, float, float]:
        lam_fe = (g_fe2o3(T) - 3.0 * lam_o) / 2.0
        resid, a_best, b_best, _v_best = minimize_spinel_grand_residual_variant(
            T, lam_fe, lam_o, variant, a_hint=a_hint, b_hint=b_hint
        )
        return resid, a_best, b_best

    r_lo, a_lo, b_lo = signed_residual(lam_lo, None, None)
    r_hi, a_hi, b_hi = signed_residual(lam_hi, None, None)
    if r_lo * r_hi > 0.0:
        raise RuntimeError(f"failed to bracket spinel|hematite boundary at T={T:.2f} K")

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


def log10_po2_from_lambda(lam_o: float, mu_o2: float, T: float) -> float:
    return (2.0 * lam_o - mu_o2) / (R * T * math.log(10.0))


@lru_cache(maxsize=None)
def oxygen_mu0(runner_str: str, gas_source: str, T: float) -> float:
    return query_mu0(Path(runner_str), gas_source, T, ["oxygen"])["oxygen"]


def fig10_mass_ratio_fe2o3_vs_feo(n_fe: float) -> float:
    alpha = max(0.0, 3.0 * n_fe - 8.0)
    beta = max(0.0, 4.0 - n_fe)
    denom = alpha * M_FEO + beta * M_FE2O3
    return beta * M_FE2O3 / denom


def rms(values: list[float]) -> float:
    return math.sqrt(sum(v * v for v in values) / len(values)) if values else math.nan


def evaluate_fig11_wustite_spinel(runner: Path, gas_source: str, variant: Variant, data: PointSet) -> tuple[float, float]:
    x_hint = None
    a_hint = None
    b_hint = None
    residuals: list[float] = []
    for invtemp, y_ref in sorted(data.points, key=lambda p: 1000.0 / p[0]):
        tk = 1000.0 / invtemp
        x_best, lam_o, a_hint, b_hint = oxygen_potential_at_wustite_spinel_boundary_variant(
            tk, variant, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        residuals.append(log10_po2_from_lambda(lam_o, mu_o2, tk) - y_ref)
        x_hint = x_best
    return rms(residuals), max(abs(v) for v in residuals)


def evaluate_fig11_spinel_hematite(runner: Path, gas_source: str, variant: Variant, data: PointSet) -> tuple[float, float]:
    residuals: list[float] = []
    for invtemp, y_ref in sorted(data.points, key=lambda p: 1000.0 / p[0]):
        tk = 1000.0 / invtemp
        lam_o, _a, _b = oxygen_potential_at_spinel_hematite_boundary_variant(tk, variant)
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        residuals.append(log10_po2_from_lambda(lam_o, mu_o2, tk) - y_ref)
    return rms(residuals), max(abs(v) for v in residuals)


def evaluate_fig10_spinel_hematite(variant: Variant, data: PointSet) -> tuple[float, float]:
    residuals: list[float] = []
    for x_ref, tc in sorted(data.points, key=lambda p: p[1]):
        tk = tc + 273.15
        _lam_o, a_best, b_best = oxygen_potential_at_spinel_hematite_boundary_variant(tk, variant)
        _g, n_fe, _yo3, _v = spinel_phase_g_variant(tk, a_best, b_best, variant)
        residuals.append(fig10_mass_ratio_fe2o3_vs_feo(n_fe) - x_ref)
    return rms(residuals), max(abs(v) for v in residuals)


def evaluate_1459c_invariant(runner: Path, gas_source: str, variant: Variant) -> tuple[float, float]:
    tk = 1459.0 + 273.15
    lam_o, a_best, b_best = oxygen_potential_at_spinel_hematite_boundary_variant(tk, variant)
    mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
    log10_po2 = log10_po2_from_lambda(lam_o, mu_o2, tk)
    _g, n_fe, _yo3, _v = spinel_phase_g_variant(tk, a_best, b_best, variant)
    atpct_o = 100.0 * 4.0 / (n_fe + 4.0)
    return log10_po2, atpct_o


def evaluate_lowT_fe_spinel_bg(runner: Path, gas_source: str, variant: Variant, temps_c: list[float]) -> tuple[float, float]:
    h2_resids: list[float] = []
    co_resids: list[float] = []
    h2_spec = H2_SPECS["h2-fe-spinel"]
    co_spec = CO_SPECS["co-fe-spinel"]
    for tc in temps_c:
        tk = tc + 273.15
        lam_o, _a, _b = oxygen_potential_at_fe_spinel_boundary_variant(tk, variant)
        mu_h = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        mu_c = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log_h2 = (lam_o - (mu_h["water"] - mu_h["hydrogen"])) / (R * tk * math.log(10.0))
        log_co = (lam_o - (mu_c["carbondioxide"] - mu_c["carbonmonoxide"])) / (R * tk * math.log(10.0))
        god_h2 = (10.0 ** log_h2) / (1.0 + 10.0 ** log_h2)
        god_co = (10.0 ** log_co) / (1.0 + 10.0 ** log_co)
        fit_h2_val = fit_h2(h2_spec, tc)
        fit_co_val = fit_co(co_spec, tc)
        if fit_h2_val is not None:
            h2_resids.append(god_h2 - fit_h2_val)
        if fit_co_val is not None:
            co_resids.append(god_co - fit_co_val)
    return rms(h2_resids), rms(co_resids)


def main() -> int:
    ap = argparse.ArgumentParser(description="mmc1-driven spinel magnetic diagnostic comparison.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas/O2 mu0 source.")
    ap.add_argument("--lowT-temps-c", default="350,450,550", help="Comma-separated low-T Fe|spinel temperatures.")
    ap.add_argument(
        "--variants",
        default="current,mmc1_weighted_tc_beta,mmc1_weighted_tc_logbeta,mmc1_norm_tc_beta,mmc1_tc_beta_plus_excess,mmc1_tc_logbeta_plus_excess,mmc1_norm_tc_beta_plus_excess",
        help="Comma-separated variant names to evaluate.",
    )
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)

    fig11_ws = load_points(THIS_DIR / "hidayat-2015-fig11-wust-spin.dat")
    fig11_sh = load_points(THIS_DIR / "hidayat-2015-fig11-spin-Fe2O3.dat")
    fig10_sh = load_points(THIS_DIR / "hidayat-2015-fig10-spin-Fe2O3.dat")
    lowT = [float(tok.strip()) for tok in args.lowT_temps_c.split(",") if tok.strip()]
    requested = {tok.strip() for tok in args.variants.split(",") if tok.strip()}
    variants = [v for v in VARIANTS if v.name in requested]
    missing = sorted(requested.difference(v.name for v in variants))
    if missing:
        raise SystemExit(f"unknown variants requested: {', '.join(missing)}")

    print("mmc1 spinel magnetic comparison")
    print(f"gas source: {args.gas_source}")
    print()

    for variant in variants:
        fig11_ws_rms, fig11_ws_max = evaluate_fig11_wustite_spinel(args.runner, args.gas_source, variant, fig11_ws)
        fig11_sh_rms, fig11_sh_max = evaluate_fig11_spinel_hematite(args.runner, args.gas_source, variant, fig11_sh)
        fig10_rms, fig10_max = evaluate_fig10_spinel_hematite(variant, fig10_sh)
        log10_1459, atpct_1459 = evaluate_1459c_invariant(args.runner, args.gas_source, variant)
        h2_bg_rms, co_bg_rms = evaluate_lowT_fe_spinel_bg(args.runner, args.gas_source, variant, lowT)

        print(variant.name)
        print(f"- note: {variant.note}")
        print(f"- Fig. 11 wustite|spinel RMS delta log10(pO2): {fig11_ws_rms:.6f} (max {fig11_ws_max:.6f})")
        print(f"- Fig. 11 spinel|Fe2O3 RMS delta log10(pO2): {fig11_sh_rms:.6f} (max {fig11_sh_max:.6f})")
        print(f"- Fig. 10 spinel|Fe2O3 RMS delta mass ratio: {fig10_rms:.6f} (max {fig10_max:.6f})")
        print(f"- Table 2 1459 C: log10(pO2)={log10_1459:+.6f}, spinel={atpct_1459:.3f} at% O")
        print(f"- low-T BG Fe|spinel RMS: H2={h2_bg_rms:.6f}, CO={co_bg_rms:.6f}")
        print()

    print("Interpretation")
    print("- If the mmc1 magnetic variant reduces the Fig. 10 residual materially while leaving Fig. 11 close, the remaining defect is plausibly magnetic in origin.")
    print("- If Fig. 10 barely moves, the next issue is more likely the reduced spinel model form itself, not just the magnetic simplification.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
