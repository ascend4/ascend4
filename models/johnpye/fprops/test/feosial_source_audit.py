#!/usr/bin/env python3
"""
Mixed-source audit for the pragmatic Fe-O-Si-Al extension.

This script compares:

1. Fe-O-Si oxygen buffers against O'Neill (1987):
   - QFM: 3Fe2SiO4 + O2 = 3SiO2 + 2Fe3O4
   - QFI: Fe2SiO4 = 2Fe + SiO2 + O2

2. Gas-reaction standard-state thermodynamics across candidate gas sources:
   - H2 + 0.5 O2 -> H2O
   - CO + 0.5 O2 -> CO2
   - CO + H2O -> CO2 + H2

The goal is to expose basis inconsistencies cleanly in one report.
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

R = 8.31446261815324
P0 = 1e5

QFM_O2_SOURCES = [
    "reaktoro_clone_supcrt98",
    "helmholtz+ref0:",
    "Moran and Shapiro",
    "ideal+ref0:RPP",
]

QFI_O2_SOURCES = [
    "helmholtz+ref0:",
    "Moran and Shapiro",
    "ideal+ref0:RPP",
    "reaktoro_clone_supcrt98",
]

GAS_SOURCES = [
    "reaktoro_clone_supcrt98",
    "helmholtz+ref0:",
    "Moran and Shapiro",
    "ideal+ref0:RPP",
]

FAYALITE_SOURCES = [
    ("slag_pragmatic_2026", "Robie/Benisek"),
    ("hidayat_2017_feo_fe2o3_sio2", "Hidayat 2017"),
]

GAS_REACTIONS = [
    {
        "title": "H2 + 0.5 O2 -> H2O",
        "species": ["hydrogen", "oxygen", "water"],
        "nu": [-1.0, -0.5, 1.0],
    },
    {
        "title": "CO + 0.5 O2 -> CO2",
        "species": ["carbonmonoxide", "oxygen", "carbondioxide"],
        "nu": [-1.0, -0.5, 1.0],
    },
    {
        "title": "CO + H2O -> CO2 + H2",
        "species": ["carbonmonoxide", "water", "carbondioxide", "hydrogen"],
        "nu": [-1.0, -1.0, 1.0, 1.0],
    },
]


def parse_temps(text: str) -> list[float]:
    vals: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            vals.append(float(tok))
    if not vals:
        raise ValueError("no temperatures provided")
    return vals


def default_runner() -> Path:
    return Path(__file__).resolve().parent / "eqm_mu0_runner"


def query_mu0(runner: Path, source: str, tk: float, species: list[str]) -> dict[str, float]:
    out = subprocess.check_output(
        [str(runner), source, f"{tk:.12g}", f"{P0:.12g}", *species],
        text=True,
    )
    data = json.loads(out)
    mu = data["mu0"]
    if len(mu) != len(species):
        raise RuntimeError("unexpected mu0 length from eqm_mu0_runner")
    vals: dict[str, float] = {}
    for name, value in zip(species, mu):
        if value is None or not math.isfinite(float(value)):
            raise RuntimeError(
                f"non-finite mu0 for '{name}' with source '{source}' at T={tk:.2f} K"
            )
        vals[name] = float(value)
    return vals


def log10_from_mu_o2(mu_o2: float, tk: float) -> float:
    return mu_o2 / (R * tk * math.log(10.0))


def qfm_oneill_1987_log10fo2(tk: float) -> float:
    if not (900.0 < tk < 1420.0):
        return math.nan
    mu_o2 = -587474.0 + 1584.427 * tk - 203.3164 * tk * math.log(tk) + 0.09271 * tk * tk
    return log10_from_mu_o2(mu_o2, tk)


def qfi_oneill_1987_log10fo2(tk: float) -> float:
    if not (900.0 < tk < 1420.0):
        return math.nan
    if tk < 1042.0:
        mu_o2 = -542941.0 - 33.182 * tk + 22.446 * tk * math.log(tk)
    elif tk <= 1184.0:
        mu_o2 = -562377.0 + 103.384 * tk + 5.4771 * tk * math.log(tk)
    else:
        mu_o2 = -602739.0 + 369.704 * tk - 27.3443 * tk * math.log(tk)
    return log10_from_mu_o2(mu_o2, tk)


def qfm_log10fo2(runner: Path, tk: float, o2_source: str, fayalite_source: str) -> float:
    mu = query_mu0(
        runner,
        (
            "Fe3O4=hidayat_2015;"
            "SiO2=slag_pragmatic_2026;"
            f"Fe2SiO4={fayalite_source};"
            f"oxygen={o2_source}"
        ),
        tk,
        ["Fe3O4", "SiO2", "Fe2SiO4", "oxygen"],
    )
    mu_buffer = 2.0 * mu["Fe3O4"] + 3.0 * mu["SiO2"] - 3.0 * mu["Fe2SiO4"] - mu["oxygen"]
    return log10_from_mu_o2(mu_buffer, tk)


def qfi_log10fo2(runner: Path, tk: float, o2_source: str, fayalite_source: str) -> tuple[float, str]:
    mu = query_mu0(
        runner,
        (
            "Fe_bcc=hidayat_2015;"
            "Fe_fcc=hidayat_2015;"
            "SiO2=slag_pragmatic_2026;"
            f"Fe2SiO4={fayalite_source};"
            f"oxygen={o2_source}"
        ),
        tk,
        ["Fe_bcc", "Fe_fcc", "SiO2", "Fe2SiO4", "oxygen"],
    )
    if mu["Fe_bcc"] <= mu["Fe_fcc"]:
        mu_fe = mu["Fe_bcc"]
        phase = "Fe_bcc"
    else:
        mu_fe = mu["Fe_fcc"]
        phase = "Fe_fcc"
    mu_buffer = mu["Fe2SiO4"] - mu["SiO2"] - 2.0 * mu_fe - mu["oxygen"]
    return log10_from_mu_o2(mu_buffer, tk), phase


def reaction_dg_log10k(mu: dict[str, float], species: list[str], nu: list[float], tk: float) -> tuple[float, float]:
    dg = sum(coeff * mu[name] for name, coeff in zip(species, nu))
    log10k = -dg / (R * tk * math.log(10.0))
    return dg, log10k


def print_qfm_table(runner: Path, tk: float) -> None:
    ref = qfm_oneill_1987_log10fo2(tk)
    print(f"QFM audit at T = {tk:.2f} K")
    print(f"{'fayalite':<15} {'O2 source':<24} {'log10 fO2':>13} {'ref':>13} {'delta':>13}")
    for fayalite_source, fayalite_label in FAYALITE_SOURCES:
        for src in QFM_O2_SOURCES:
            value = qfm_log10fo2(runner, tk, src, fayalite_source)
            delta = value - ref if math.isfinite(ref) else math.nan
            print(f"{fayalite_label:<15} {src:<24} {value:13.6f} {ref:13.6f} {delta:13.6f}")
    print()


def print_qfi_table(runner: Path, tk: float) -> None:
    ref = qfi_oneill_1987_log10fo2(tk)
    print(f"QFI audit at T = {tk:.2f} K")
    print(f"{'fayalite':<15} {'O2 source':<24} {'Fe':<8} {'log10 fO2':>13} {'ref':>13} {'delta':>13}")
    for fayalite_source, fayalite_label in FAYALITE_SOURCES:
        for src in QFI_O2_SOURCES:
            value, phase = qfi_log10fo2(runner, tk, src, fayalite_source)
            delta = value - ref if math.isfinite(ref) else math.nan
            print(f"{fayalite_label:<15} {src:<24} {phase:<8} {value:13.6f} {ref:13.6f} {delta:13.6f}")
    print()


def print_gas_tables(runner: Path, tk: float) -> None:
    for rxn in GAS_REACTIONS:
        baseline_log10k = None
        print(f"Gas audit at T = {tk:.2f} K: {rxn['title']}")
        print(f"{'source':<24} {'dG [kJ/mol]':>14} {'log10 K':>13} {'dlogK vs clone':>16}")
        for src in GAS_SOURCES:
            mu = query_mu0(runner, src, tk, rxn["species"])
            dg, log10k = reaction_dg_log10k(mu, rxn["species"], rxn["nu"], tk)
            if baseline_log10k is None:
                baseline_log10k = log10k
            dlogk = log10k - baseline_log10k
            print(f"{src:<24} {dg / 1000.0:14.6f} {log10k:13.6f} {dlogk:16.6f}")
        print()


def main() -> int:
    ap = argparse.ArgumentParser(description="Fe-O-Si-Al source/basis audit.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument(
        "--temps-k",
        default="1000",
        help="Comma-separated temperatures in kelvin.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        print(f"runner not found: {args.runner}", file=sys.stderr)
        return 2

    temps_k = parse_temps(args.temps_k)
    print("Fe-O-Si-Al source audit")
    print("Condensed basis: Fe oxides/metal from hidayat_2015, quartz from slag_pragmatic_2026; fayalite varied")
    print()

    for tk in temps_k:
        print_qfm_table(args.runner, tk)
        print_qfi_table(args.runner, tk)
        print_gas_tables(args.runner, tk)

    print("Interpretation")
    print("- QFM and QFI are expected to expose mixed-source consistency, not just O2 quality in isolation.")
    print("- Gas reactions isolate source differences without condensed-phase terms.")
    print("- If one gas source is best for QFM and another for QFI, the basis mismatch is system-level and should be investigated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
