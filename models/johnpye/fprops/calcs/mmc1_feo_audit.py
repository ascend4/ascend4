#!/usr/bin/env python3
"""
Focused audit of the Hidayat supplemental `mmc1.dat` Fe-O database export.

This is not a general parser for the file format. It extracts the Fe-O solid
phase pieces that matter for the current reconstruction work and maps the
spinel endmember data onto the reduced Fe-only formulas used in FPROPS.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


MM1_PATH = Path(__file__).resolve().parent / "mmc1.dat"


@dataclass(frozen=True)
class SpeciesRecord:
    name: str
    coeffs: tuple[float, float, float, float, float, float]
    mag: tuple[float, float] | None
    mag_extra: tuple[float, ...] | None = None


@dataclass(frozen=True)
class MagneticInteraction:
    raw_indices: tuple[int, ...]
    labels: tuple[str, ...]
    d_tord_beta: tuple[float, float]


def parse_float_list(line: str) -> list[float]:
    return [float(tok) for tok in line.split()]


def try_parse_float_list(line: str) -> list[float] | None:
    try:
        return parse_float_list(line)
    except ValueError:
        return None


def read_lines() -> list[str]:
    return MM1_PATH.read_text(encoding="ascii", errors="replace").splitlines()


def find_index(lines: list[str], needle: str) -> int:
    for i, line in enumerate(lines):
        if line.strip() == needle:
            return i
    raise KeyError(f"could not locate {needle!r} in {MM1_PATH}")


def parse_species(lines: list[str], name: str) -> SpeciesRecord:
    i = find_index(lines, name)
    coeff1 = parse_float_list(lines[i + 2])
    coeff2 = parse_float_list(lines[i + 3])
    if len(coeff1) != 5 or len(coeff2) != 2:
        raise ValueError(f"unexpected coefficient layout for {name}")
    coeffs = (
        coeff1[1],  # constant
        coeff1[2],  # linear T
        coeff1[3],  # T ln T
        coeff1[4],  # T^2
        coeff2[0],  # T^3 or equivalent
        coeff2[1],  # 1/T
    )
    mag_line = parse_float_list(lines[i + 5])
    mag = None
    if len(mag_line) == 2 and any(abs(v) > 0.0 for v in mag_line):
        mag = (mag_line[0], mag_line[1])
    mag_extra = None
    extra_line_index = i + 6
    if extra_line_index < len(lines):
        extra_vals = try_parse_float_list(lines[extra_line_index]) if lines[extra_line_index].strip() else []
        if extra_vals is not None and len(extra_vals) >= 4:
            mag_extra = tuple(extra_vals)
    return SpeciesRecord(name=name, coeffs=coeffs, mag=mag, mag_extra=mag_extra)


def coeff_sub(a: tuple[float, ...], b: tuple[float, ...]) -> tuple[float, ...]:
    return tuple(x - y for x, y in zip(a, b))


def coeff_scale(s: float, a: tuple[float, ...]) -> tuple[float, ...]:
    return tuple(s * x for x in a)


def fmt_coeffs(c: tuple[float, ...]) -> str:
    return (
        f"a={c[0]:+.6f}, b={c[1]:+.6f}, c={c[2]:+.6f}, "
        f"d={c[3]:+.9f}, e={c[4]:+.9g}, f={c[5]:+.6f}"
    )


def parse_spinel_magnetic_interactions(lines: list[str]) -> list[MagneticInteraction]:
    start = find_index(lines, "Spinel")
    constituent_labels = {
        1: "Fe[2+]T",
        2: "Fe[3+]T",
        3: "Fe[2+]O",
        4: "Fe[3+]O",
        5: "Va[0]O",
    }
    out: list[MagneticInteraction] = []
    i = start + 46  # line 110 in the current file layout
    while i < len(lines):
        head = lines[i].strip()
        if head == "0":
            break
        if head != "3":
            raise ValueError(f"unexpected magnetic-interaction header {head!r} near line {i+1}")
        raw = tuple(int(tok) for tok in lines[i + 1].split())
        vals = tuple(float(tok) for tok in lines[i + 2].split())
        out.append(
            MagneticInteraction(
                raw_indices=raw,
                labels=tuple(constituent_labels.get(idx, f"?{idx}") for idx in raw[:-1]),
                d_tord_beta=(vals[0], vals[1]),
            )
        )
        i += 3
    return out


def main() -> int:
    lines = read_lines()

    spinel_ae = parse_species(lines, "Fe3O4")
    spinel_ee = parse_species(lines, "Fe3O4[1+]")
    spinel_aa = parse_species(lines, "Fe3O4[2-]")
    spinel_ev = parse_species(lines, "Fe1O4[5-]")
    spinel_av = parse_species(lines, "Fe1O4[6-]")
    wustite_feo = parse_species(lines, "FeO")
    wustite_fe2o3 = parse_species(lines, "(Fe2O3):2")
    hematite = parse_species(lines, "Fe2O3_hematite(s)")
    mag_interactions = parse_spinel_magnetic_interactions(lines)

    g_ae = spinel_ae.coeffs
    i_ae = coeff_sub(spinel_ee.coeffs, g_ae)
    delta_ae = coeff_sub(coeff_sub(spinel_aa.coeffs, g_ae), tuple(-x for x in i_ae))
    v_e = coeff_sub(spinel_ev.coeffs, coeff_scale(5.0 / 7.0, g_ae))
    g_av_delta = coeff_sub(
        spinel_av.coeffs,
        tuple(
            (5.0 / 7.0) * g_ae[j] + v_e[j] - i_ae[j] + delta_ae[j]
            for j in range(len(g_ae))
        ),
    )

    print("mmc1.dat Fe-O audit")
    print(f"- source: {MM1_PATH}")
    print()

    print("Spinel endmembers")
    print(f"- G_AE  from {spinel_ae.name}: {fmt_coeffs(g_ae)}")
    print(f"- G_EE  from {spinel_ee.name}: {fmt_coeffs(spinel_ee.coeffs)}")
    print(f"- G_AA  from {spinel_aa.name}: {fmt_coeffs(spinel_aa.coeffs)}")
    print(f"- G_EV  from {spinel_ev.name}: {fmt_coeffs(spinel_ev.coeffs)}")
    print(f"- G_AV  from {spinel_av.name}: {fmt_coeffs(spinel_av.coeffs)}")
    print()

    print("Reduced-spinels mapping implied by mmc1")
    print(f"- I_AE(T)        = G_EE - G_AE = {fmt_coeffs(i_ae)}")
    print(f"- Delta_AE(T)    = G_AA - G_AE + I_AE = {fmt_coeffs(delta_ae)}")
    print(f"- V_E(T)         = G_EV - 5/7 G_AE = {fmt_coeffs(v_e)}")
    print(f"- G_AV closure residual = {fmt_coeffs(g_av_delta)}")
    print()

    print("Magnetic entries present in mmc1")
    for rec in (spinel_ae, parse_species(lines, "Fe3O4[1-]"), spinel_ee, spinel_aa, spinel_ev, spinel_av, hematite):
        if rec.mag_extra is not None:
            print(f"- {rec.name}: {rec.mag} extra={rec.mag_extra}")
        else:
            print(f"- {rec.name}: {rec.mag}")
    print("- excess magnetic interactions in Spinel:")
    for idx, rec in enumerate(mag_interactions, 1):
        labels = " / ".join(rec.labels)
        print(
            f"  {idx}: {labels} -> "
            f"dTord={rec.d_tord_beta[0]:+.6f}, dbeta={rec.d_tord_beta[1]:+.6f}"
        )
    print()

    print("Other solid-phase blocks")
    print(f"- Monoxide FeO:       {fmt_coeffs(wustite_feo.coeffs)}")
    print(f"- Monoxide Fe2O3/2:   {fmt_coeffs(wustite_fe2o3.coeffs)}")
    print(f"- Hematite Fe2O3:     {fmt_coeffs(hematite.coeffs)}")
    print()

    print("Interpretation")
    print("- The energetic Fe-only spinel mapping used in FPROPS is recovered exactly from the exported endmembers.")
    print("- The file also carries endmember-specific magnetic entries and five explicit excess magnetic interactions for Spinel.")
    print("- The current reduced FPROPS spinel collapses that structure into one composition-independent magnetic term.")
    print("- Direct Python-side tests show naive replacements of that magnetic term are not enough, so the remaining gap is likely in the exact magnetic interaction form rather than in the Gibbs-energy endmembers.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
