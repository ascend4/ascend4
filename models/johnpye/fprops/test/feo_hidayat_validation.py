#!/usr/bin/env python3
"""
Quantitative diagnostics for the current Fe-O Tier 2 model against
selected Hidayat (2015) targets.

This script is intentionally standalone. It mirrors the current
`hidayat_2015` / `degterov_2001` thermodynamic pieces implemented in
`gibbs_species.c`, `wustite_hidayat.c`, and `spinel_fe_degterov.c`.

The first quantitative target is the solid-state eutectoid from Hidayat
Table 2:

    Wustite -> Fe(bcc) + Magnetite    at 561 C

with the wustite composition reported as 51.4 at% O.

For the current Tier 2 model, coexistence at fixed T requires:

    g_Fe_bcc = 3 mu_A - 2 mu_B

where A = FeO and B = FeO1.5 are the wustite endmembers.

For the upgraded Fe-side model we also check the minimized grand-potential
residual of the Hidayat `BCC_A2` Fe-O solution phase.

On the magnetite side we now use the Fe-only slice of the Degterov spinel
CEF model. The relevant check is therefore the minimized grand-potential
residual of the spinel phase at the elemental potentials implied by the
wustite composition.
"""

from __future__ import annotations

import math

R = 8.31446261815324


def hillert_jarl_gmag(T: float, Tord: float, beta: float, p: float) -> float:
    tau = T / abs(Tord)
    A = 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0)
    if tau <= 1.0:
        poly = tau**3 / 6.0 + tau**9 / 135.0 + tau**15 / 600.0
        f = 1.0 - (((79.0 / (140.0 * p)) / tau) + (474.0 / 497.0) * (1.0 / p - 1.0) * poly) / A
    else:
        f = -(tau**-5 / 10.0 + tau**-15 / 315.0 + tau**-25 / 1500.0) / A
    return f * R * T * math.log(abs(beta) + 1.0)


def g_hser_fe(T: float) -> float:
    return (
        1225.7
        + 124.134 * T
        - 23.5143 * T * math.log(T)
        - 0.00439752 * T * T
        + 77359.0 / T
        - 5.8927e-8 * T**3
    )


def g_fe_bcc(T: float) -> float:
    if T <= 1811.0:
        g = g_hser_fe(T)
    else:
        g = -25383.581 + 299.31255 * T - 46.0 * T * math.log(T) + 2.29603e31 * T**-9
    return g + hillert_jarl_gmag(T, 1043.0, 2.22, 0.40)


def g_fe_fcc(T: float) -> float:
    if T <= 1811.0:
        g = g_hser_fe(T) - 1462.4 + 8.282 * T - 1.15 * T * math.log(T) + 6.4e-4 * T * T
    else:
        g = -27098.266 + 300.25256 * T - 46.0 * T * math.log(T) + 2.78854e31 * T**-9
    return g + hillert_jarl_gmag(T, -201.0, -2.1, 0.28)


def g_bcc_fe0(T: float) -> float:
    return g_fe_bcc(T)


def g_bcc_o0(T: float) -> float:
    return (
        120184.8
        + 139.1406 * T
        - 24.5000 * T * math.log(T)
        - 9.8420e-4 * T * T
        - 0.12938e-6 * T**3
        + 322517.0 / T
    )


def l_bcc_feo(T: float) -> float:
    return -315149.19 + 20.6935 * T


def g_fcc_fe0(T: float) -> float:
    return g_fe_fcc(T)


def g_fcc_o0(T: float) -> float:
    return g_bcc_o0(T)


def l_fcc_feo(T: float) -> float:
    return -315652.63336 + 27.6144 * T


def g_fe3o4(T: float) -> float:
    return -1140237.0 + 1015.067 * T - 174.832 * T * math.log(T) - 0.008149196 * T * T + 1445276.0 / T


def g_fe2o3(T: float) -> float:
    g = -859683.1 + 828.0501 * T - 137.0089 * T * math.log(T) + 1453820.0 / T
    if T > 2500.0:
        g = -857356.9 + 823.7122 * T - 136.5437 * T * math.log(T)
    return g + hillert_jarl_gmag(T, 955.667, 8.36667, 0.28)


def spinel_g_ae(T: float) -> float:
    # Hidayat 2015 Table 1 adjusted the Fe3O4 spinel endmember slightly
    # relative to the earlier Degterov 2001 optimization.
    return -1140237.0 + 1015.067 * T - 0.008149197 * T * T - 174.832 * T * math.log(T) + 1445276.0 / T


def spinel_i_ae(T: float) -> float:
    return -31229.0 + 22.063 * T


def spinel_v_e(T: float) -> float:
    return 29932.0 + 28.547 * T


def spinel_delta_ae() -> float:
    return 15781.0


def g0_feo(T: float) -> float:
    return (
        -285203.5
        + 274.2455 * T
        - 49.19444 * T * math.log(T)
        - 0.004678477 * T * T
        + 297568.8 / T
        + 574.4469 * math.log(T)
    )


def g0_feo1p5(T: float) -> float:
    return (
        -523138.0
        + 73.37019 * T
        - 26.96809 * T * math.log(T)
        - 0.008835071 * T * T
        + 1498519.0 / T
        + 25471.09 * math.log(T)
    )


def gex_wustite(x: float) -> float:
    q00 = -59412.8
    q10 = 42676.8
    xa = 1.0 - x
    return xa * x * (q00 + q10 * xa)


def dgex_wustite_dx(x: float) -> float:
    q00 = -59412.8
    q10 = 42676.8
    return q00 * (1.0 - 2.0 * x) + q10 * (1.0 - 4.0 * x + 3.0 * x * x)


def mu_a_wustite(T: float, x: float) -> float:
    gex = gex_wustite(x)
    dgdx = dgex_wustite_dx(x)
    return g0_feo(T) + R * T * math.log(1.0 - x) + gex - x * dgdx


def mu_b_wustite(T: float, x: float) -> float:
    gex = gex_wustite(x)
    dgdx = dgex_wustite_dx(x)
    return g0_feo1p5(T) + R * T * math.log(x) + gex + (1.0 - x) * dgdx


def x_from_atpct_o(atpct_o: float) -> float:
    y = atpct_o / 100.0
    return 2.0 * (1.0 - 2.0 * y) / (y - 1.0)


def atpct_o_from_x(x: float) -> float:
    return 100.0 * (1.0 + 0.5 * x) / (2.0 + 0.5 * x)


def residual_fe_wustite(T: float, x: float) -> float:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    return g_fe_bcc(T) - (3.0 * mu_a - 2.0 * mu_b)


def residual_wustite_magnetite(T: float, x: float) -> float:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    return g_fe3o4(T) - (mu_a + 2.0 * mu_b)


def bcc_solution_g(T: float, y_o: float) -> float:
    y_fe = 1.0 - y_o
    return (
        y_fe * g_bcc_fe0(T)
        + y_o * g_bcc_o0(T)
        + R * T * (y_fe * math.log(y_fe) + y_o * math.log(y_o))
        + y_fe * y_o * l_bcc_feo(T)
    )


def residual_fe_bcc_solution(T: float, x: float) -> tuple[float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    best = math.inf
    best_y = math.nan
    y_lo = 1e-8
    y_hi = 0.1
    for _ in range(4):
        steps = 80
        span = y_hi - y_lo
        for i in range(steps + 1):
            y_o = y_lo + span * i / steps
            y_fe = 1.0 - y_o
            resid = bcc_solution_g(T, y_o) - (y_fe * lam_fe + y_o * lam_o)
            if resid < best:
                best = resid
                best_y = y_o
        dy = max(1e-8, 0.15 * span)
        y_lo = max(1e-10, best_y - dy)
        y_hi = min(0.5, best_y + dy)
    return best, best_y


def fcc_solution_g(T: float, y_o: float) -> float:
    y_fe = 1.0 - y_o
    return (
        y_fe * g_fcc_fe0(T)
        + y_o * g_fcc_o0(T)
        + R * T * (y_fe * math.log(y_fe) + y_o * math.log(y_o))
        + y_fe * y_o * l_fcc_feo(T)
    )


def residual_fe_fcc_solution(T: float, x: float) -> tuple[float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    best = math.inf
    best_y = math.nan
    y_lo = 1e-8
    y_hi = 0.1
    for _ in range(4):
        steps = 80
        span = y_hi - y_lo
        for i in range(steps + 1):
            y_o = y_lo + span * i / steps
            y_fe = 1.0 - y_o
            resid = fcc_solution_g(T, y_o) - (y_fe * lam_fe + y_o * lam_o)
            if resid < best:
                best = resid
                best_y = y_o
        dy = max(1e-8, 0.15 * span)
        y_lo = max(1e-10, best_y - dy)
        y_hi = min(0.5, best_y + dy)
    return best, best_y


def spinel_phase_g(T: float, a: float, b: float) -> tuple[float, float, float, float]:
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
    gmag = hillert_jarl_gmag(T, 848.0, 44.54, 0.28)
    g = gmix - T * sconf + gmag
    n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
    n_o = 4.0
    return g, n_fe, yo_fe3, yo_va


def spinel_phase_g_mmc1_guess(T: float, a: float, b: float) -> tuple[float, float, float, float]:
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
        + 424.0 * w_ea
        + 141.33333 * w_125
        + 2544.0 * w_134
        + 848.0 * w_145
        + 2544.0 * w_234
        - 5088.0 * w_235
    )
    beta = (
        44.54 * w_ae
        + 22.27 * w_ea
        + 7.4233333 * w_125
        + 133.62 * w_134
        + 44.54 * w_145
        + 133.62 * w_234
        - 267.24 * w_235
    )
    if tord > 1e-12 and beta > 1e-12:
        gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
    else:
        gmag = 0.0
    g = gmix - T * sconf + gmag
    n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
    n_o = 4.0
    return g, n_fe, yo_fe3, yo_va


def low_t_curie_taper(tc: float, t0_c: float = 350.0, t1_c: float = 700.0) -> float:
    if tc <= t0_c:
        return 1.0
    if tc >= t1_c:
        return 0.0
    x = (tc - t0_c) / (t1_c - t0_c)
    return 0.5 * (1.0 + math.cos(math.pi * x))


def spinel_phase_g_mmc1_tapered_fit(T: float, a: float, b: float) -> tuple[float, float, float, float]:
    """
    Experimental low-temperature Gibbs correction on top of the mmc1-based
    magnetic reconstruction.

    This uses the branch-localized lambda-fit only as a target scale, but
    applies the correction coherently at the spinel phase Gibbs level and
    tapers it to zero above the Curie-region.
    """
    g, n_fe, yo_fe3, yo_va = spinel_phase_g_mmc1_guess(T, a, b)
    if not math.isfinite(g):
        return g, n_fe, yo_fe3, yo_va
    tc = T - 273.15
    dlam_kj_per_mol_o = 4.476095744721812 - 0.007137933743794555 * tc
    dg_j_per_mol_spinel = 4.0 * 1000.0 * dlam_kj_per_mol_o * low_t_curie_taper(tc)
    return g - dg_j_per_mol_spinel, n_fe, yo_fe3, yo_va


def spinel_phase_g_mmc1_selective_best(T: float, a: float, b: float) -> tuple[float, float, float, float]:
    """
    Best current mmc1-style selective magnetic rebalance from the reduced-side
    screening work.

    This keeps the non-magnetic manifold untouched and only reweights the
    magnetic contributions that were found to differentiate reduced spinel from
    the oxidized spinel|hematite branch:

    - EA endmember magnetic contribution scaled by 1.30
    - reduced excess terms (234 / 235) scaled by 0.95
    """
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

    s_ea = 1.30
    s_red = 0.95
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
    if tord > 1e-12 and beta > 1e-12:
        gmag = hillert_jarl_gmag(T, tord, beta, 0.28)
    else:
        gmag = 0.0
    g = gmix - T * sconf + gmag
    n_fe = 1.0 + 2.0 * (yo_fe2 + yo_fe3)
    return g, n_fe, yo_fe3, yo_va


def minimize_spinel_grand_residual_with_phase_g(
    T: float,
    lam_fe: float,
    lam_o: float,
    phase_g_fn,
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
        rounds = 4
        steps_a = 80
        steps_b = 80
        b_half = 0.15
    else:
        a_lo = max(0.0, a_hint - 0.12)
        a_hi = min(1.0, a_hint + 0.12)
        best_a = min(max(a_hint, 0.0), 1.0)
        best_b = max(b_hint, 0.0)
        rounds = 4
        steps_a = 28
        steps_b = 28
        b_half = 0.08

    for _ in range(rounds):
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
                g, n_fe, _yo_fe3, yo_va = phase_g_fn(T, a, b)
                if not math.isfinite(g):
                    continue
                resid = g - lam_fe * n_fe - lam_o * 4.0
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


def minimize_spinel_grand_residual_degterov(
    T: float,
    lam_fe: float,
    lam_o: float,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    return minimize_spinel_grand_residual_with_phase_g(
        T, lam_fe, lam_o, spinel_phase_g, a_hint=a_hint, b_hint=b_hint
    )


def residual_wustite_spinel_degterov(
    T: float,
    x: float,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    return minimize_spinel_grand_residual_degterov(T, lam_fe, lam_o, a_hint=a_hint, b_hint=b_hint)


def residual_wustite_spinel_mmc1_guess(
    T: float,
    x: float,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    return minimize_spinel_grand_residual_with_phase_g(
        T, lam_fe, lam_o, spinel_phase_g_mmc1_guess, a_hint=a_hint, b_hint=b_hint
    )


def residual_wustite_spinel_mmc1_tapered_fit(
    T: float,
    x: float,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    return minimize_spinel_grand_residual_with_phase_g(
        T, lam_fe, lam_o, spinel_phase_g_mmc1_tapered_fit, a_hint=a_hint, b_hint=b_hint
    )


def residual_wustite_spinel_mmc1_selective_best(
    T: float,
    x: float,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    return minimize_spinel_grand_residual_with_phase_g(
        T, lam_fe, lam_o, spinel_phase_g_mmc1_selective_best, a_hint=a_hint, b_hint=b_hint
    )


def minimize_abs_residual_at_T(T: float, fn, xmin: float = 1e-4, xmax: float = 0.95) -> tuple[float, float]:
    best_x = xmin
    best_val = abs(fn(T, xmin))
    x_lo = xmin
    x_hi = xmax
    for _ in range(4):
        steps = 80
        span = x_hi - x_lo
        for i in range(steps + 1):
            x = x_lo + span * i / steps
            val = abs(fn(T, x))
            if val < best_val:
                best_x = x
                best_val = val
        dx = max(5e-4, 0.15 * span)
        x_lo = max(xmin, best_x - dx)
        x_hi = min(xmax, best_x + dx)
    return best_x, best_val


def find_transition_temperature(fn, tmin: float, tmax: float) -> float:
    fmin = fn(tmin)
    fmax = fn(tmax)
    if fmin == 0.0:
        return tmin
    if fmax == 0.0:
        return tmax
    if fmin * fmax > 0.0:
        raise ValueError("transition is not bracketed")
    for _ in range(120):
        tmid = 0.5 * (tmin + tmax)
        fmid = fn(tmid)
        if fmid == 0.0:
            return tmid
        if fmin * fmid <= 0.0:
            tmax = tmid
            fmax = fmid
        else:
            tmin = tmid
            fmin = fmid
    return 0.5 * (tmin + tmax)


def main() -> None:
    T_target = 561.0 + 273.15
    atpct_o_target = 51.4
    x_target = x_from_atpct_o(atpct_o_target)

    r_fe = residual_fe_wustite(T_target, x_target)
    r_fe_bcc, y_o_bcc = residual_fe_bcc_solution(T_target, x_target)
    r_sp_old = residual_wustite_magnetite(T_target, x_target)
    r_sp_new, a_sp, b_sp, v_sp = residual_wustite_spinel_degterov(T_target, x_target)

    x_fe_best, r_fe_best = minimize_abs_residual_at_T(T_target, residual_fe_wustite)
    def bcc_abs_residual(T: float, x: float) -> float:
        resid, _y = residual_fe_bcc_solution(T, x)
        return abs(resid)

    x_fe_bcc_best, r_fe_bcc_best = minimize_abs_residual_at_T(T_target, bcc_abs_residual)
    _r_fe_bcc_signed, y_o_bcc_best = residual_fe_bcc_solution(T_target, x_fe_bcc_best)
    x_sp_old_best, r_sp_old_best = minimize_abs_residual_at_T(T_target, residual_wustite_magnetite)

    def spinel_abs_residual(T: float, x: float) -> float:
        resid, _a, _b, _v = residual_wustite_spinel_degterov(T, x)
        return abs(resid)

    x_sp_new_best, r_sp_new_best = minimize_abs_residual_at_T(T_target, spinel_abs_residual)
    _r_sp_new_best_signed, a_sp_best, b_sp_best, v_sp_best = residual_wustite_spinel_degterov(
        T_target, x_sp_new_best
    )

    print("Hidayat Table 2 solid-state validation")
    print(f"target: Wustite -> Fe(bcc) + Magnetite at T = {T_target:.2f} K (561 C)")
    print(f"target wustite composition: {atpct_o_target:.3f} at% O -> x = {x_target:.6f}")
    print()
    print("Residuals at the Hidayat target point")
    print(f"Fe(bcc) | Wustite residual     = {r_fe/1000.0:+.3f} kJ/mol")
    print(f"BCC_A2 | Wustite residual      = {r_fe_bcc/1000.0:+.3f} kJ/mol   (Hidayat Fe-O solution)")
    print(f"Wustite | Magnetite residual  = {r_sp_old/1000.0:+.3f} kJ/mol   (stoich Fe3O4 surrogate)")
    print(f"Wustite | Spinel residual     = {r_sp_new/1000.0:+.3f} kJ/mol   (Degterov Fe-only spinel)")
    print(f"best BCC_A2 oxygen fraction at target point: y_O = {y_o_bcc:.6e}")
    print(
        "best spinel site state at target point: "
        f"y_t(Fe2+) = {a_sp:.4f}, y_o(Fe2+) = {b_sp:.4f}, y_o(Va) = {v_sp:.4f}"
    )
    print()
    print("Best single-boundary fits at the same temperature")
    print(
        "Fe(bcc) | Wustite: "
        f"x = {x_fe_best:.6f}, at% O = {atpct_o_from_x(x_fe_best):.3f}, "
        f"|residual| = {r_fe_best/1000.0:.3f} kJ/mol"
    )
    print(
        "BCC_A2 | Wustite: "
        f"x = {x_fe_bcc_best:.6f}, at% O = {atpct_o_from_x(x_fe_bcc_best):.3f}, "
        f"|residual| = {r_fe_bcc_best/1000.0:.3f} kJ/mol, "
        f"y_O = {y_o_bcc_best:.6e}"
    )
    print(
        "Wustite | Magnetite (stoich): "
        f"x = {x_sp_old_best:.6f}, at% O = {atpct_o_from_x(x_sp_old_best):.3f}, "
        f"|residual| = {r_sp_old_best/1000.0:.3f} kJ/mol"
    )
    print(
        "Wustite | Spinel (Degterov): "
        f"x = {x_sp_new_best:.6f}, at% O = {atpct_o_from_x(x_sp_new_best):.3f}, "
        f"|residual| = {r_sp_new_best/1000.0:.3f} kJ/mol"
    )
    print(
        "best spinel site state at best Degterov fit: "
        f"y_t(Fe2+) = {a_sp_best:.4f}, y_o(Fe2+) = {b_sp_best:.4f}, y_o(Va) = {v_sp_best:.4f}"
    )

    print()
    print("Hidayat Table 2 bcc/fcc-wustite invariant")
    T_alpha_gamma = 912.0 + 273.15
    atpct_o_alpha_gamma = 51.3
    x_alpha_gamma = x_from_atpct_o(atpct_o_alpha_gamma)
    r_bcc_ag = residual_fe_wustite(T_alpha_gamma, x_alpha_gamma)
    mu_a_ag = mu_a_wustite(T_alpha_gamma, x_alpha_gamma)
    mu_b_ag = mu_b_wustite(T_alpha_gamma, x_alpha_gamma)
    r_fcc_ag = g_fe_fcc(T_alpha_gamma) - (3.0 * mu_a_ag - 2.0 * mu_b_ag)
    r_bcc_ag_sol, y_bcc_ag = residual_fe_bcc_solution(T_alpha_gamma, x_alpha_gamma)
    r_fcc_ag_sol, y_fcc_ag = residual_fe_fcc_solution(T_alpha_gamma, x_alpha_gamma)
    print(
        f"target: Fe(fcc) + wustite -> Fe(bcc) at T = {T_alpha_gamma:.2f} K (912 C), "
        f"wustite = {atpct_o_alpha_gamma:.3f} at% O -> x = {x_alpha_gamma:.6f}"
    )
    print("Residuals at the Hidayat target point")
    print(f"Fe(bcc) | Wustite residual     = {r_bcc_ag/1000.0:+.3f} kJ/mol")
    print(f"Fe(fcc) | Wustite residual     = {r_fcc_ag/1000.0:+.3f} kJ/mol")
    print(f"BCC_A2 | Wustite residual      = {r_bcc_ag_sol/1000.0:+.3f} kJ/mol   (Hidayat Fe-O solution)")
    print(f"FCC_A1 | Wustite residual      = {r_fcc_ag_sol/1000.0:+.3f} kJ/mol   (Hidayat Fe-O solution)")
    print(f"Fe(fcc)-Fe(bcc) Gibbs offset   = {(g_fe_fcc(T_alpha_gamma)-g_fe_bcc(T_alpha_gamma))/1000.0:+.3f} kJ/mol")
    print(f"best BCC_A2 oxygen fraction    = {y_bcc_ag:.6e}")
    print(f"best FCC_A1 oxygen fraction    = {y_fcc_ag:.6e}")

    x_bcc_ag_best, r_bcc_ag_best = minimize_abs_residual_at_T(T_alpha_gamma, residual_fe_wustite)

    def residual_fe_fcc_wustite(T: float, x: float) -> float:
        mu_a = mu_a_wustite(T, x)
        mu_b = mu_b_wustite(T, x)
        return g_fe_fcc(T) - (3.0 * mu_a - 2.0 * mu_b)

    x_fcc_ag_best, r_fcc_ag_best = minimize_abs_residual_at_T(T_alpha_gamma, residual_fe_fcc_wustite)

    def fcc_abs_residual(T: float, x: float) -> float:
        resid, _y = residual_fe_fcc_solution(T, x)
        return abs(resid)

    x_fcc_ag_sol_best, r_fcc_ag_sol_best = minimize_abs_residual_at_T(T_alpha_gamma, fcc_abs_residual)
    _r_fcc_ag_sol_signed, y_fcc_ag_best = residual_fe_fcc_solution(T_alpha_gamma, x_fcc_ag_sol_best)

    def coupled_fe_transition_residual(T: float, x: float) -> float:
        return max(abs(residual_fe_wustite(T, x)), abs(residual_fe_fcc_wustite(T, x)))

    x_coupled_best, r_coupled_best = minimize_abs_residual_at_T(T_alpha_gamma, coupled_fe_transition_residual)

    print("Best single-boundary fits at the same temperature")
    print(
        "Fe(bcc) | Wustite: "
        f"x = {x_bcc_ag_best:.6f}, at% O = {atpct_o_from_x(x_bcc_ag_best):.3f}, "
        f"|residual| = {r_bcc_ag_best/1000.0:.3f} kJ/mol"
    )
    print(
        "Fe(fcc) | Wustite: "
        f"x = {x_fcc_ag_best:.6f}, at% O = {atpct_o_from_x(x_fcc_ag_best):.3f}, "
        f"|residual| = {r_fcc_ag_best/1000.0:.3f} kJ/mol"
    )
    print(
        "FCC_A1 | Wustite: "
        f"x = {x_fcc_ag_sol_best:.6f}, at% O = {atpct_o_from_x(x_fcc_ag_sol_best):.3f}, "
        f"|residual| = {r_fcc_ag_sol_best/1000.0:.3f} kJ/mol, "
        f"y_O = {y_fcc_ag_best:.6e}"
    )
    print(
        "Coupled Fe(bcc)/Fe(fcc)/Wustite target: "
        f"x = {x_coupled_best:.6f}, at% O = {atpct_o_from_x(x_coupled_best):.3f}, "
        f"max|residual| = {r_coupled_best/1000.0:.3f} kJ/mol"
    )
    print()

    def pure_fe_offset(T: float) -> float:
        return g_fe_fcc(T) - g_fe_bcc(T)

    t_alpha_gamma_pure = find_transition_temperature(pure_fe_offset, 700.0, 1300.0)
    print("Pure Fe allotropic crossover in the current implementation")
    print(
        f"Fe(fcc)-Fe(bcc) = 0 at T = {t_alpha_gamma_pure:.2f} K "
        f"({t_alpha_gamma_pure - 273.15:.2f} C)"
    )
    print(
        f"Fe(fcc)-Fe(bcc) at 1185.15 K (912 C) = "
        f"{pure_fe_offset(T_alpha_gamma)/1000.0:+.3f} kJ/mol"
    )


if __name__ == "__main__":
    main()
