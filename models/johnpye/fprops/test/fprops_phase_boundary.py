#!/usr/bin/env python3
"""
Boundary helpers backed by the FPROPS phase API Python bindings.

These routines are intentionally small diagnostic glue for the Baur-Glaessner
comparison scripts. They compute oxygen potentials from C phase models via
`fprops.EqmPhase.entry_residual(...)`; gas conversion is left to each plotting
script so the existing H2/H2O and CO/CO2 source choices stay visible.
"""

from __future__ import annotations

import math
import sys
from functools import lru_cache
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
PYTHON_DIR = THIS_DIR.parent / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

import fprops  # noqa: E402

P0 = 101325.0


def spinel_source(spinel_variant: str) -> str:
    if spinel_variant in ("current", ""):
        return "degterov_2001"
    if spinel_variant == "bg_tuned":
        return "fe_spinel_bg_tuned_2026"
    if spinel_variant == "mmc1_guess":
        return "fe_spinel_mmc1_guess_2026"
    if spinel_variant in ("hidayat_adj1", "mmc1_selective_best"):
        return "hidayat_adj1"
    raise ValueError(f"no C phase source is available for spinel variant {spinel_variant!r}")


@lru_cache(maxsize=None)
def phase(spec: str):
    return fprops.EqmPhase(spec)


def stable_fe_phase(T: float):
    bcc = phase("Fe_bcc=hidayat_2015")
    fcc = phase("Fe_fcc=hidayat_2015")
    g_bcc = bcc.gibbs(T, P0, [])
    g_fcc = fcc.gibbs(T, P0, [])
    return ("Fe_bcc", g_bcc) if g_bcc <= g_fcc else ("Fe_fcc", g_fcc)


def find_root_scan(fn, lo: float, hi: float, steps: int = 500, tol: float = 1e-10) -> float:
    last_x: float | None = None
    last_f: float | None = None
    bracket: tuple[float, float, float, float] | None = None
    for i in range(steps + 1):
        x = lo + (hi - lo) * i / steps
        try:
            fx = float(fn(x))
        except Exception:
            continue
        if not math.isfinite(fx):
            continue
        if abs(fx) <= tol:
            return x
        if last_f is not None and last_x is not None and last_f * fx <= 0.0:
            bracket = (last_x, x, last_f, fx)
            break
        last_x = x
        last_f = fx
    if bracket is None:
        raise RuntimeError(f"failed to bracket root over [{lo:g}, {hi:g}]")

    a, b, fa, _fb = bracket
    for _ in range(90):
        m = 0.5 * (a + b)
        fm = float(fn(m))
        if abs(fm) <= tol or abs(b - a) <= 1e-9:
            return m
        if fa * fm <= 0.0:
            b = m
        else:
            a = m
            fa = fm
    return 0.5 * (a + b)


def phase_entry_phi(model, T: float, lambda_fe: float, lambda_o: float) -> float:
    phi, _coords = model.entry_residual(T, P0, [lambda_fe, lambda_o])
    return phi


def lambda_fe_for_phase(model, T: float, lambda_o: float) -> float:
    return find_root_scan(
        lambda lambda_fe: phase_entry_phi(model, T, lambda_fe, lambda_o),
        -500000.0,
        200000.0,
        steps=500,
    )


@lru_cache(maxsize=None)
def phase_boundary_lambda_o(boundary: str, T: float, spinel_variant: str = "current") -> tuple[float, dict[str, object]]:
    wustite = phase("wustite=hidayat_2015")
    spinel = phase(f"spinel={spinel_source(spinel_variant)}")
    fe_name, lambda_fe = stable_fe_phase(T)

    if boundary == "fe-wustite":
        lambda_o = find_root_scan(
            lambda lo: phase_entry_phi(wustite, T, lambda_fe, lo),
            -800000.0,
            100000.0,
            steps=600,
        )
        return lambda_o, {"fe": fe_name}

    if boundary == "fe-spinel":
        lambda_o = find_root_scan(
            lambda lo: phase_entry_phi(spinel, T, lambda_fe, lo),
            -800000.0,
            100000.0,
            steps=600,
        )
        return lambda_o, {"fe": fe_name, "spinel_source": spinel_source(spinel_variant)}

    if boundary == "wustite-spinel":
        def residual(lambda_o: float) -> float:
            lambda_fe_w = lambda_fe_for_phase(wustite, T, lambda_o)
            return phase_entry_phi(spinel, T, lambda_fe_w, lambda_o)

        lambda_o = find_root_scan(residual, -800000.0, 100000.0, steps=500)
        return lambda_o, {"spinel_source": spinel_source(spinel_variant)}

    raise KeyError(boundary)
