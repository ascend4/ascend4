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

On the magnetite side we now use the Fe-only slice of the Degterov spinel
CEF model. The relevant check is therefore the minimized grand-potential
residual of the spinel phase at the elemental potentials implied by the
wustite composition.
"""

from __future__ import annotations

import math

R = 8.31446261815324


def hillert_jarl_gmag(T: float, Tord: float, beta: float, p: float) -> float:
    tau = T / Tord
    A = 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0)
    if tau <= 1.0:
        poly = tau**3 / 6.0 + tau**9 / 135.0 + tau**15 / 600.0
        f = 1.0 - (((79.0 / (140.0 * p)) / tau) + (474.0 / 497.0) * (1.0 / p - 1.0) * poly) / A
    else:
        f = -(tau**-5 / 10.0 + tau**-15 / 315.0 + tau**-25 / 1500.0) / A
    return f * R * T * math.log(beta + 1.0)


def g_fe_bcc(T: float) -> float:
    return (
        10375.2
        + 114.5502 * T
        - 23.5143 * T * math.log(T)
        - 0.004398 * T * T
        + 77359.0 / T
        - 5.8927e-8 * T**3
        + hillert_jarl_gmag(T, 1043.0, 2.22, 0.40)
    )


def g_fe3o4(T: float) -> float:
    return -1140237.0 + 1015.067 * T - 174.832 * T * math.log(T) - 0.008149196 * T * T + 1445276.0 / T


def spinel_g_ae(T: float) -> float:
    return -1141701.0 + 1014.54 * T - 0.008149197 * T * T - 174.832 * T * math.log(T) + 1445276.0 / T


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
    return x * (1.0 - x) * (q00 + q10 * (1.0 - 2.0 * x))


def dgex_wustite_dx(x: float) -> float:
    q00 = -59412.8
    q10 = 42676.8
    return q00 * (1.0 - 2.0 * x) + q10 * (1.0 - 6.0 * x + 6.0 * x * x)


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


def residual_wustite_spinel_degterov(T: float, x: float) -> tuple[float, float, float, float]:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_o = 2.0 * (mu_b - mu_a)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b

    best = math.inf
    best_a = 0.0
    best_b = 0.5
    best_v = 0.0
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
            if best_a == best_a and abs(a - best_a) < 0.2:
                b_center = min(max(best_b, 0.0), bmax)
                b_lo = max(0.0, b_center - 0.15)
                b_hi = min(bmax, b_center + 0.15)
            else:
                b_lo = 0.0
                b_hi = bmax
            steps_b = 80
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, yo_va = spinel_phase_g(T, a, b)
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
    return best, best_a, best_b, best_v


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


def main() -> None:
    T_target = 561.0 + 273.15
    atpct_o_target = 51.4
    x_target = x_from_atpct_o(atpct_o_target)

    r_fe = residual_fe_wustite(T_target, x_target)
    r_sp_old = residual_wustite_magnetite(T_target, x_target)
    r_sp_new, a_sp, b_sp, v_sp = residual_wustite_spinel_degterov(T_target, x_target)

    x_fe_best, r_fe_best = minimize_abs_residual_at_T(T_target, residual_fe_wustite)
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
    print(f"Wustite | Magnetite residual  = {r_sp_old/1000.0:+.3f} kJ/mol   (stoich Fe3O4 surrogate)")
    print(f"Wustite | Spinel residual     = {r_sp_new/1000.0:+.3f} kJ/mol   (Degterov Fe-only spinel)")
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


if __name__ == "__main__":
    main()
