#!/usr/bin/env python3
"""
Audit the reduced pure-Fe spinel manifold against the assessed `mmc1.dat`
spinel block.

Purpose:

- explain what the current "Fe-only spinel" model actually is,
- verify that vacancies are explicitly present,
- show that the current `(a,b) -> (c,v)` mapping is the charge-neutral
  reduction of the `Fe-O-e(Spinel)` constituent model in `mmc1.dat`, and
- check whether the non-magnetic Gibbs-energy surface on that neutral
  manifold already matches the assessed database exactly.

If it does, then the remaining low-temperature Fe|spinel gap is unlikely to
come from omitted vacancies, omitted endmembers, or a mass-vs-molar
misinterpretation of the spinel block.
"""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass
from pathlib import Path

from feo_hidayat_validation import (
    R,
    spinel_delta_ae,
    spinel_g_ae,
    spinel_i_ae,
    spinel_v_e,
)

MM1_PATH = Path(__file__).resolve().parent.parent / "calcs" / "mmc1.dat"


@dataclass(frozen=True)
class SpeciesRecord:
    name: str
    coeffs: tuple[float, float, float, float, float, float]
    stoich: tuple[float, float, float]


def read_lines() -> list[str]:
    return MM1_PATH.read_text(encoding="ascii", errors="replace").splitlines()


def find_index(lines: list[str], needle: str) -> int:
    for i, line in enumerate(lines):
        if line.strip() == needle:
            return i
    raise KeyError(f"could not locate {needle!r} in {MM1_PATH}")


def parse_float_list(line: str) -> list[float]:
    return [float(tok) for tok in line.split()]


def parse_species(lines: list[str], name: str) -> SpeciesRecord:
    i = find_index(lines, name)
    header = parse_float_list(lines[i + 1])
    if len(header) < 5:
        raise ValueError(f"unexpected species header for {name}")
    stoich = (header[2], header[3], header[4])  # Fe, O, e(Spinel)
    coeff1 = parse_float_list(lines[i + 2])
    coeff2 = parse_float_list(lines[i + 3])
    coeffs = (
        coeff1[1],
        coeff1[2],
        coeff1[3],
        coeff1[4],
        coeff2[0],
        coeff2[1],
    )
    return SpeciesRecord(name=name, coeffs=coeffs, stoich=stoich)


def eval_coeffs(coeffs: tuple[float, float, float, float, float, float], T: float) -> float:
    a, b, c, d, e, f = coeffs
    return a + b * T + c * T * math.log(T) + d * T * T + e * T * T * T + f / T


def safe_ylogy(y: float) -> float:
    return 0.0 if not (y > 0.0) else y * math.log(y)


def current_nonmag_spinel_g(T: float, a: float, b: float) -> tuple[float, float, float, float]:
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

    gmix = (
        yt_fe2 * yo_fe2 * g_aa
        + yt_fe2 * yo_fe3 * g_ae
        + yt_fe2 * yo_va * g_av
        + yt_fe3 * yo_fe2 * g_ea
        + yt_fe3 * yo_fe3 * g_ee
        + yt_fe3 * yo_va * g_ev
    )
    sconf = -R * (
        safe_ylogy(yt_fe2)
        + safe_ylogy(yt_fe3)
        + 2.0 * (safe_ylogy(yo_fe2) + safe_ylogy(yo_fe3) + safe_ylogy(yo_va))
    )
    return gmix - T * sconf, c, v, 1.0 + 2.0 * (yo_fe2 + yo_fe3)


def mmc1_nonmag_spinel_g(T: float, a: float, b: float, species: dict[str, SpeciesRecord]) -> tuple[float, float, float, float, float]:
    c = (a + 5.0 - 4.0 * b) / 6.0
    v = (1.0 - a - 2.0 * b) / 6.0
    if not (0.0 <= a <= 1.0 and 0.0 <= b <= 1.0 and 0.0 <= c <= 1.0 and 0.0 <= v <= 1.0):
        return math.nan, math.nan, math.nan, math.nan, math.nan

    w_aa = a * b
    w_ae = a * c
    w_av = a * v
    w_ea = (1.0 - a) * b
    w_ee = (1.0 - a) * c
    w_ev = (1.0 - a) * v

    coeffs = {
        "AA": species["Fe3O4[2-]"].coeffs,
        "AE": species["Fe3O4"].coeffs,
        "AV": species["Fe1O4[6-]"].coeffs,
        "EA": species["Fe3O4[1-]"].coeffs,
        "EE": species["Fe3O4[1+]"].coeffs,
        "EV": species["Fe1O4[5-]"].coeffs,
    }

    gmix = (
        w_aa * eval_coeffs(coeffs["AA"], T)
        + w_ae * eval_coeffs(coeffs["AE"], T)
        + w_av * eval_coeffs(coeffs["AV"], T)
        + w_ea * eval_coeffs(coeffs["EA"], T)
        + w_ee * eval_coeffs(coeffs["EE"], T)
        + w_ev * eval_coeffs(coeffs["EV"], T)
    )
    sconf = -R * (
        safe_ylogy(a)
        + safe_ylogy(1.0 - a)
        + 2.0 * (safe_ylogy(b) + safe_ylogy(c) + safe_ylogy(v))
    )

    n_fe = (
        w_aa * species["Fe3O4[2-]"].stoich[0]
        + w_ae * species["Fe3O4"].stoich[0]
        + w_av * species["Fe1O4[6-]"].stoich[0]
        + w_ea * species["Fe3O4[1-]"].stoich[0]
        + w_ee * species["Fe3O4[1+]"].stoich[0]
        + w_ev * species["Fe1O4[5-]"].stoich[0]
    )
    n_o = (
        w_aa * species["Fe3O4[2-]"].stoich[1]
        + w_ae * species["Fe3O4"].stoich[1]
        + w_av * species["Fe1O4[6-]"].stoich[1]
        + w_ea * species["Fe3O4[1-]"].stoich[1]
        + w_ee * species["Fe3O4[1+]"].stoich[1]
        + w_ev * species["Fe1O4[5-]"].stoich[1]
    )
    n_e = (
        w_aa * species["Fe3O4[2-]"].stoich[2]
        + w_ae * species["Fe3O4"].stoich[2]
        + w_av * species["Fe1O4[6-]"].stoich[2]
        + w_ea * species["Fe3O4[1-]"].stoich[2]
        + w_ee * species["Fe3O4[1+]"].stoich[2]
        + w_ev * species["Fe1O4[5-]"].stoich[2]
    )
    return gmix - T * sconf, n_fe, n_o, n_e, v


def sample_grid() -> list[tuple[float, float]]:
    out: list[tuple[float, float]] = []
    for a in (0.05, 0.20, 0.35, 0.50, 0.65, 0.80, 0.95):
        bmax = 0.5 * (1.0 - a)
        for frac in (0.05, 0.20, 0.40, 0.60, 0.80, 0.95):
            b = frac * bmax
            out.append((a, b))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare the reduced pure-Fe spinel manifold against mmc1.dat.")
    ap.add_argument("--temp-k", type=float, default=900.0, help="Temperature for the pointwise manifold check.")
    args = ap.parse_args()

    lines = read_lines()
    species = {
        name: parse_species(lines, name)
        for name in (
            "Fe3O4",
            "Fe3O4[1-]",
            "Fe3O4[1+]",
            "Fe3O4[2-]",
            "Fe1O4[5-]",
            "Fe1O4[6-]",
        )
    }

    print("Reduced pure-Fe spinel manifold audit")
    print(f"- source: {MM1_PATH}")
    print(f"- comparison temperature: {args.temp_k:.2f} K")
    print()
    print("Constituent/endmember stoichiometries from mmc1.dat (Fe, O, e)")
    for name in ("Fe3O4", "Fe3O4[1-]", "Fe3O4[1+]", "Fe3O4[2-]", "Fe1O4[5-]", "Fe1O4[6-]"):
        print(f"- {name:12s}: {species[name].stoich}")
    print()

    max_abs_gdiff = 0.0
    max_abs_ediff = 0.0
    max_abs_fediff = 0.0
    worst: tuple[float, float] | None = None
    for a, b in sample_grid():
        g_cur, c, v, nfe_cur = current_nonmag_spinel_g(args.temp_k, a, b)
        g_mmc1, nfe_mmc1, no_mmc1, ne_mmc1, v_mmc1 = mmc1_nonmag_spinel_g(args.temp_k, a, b, species)
        if not math.isfinite(g_cur) or not math.isfinite(g_mmc1):
            continue
        dg = g_cur - g_mmc1
        de = ne_mmc1
        dfe = nfe_cur - nfe_mmc1
        if abs(dg) > max_abs_gdiff:
            max_abs_gdiff = abs(dg)
            max_abs_ediff = abs(de)
            max_abs_fediff = abs(dfe)
            worst = (a, b)

    if worst is None:
        raise SystemExit("no valid grid points")

    a_w, b_w = worst
    g_cur, c_w, v_w, nfe_cur = current_nonmag_spinel_g(args.temp_k, a_w, b_w)
    g_mmc1, nfe_mmc1, no_mmc1, ne_mmc1, _v_mmc1 = mmc1_nonmag_spinel_g(args.temp_k, a_w, b_w, species)

    print("Worst sampled difference on the charge-neutral manifold")
    print(f"- a = y_t(Fe2+) = {a_w:.6f}")
    print(f"- b = y_o(Fe2+) = {b_w:.6f}")
    print(f"- implied c = y_o(Fe3+) = {c_w:.6f}")
    print(f"- implied v = y_o(Va)   = {v_w:.6f}")
    print(f"- current nonmag g      = {g_cur:.9f} J/mol phase")
    print(f"- mmc1    nonmag g      = {g_mmc1:.9f} J/mol phase")
    print(f"- delta g               = {g_cur - g_mmc1:+.9e} J/mol phase")
    print(f"- current n_Fe          = {nfe_cur:.9f}")
    print(f"- mmc1    n_Fe          = {nfe_mmc1:.9f}")
    print(f"- delta n_Fe            = {nfe_cur - nfe_mmc1:+.9e}")
    print(f"- mmc1    n_O           = {no_mmc1:.9f}")
    print(f"- mmc1    n_e           = {ne_mmc1:+.9e}")
    print()

    print("Interpretation")
    print("- The current reduced pure-Fe spinel model is charge-neutral by construction, not vacancy-free.")
    print("- The `e(Spinel)` component in mmc1.dat is bookkeeping for charged constituents; on the neutral manifold, it is eliminated exactly.")
    print("- If delta g and delta n_Fe are numerically negligible, then the non-magnetic assessed spinel manifold is already reproduced exactly by the current reduced pure-Fe formulas.")
    print("- In that case the remaining Fe|spinel issue is unlikely to come from omitted vacancies, omitted endmembers, or a mass-vs-molar basis mistake in the spinel block.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
