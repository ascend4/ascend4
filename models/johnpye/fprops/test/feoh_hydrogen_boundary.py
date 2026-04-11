#!/usr/bin/env python3
"""
First-pass Tier 3 Fe-O-H diagnostic.

This script combines:
  - the current audited Hidayat Fe-O condensed thermodynamics, and
  - chemistry-ready gas mu0(T, 1 bar) data for H2 and H2O

to generate first Baur-Glaessner-style reduction-boundary tables for:

    Fe | wustite | H2/H2O
    wustite | spinel | H2/H2O   (provisional)

The output is intended as a practical Tier 3 starting point, not as final
literature-grade validation.

How to run:
    python3 models/johnpye/fprops/test/feoh_hydrogen_boundary.py

Optional:
    python3 models/johnpye/fprops/test/feoh_hydrogen_boundary.py \
        --temps-c 600,700,800,900,1000,1100,1200
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

from feo_hidayat_validation import (
    R,
    atpct_o_from_x,
    g_fe_bcc,
    g_fe_fcc,
    g_fe3o4,
    minimize_abs_residual_at_T,
    mu_a_wustite,
    mu_b_wustite,
    residual_wustite_magnetite,
    residual_wustite_spinel_degterov,
    residual_wustite_spinel_mmc1_guess,
    residual_wustite_spinel_mmc1_selective_best,
    residual_wustite_spinel_mmc1_tapered_fit,
    spinel_phase_g,
    spinel_phase_g_mmc1_guess,
    spinel_phase_g_mmc1_selective_best,
    spinel_phase_g_mmc1_tapered_fit,
)

P0 = 1e5
SPINEL_BG_TUNED_DG_AE_A = -16984.331545915302
SPINEL_BG_TUNED_DG_AE_B = 18.769347422265838
SPINEL_FE_LAMBDA_FIT_A_KJ_PER_MOL_O = 4.476095744721812
SPINEL_FE_LAMBDA_FIT_B_KJ_PER_MOL_O_PER_C = -0.007137933743794555

GAS_SOURCE_ALIASES = {
    "reaktoro": "reaktoro_clone_supcrt98",
    "supcrt98": "reaktoro_clone_supcrt98",
    "ms": "moran_and_shapiro",
    "m&s": "moran_and_shapiro",
    "helm_ref0": "helmholtz+ref0:",
    "helmholtz_ref0": "helmholtz+ref0:",
    "rpp": "ideal+ref0:RPP",
    "rpp_ref0": "ideal+ref0:RPP",
}


def default_runner() -> Path:
    return Path(__file__).resolve().parent / "eqm_mu0_runner"


def parse_temps_c(text: str) -> list[float]:
    out: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            out.append(float(tok))
    if not out:
        raise ValueError("no temperatures provided")
    return out


def normalize_gas_source(source: str) -> str:
    key = source.strip()
    return GAS_SOURCE_ALIASES.get(key.lower(), key)


def query_mu0(runner: Path, source: str, tk: float, species: list[str]) -> dict[str, float]:
    cmd = [str(runner), source, f"{tk:.12g}", f"{P0:.12g}", *species]
    out = subprocess.check_output(cmd, text=True)
    data = json.loads(out)
    vals = data["mu0"]
    if len(vals) != len(species):
        raise RuntimeError("unexpected eqm_mu0_runner output length")
    result: dict[str, float] = {}
    for name, val in zip(species, vals):
        if val is None or not math.isfinite(float(val)):
            raise RuntimeError(f"missing/non-finite mu0 for {name} at T={tk:.2f} K")
        result[name] = float(val)
    return result


def stable_fe_label(T: float) -> str:
    return "bcc" if g_fe_bcc(T) <= g_fe_fcc(T) else "fcc"


def stable_fe_g(T: float) -> float:
    return g_fe_bcc(T) if stable_fe_label(T) == "bcc" else g_fe_fcc(T)


def residual_stable_fe_wustite(T: float, x: float) -> float:
    mu_a = mu_a_wustite(T, x)
    mu_b = mu_b_wustite(T, x)
    lam_fe = 3.0 * mu_a - 2.0 * mu_b
    return stable_fe_g(T) - lam_fe


def oxygen_potential_at_fe_wustite_boundary(T: float) -> tuple[float, float]:
    x_best, resid = minimize_abs_residual_at_T(T, residual_stable_fe_wustite)
    if not math.isfinite(x_best) or not math.isfinite(resid):
        raise RuntimeError(f"failed to locate Fe|wustite boundary at T={T:.2f} K")
    mu_a = mu_a_wustite(T, x_best)
    mu_b = mu_b_wustite(T, x_best)
    lam_o = 2.0 * (mu_b - mu_a)
    return x_best, lam_o


def gas_ratio_logs(mu_h2: float, mu_h2o: float, lam_o: float, T: float) -> tuple[float, float]:
    ln_ratio = (lam_o - (mu_h2o - mu_h2)) / (R * T)
    log10_h2o_h2 = ln_ratio / math.log(10.0)
    log10_h2_h2o = -log10_h2o_h2
    return log10_h2o_h2, log10_h2_h2o


def oxygen_potential_at_wustite_spinel_boundary(T: float) -> tuple[float, float]:
    x_best, lam_o, _a, _b, _v, _resid = oxygen_potential_at_wustite_spinel_boundary_state(T)
    return x_best, lam_o


def _oxygen_potential_at_wustite_spinel_boundary_state_impl(
    T: float,
    residual_fn,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float, float, float]:
    if x_hint is None or not math.isfinite(x_hint):
        x_lo = 1e-4
        x_hi = 0.95
        rounds = 4
        steps_x = 80
    else:
        x_lo = max(1e-4, x_hint - 0.04)
        x_hi = min(0.95, x_hint + 0.04)
        rounds = 4
        steps_x = 28

    best_x = x_lo
    best_abs = math.inf
    best_a = a_hint if a_hint is not None and math.isfinite(a_hint) else math.nan
    best_b = b_hint if b_hint is not None and math.isfinite(b_hint) else math.nan
    best_v = math.nan
    best_signed = math.nan

    for _ in range(rounds):
        span_x = x_hi - x_lo
        a_seed = best_a
        b_seed = best_b
        for ix in range(steps_x + 1):
            x = x_lo + span_x * ix / steps_x
            resid, a_cur, b_cur, v_cur = residual_fn(T, x, a_hint=a_seed, b_hint=b_seed)
            a_seed = a_cur
            b_seed = b_cur
            abs_resid = abs(resid)
            if abs_resid < best_abs:
                best_x = x
                best_abs = abs_resid
                best_a = a_cur
                best_b = b_cur
                best_v = v_cur
                best_signed = resid
        dx = max(5e-4, 0.15 * span_x)
        x_lo = max(1e-4, best_x - dx)
        x_hi = min(0.95, best_x + dx)

    if not math.isfinite(best_x) or not math.isfinite(best_abs):
        raise RuntimeError(f"failed to locate wustite|spinel boundary at T={T:.2f} K")
    mu_a = mu_a_wustite(T, best_x)
    mu_b = mu_b_wustite(T, best_x)
    lam_o = 2.0 * (mu_b - mu_a)
    return best_x, lam_o, best_a, best_b, best_v, best_signed


def oxygen_potential_at_wustite_spinel_boundary_state(
    T: float,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float, float, float]:
    return _oxygen_potential_at_wustite_spinel_boundary_state_impl(
        T, residual_wustite_spinel_degterov, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
    )


def oxygen_potential_at_wustite_spinel_boundary_state_mmc1_guess(
    T: float,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float, float, float]:
    return _oxygen_potential_at_wustite_spinel_boundary_state_impl(
        T, residual_wustite_spinel_mmc1_guess, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
    )


def oxygen_potential_at_wustite_spinel_boundary_state_mmc1_tapered_fit(
    T: float,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float, float, float]:
    return _oxygen_potential_at_wustite_spinel_boundary_state_impl(
        T, residual_wustite_spinel_mmc1_tapered_fit, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
    )


def oxygen_potential_at_wustite_spinel_boundary_state_mmc1_selective_best(
    T: float,
    x_hint: float | None = None,
    a_hint: float | None = None,
    b_hint: float | None = None,
) -> tuple[float, float, float, float, float, float]:
    return _oxygen_potential_at_wustite_spinel_boundary_state_impl(
        T, residual_wustite_spinel_mmc1_selective_best, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
    )


def trace_wustite_spinel_boundary(temps_k: list[float]) -> list[tuple[float, float, float, float, float, float]]:
    states: list[tuple[float, float, float, float, float, float]] = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tk in temps_k:
        state = oxygen_potential_at_wustite_spinel_boundary_state(
            tk, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        states.append(state)
        x_hint, _lam_o, a_hint, b_hint, _v, _resid = state
    return states


def trace_wustite_spinel_boundary_mmc1_guess(
    temps_k: list[float],
) -> list[tuple[float, float, float, float, float, float]]:
    states: list[tuple[float, float, float, float, float, float]] = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tk in temps_k:
        state = oxygen_potential_at_wustite_spinel_boundary_state_mmc1_guess(
            tk, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        states.append(state)
        x_hint, _lam_o, a_hint, b_hint, _v, _resid = state
    return states


def trace_wustite_spinel_boundary_mmc1_tapered_fit(
    temps_k: list[float],
) -> list[tuple[float, float, float, float, float, float]]:
    states: list[tuple[float, float, float, float, float, float]] = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tk in temps_k:
        state = oxygen_potential_at_wustite_spinel_boundary_state_mmc1_tapered_fit(
            tk, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        states.append(state)
        x_hint, _lam_o, a_hint, b_hint, _v, _resid = state
    return states


def trace_wustite_spinel_boundary_mmc1_selective_best(
    temps_k: list[float],
) -> list[tuple[float, float, float, float, float, float]]:
    states: list[tuple[float, float, float, float, float, float]] = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tk in temps_k:
        state = oxygen_potential_at_wustite_spinel_boundary_state_mmc1_selective_best(
            tk, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        states.append(state)
        x_hint, _lam_o, a_hint, b_hint, _v, _resid = state
    return states


def oxygen_potential_at_wustite_spinel_boundary_slow(T: float) -> tuple[float, float]:
    def spinel_abs_residual(T_inner: float, x: float) -> float:
        resid, _a, _b, _v = residual_wustite_spinel_degterov(T_inner, x)
        return abs(resid)

    x_best, resid = minimize_abs_residual_at_T(T, spinel_abs_residual)
    if not math.isfinite(x_best) or not math.isfinite(resid):
        raise RuntimeError(f"failed to locate wustite|spinel boundary at T={T:.2f} K")
    mu_a = mu_a_wustite(T, x_best)
    mu_b = mu_b_wustite(T, x_best)
    lam_o = 2.0 * (mu_b - mu_a)
    return x_best, lam_o


def oxygen_potential_at_wustite_magnetite_boundary(T: float) -> tuple[float, float]:
    x_best, resid = minimize_abs_residual_at_T(T, residual_wustite_magnetite)
    if not math.isfinite(x_best) or not math.isfinite(resid):
        raise RuntimeError(f"failed to locate wustite|magnetite boundary at T={T:.2f} K")
    mu_a = mu_a_wustite(T, x_best)
    mu_b = mu_b_wustite(T, x_best)
    lam_o = 2.0 * (mu_b - mu_a)
    return x_best, lam_o


def oxygen_potential_at_fe_spinel_boundary(T: float) -> tuple[str, float]:
    """
    Fe|spinel boundary using the reduced Degterov Fe-only spinel model.

    For fixed metallic iron potential lambda_Fe = g_Fe(T), the spinel phase
    becomes co-stable when its minimized grand potential equals zero:

        min_{a,b} [ g_spinel(a,b) - lambda_Fe n_Fe(a,b) - lambda_O * 4 ] = 0

    which gives

        lambda_O = min_{a,b} [ g_spinel(a,b) - lambda_Fe n_Fe(a,b) ] / 4
    """
    gfe = stable_fe_g(T)
    best = math.inf
    a_lo = 0.0
    a_hi = 1.0
    best_a = 0.0
    best_b = 0.5
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            steps_b = 80
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
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return stable_fe_label(T), best


def oxygen_potential_at_fe_spinel_boundary_bg_tuned(T: float) -> tuple[str, float]:
    """
    Provisional low-temperature BG-tuned Fe|spinel boundary.

    This keeps the reduced Fe-only Degterov/Hidayat spinel model but applies
    a small affine correction to the Fe3O4 spinel endmember:

        dG_AE(T) = A + B T

    with A,B fitted against the low-temperature BG Fe|spinel traces as a
    diagnostic source variant. This is intentionally not the default model.
    """
    gfe = stable_fe_g(T)
    best = math.inf
    a_lo = 0.0
    a_hi = 1.0
    best_a = 0.0
    best_b = 0.5
    dg_affine = SPINEL_BG_TUNED_DG_AE_A + SPINEL_BG_TUNED_DG_AE_B * T
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            steps_b = 80
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g + dg_affine - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return stable_fe_label(T), best


def oxygen_potential_at_fe_spinel_boundary_mmc1_guess(T: float) -> tuple[str, float]:
    gfe = stable_fe_g(T)
    best = math.inf
    a_lo = 0.0
    a_hi = 1.0
    best_a = 0.0
    best_b = 0.5
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            steps_b = 80
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g_mmc1_guess(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return stable_fe_label(T), best


def oxygen_potential_at_fe_spinel_boundary_mmc1_tapered_fit(T: float) -> tuple[str, float]:
    gfe = stable_fe_g(T)
    best = math.inf
    a_lo = 0.0
    a_hi = 1.0
    best_a = 0.0
    best_b = 0.5
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            steps_b = 80
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g_mmc1_tapered_fit(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return stable_fe_label(T), best


def oxygen_potential_at_fe_spinel_boundary_mmc1_selective_best(T: float) -> tuple[str, float]:
    gfe = stable_fe_g(T)
    best = math.inf
    a_lo = 0.0
    a_hi = 1.0
    best_a = 0.0
    best_b = 0.5
    for _ in range(4):
        steps_a = 80
        span_a = a_hi - a_lo
        for ia in range(steps_a + 1):
            a = a_lo + span_a * ia / steps_a
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
            steps_b = 80
            for ib in range(steps_b + 1):
                b = b_lo + (b_hi - b_lo) * ib / steps_b
                g, n_fe, _yo_fe3, _yo_va = spinel_phase_g_mmc1_selective_best(T, a, b)
                if not math.isfinite(g):
                    continue
                trial = (g - gfe * n_fe) / 4.0
                if trial < best:
                    best = trial
                    best_a = a
                    best_b = b
        da = max(0.01, 0.2 * span_a)
        a_lo = max(0.0, best_a - da)
        a_hi = min(1.0, best_a + da)
    return stable_fe_label(T), best


def oxygen_potential_at_fe_spinel_boundary_lambda_fit(T: float) -> tuple[str, float]:
    """
    Diagnostic Fe|spinel branch-only correction on a common oxygen-potential basis.

    This does not define a new thermodynamic source. It applies an empirical
    correction only to the reduced Fe|spinel boundary oxygen potential:

        d(lambda_O) [kJ/mol O] = A + B T_C

    with A,B fitted jointly to the low-temperature H2 and CO BG Fe|spinel
    traces after converting both to the common lambda_O basis. It is useful as
    a localization test because it leaves the oxidized-side Hidayat constraints
    and the wustite|spinel branch unchanged.
    """
    phase, lam_o = oxygen_potential_at_fe_spinel_boundary(T)
    tc = T - 273.15
    delta_kj_per_mol_o = (
        SPINEL_FE_LAMBDA_FIT_A_KJ_PER_MOL_O
        + SPINEL_FE_LAMBDA_FIT_B_KJ_PER_MOL_O_PER_C * tc
    )
    return phase, lam_o - 1000.0 * delta_kj_per_mol_o


def oxygen_potential_at_fe_magnetite_boundary(T: float) -> tuple[str, float]:
    """
    Stoichiometric Fe|Fe3O4 boundary using the current metallic Fe model and
    the stoichiometric Hidayat Fe3O4 surrogate.
    """
    gfe = stable_fe_g(T)
    lam_o = (g_fe3o4(T) - 3.0 * gfe) / 4.0
    return stable_fe_label(T), lam_o


def print_table(title: str, temps_c: list[float], boundary_fn, runner: Path, gas_source: str) -> None:
    print(title)
    print("Condensed source: Hidayat 2015 Fe-O implementation")
    print(f"Gas source: {gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'phase':>8} {'x_wus':>10} {'at%O':>8} "
        f"{'log10(H2O/H2)':>15} {'log10(H2/H2O)':>15}"
    )
    for tc in temps_c:
        tk = tc + 273.15
        if boundary_fn in (oxygen_potential_at_fe_magnetite_boundary, oxygen_potential_at_fe_spinel_boundary):
            phase, lam_o = boundary_fn(tk)
            x_best = math.nan
            atpct = math.nan
        else:
            x_best, lam_o = boundary_fn(tk)
            atpct = atpct_o_from_x(x_best)
            phase = stable_fe_label(tk) if boundary_fn is oxygen_potential_at_fe_wustite_boundary else "spinel"
        mu = query_mu0(runner, gas_source, tk, ["hydrogen", "water"])
        log10_h2o_h2, log10_h2_h2o = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, tk)
        print(
            f"{tc:6.0f} {phase:>8} "
            f"{x_best if math.isfinite(x_best) else float('nan'):10.6f} "
            f"{atpct if math.isfinite(atpct) else float('nan'):8.3f} "
            f"{log10_h2o_h2:15.6f} {log10_h2_h2o:15.6f}"
        )
    print()


def main() -> int:
    ap = argparse.ArgumentParser(description="First-pass Fe|wustite|H2/H2O boundary table.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--gas-source",
        default="helmholtz+ref0:",
        help="Gas mu0 source for hydrogen and water.",
    )
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000,1100,1200",
        help="Comma-separated temperatures in Celsius.",
    )
    ap.add_argument(
        "--boundary",
        choices=("fe-wustite", "wustite-spinel", "fe-spinel", "fe-magnetite", "both"),
        default="both",
        help="Which condensed boundary to tabulate.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"Runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_c = parse_temps_c(args.temps_c)
    gas_source = normalize_gas_source(args.gas_source)

    if args.boundary in ("fe-wustite", "both"):
        print_table(
            "First-pass Tier 3 Fe|wustite|H2/H2O boundary",
            temps_c,
            oxygen_potential_at_fe_wustite_boundary,
            args.runner,
            gas_source,
        )

    if args.boundary in ("wustite-spinel", "both"):
        print_table(
            "Provisional Tier 3 wustite|spinel|H2/H2O boundary",
            temps_c,
            oxygen_potential_at_wustite_spinel_boundary,
            args.runner,
            gas_source,
        )

    if args.boundary == "fe-spinel":
        print_table(
            "Provisional Tier 3 Fe|spinel|H2/H2O boundary",
            temps_c,
            oxygen_potential_at_fe_spinel_boundary,
            args.runner,
            gas_source,
        )

    if args.boundary == "fe-magnetite":
        print_table(
            "Stoichiometric Tier 3 Fe|Fe3O4|H2/H2O boundary",
            temps_c,
            oxygen_potential_at_fe_magnetite_boundary,
            args.runner,
            gas_source,
        )

    print("Interpretation")
    print("- The Fe|wustite table is the most trustworthy Tier 3 output at present.")
    print("- The wustite|spinel table is useful for trend-checking, but still provisional.")
    print(f"- Gas source used here: {gas_source}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
