#!/usr/bin/env python3
"""
Decompose the mixed Hidayat-oxide + gas-package basis mismatch into:

1. an elemental-shift part
2. an irreducible balanced-reaction residual

This answers a specific question:

    Can the gas basis be reconciled with the Hidayat oxide basis by
    simple elemental reference shifts, or do balanced reaction
    mismatches remain?

The mixed FPROPS source used here is:

    Fe3O4, Fe2O3 -> hidayat_2015
    H2, H2O, O2, CO, CO2 -> helmholtz+ref0:

Compared against Reaktoro/SUPCRT98 for the same species.
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
from dataclasses import dataclass
from pathlib import Path

import numpy as np

THIS_DIR = Path(__file__).resolve().parent

SOURCE_MAP = (
    "Fe3O4=hidayat_2015;"
    "Fe2O3=hidayat_2015;"
    "hydrogen=helmholtz+ref0:;"
    "water=helmholtz+ref0:;"
    "oxygen=helmholtz+ref0:;"
    "carbonmonoxide=helmholtz+ref0:;"
    "carbondioxide=helmholtz+ref0:"
)

SPECIES = [
    "Fe3O4",
    "Fe2O3",
    "hydrogen",
    "water",
    "oxygen",
    "carbonmonoxide",
    "carbondioxide",
]

FORMULAS = {
    "Fe3O4": {"Fe": 3.0, "O": 4.0},
    "Fe2O3": {"Fe": 2.0, "O": 3.0},
    "hydrogen": {"H": 2.0},
    "water": {"H": 2.0, "O": 1.0},
    "oxygen": {"O": 2.0},
    "carbonmonoxide": {"C": 1.0, "O": 1.0},
    "carbondioxide": {"C": 1.0, "O": 2.0},
}

ELEMENTS = ["Fe", "H", "C", "O"]

REAKTORO_NAME_MAP = {
    "Fe3O4": "Magnetite",
    "Fe2O3": "Hematite",
}


@dataclass(frozen=True)
class Reaction:
    name: str
    nu: dict[str, float]


REACTIONS = (
    Reaction("H2 + 0.5 O2 -> H2O", {"hydrogen": -1.0, "oxygen": -0.5, "water": 1.0}),
    Reaction("CO + 0.5 O2 -> CO2", {"carbonmonoxide": -1.0, "oxygen": -0.5, "carbondioxide": 1.0}),
    Reaction(
        "2 Fe3O4 + H2 -> 3 Fe2O3 + H2O",
        {"Fe3O4": -2.0, "hydrogen": -1.0, "Fe2O3": 3.0, "water": 1.0},
    ),
    Reaction(
        "2 Fe3O4 + CO -> 3 Fe2O3 + CO2",
        {"Fe3O4": -2.0, "carbonmonoxide": -1.0, "Fe2O3": 3.0, "carbondioxide": 1.0},
    ),
)


def default_fprops_runner() -> Path:
    return THIS_DIR / "eqm_mu0_runner"


def default_reaktoro_runner() -> Path:
    return THIS_DIR / "reaktoro_mu0_runner.py"


def parse_temps(text: str) -> list[float]:
    vals: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            vals.append(float(tok))
    if not vals:
        raise ValueError("no temperatures provided")
    return vals


def query_fprops_mu0(runner: Path, tk: float) -> dict[str, float]:
    out = subprocess.check_output(
        [str(runner), SOURCE_MAP, f"{tk:.12g}", "100000", *SPECIES],
        text=True,
    )
    data = json.loads(out)
    return {s: float(v) for s, v in zip(SPECIES, data["mu0"])}


def query_reaktoro_mu0(runner: Path, tk: float) -> dict[str, float]:
    cmd = [
        "micromamba",
        "run",
        "-n",
        "base",
        "python",
        str(runner),
        "--db",
        "supcrt98",
        "--T",
        f"{tk:.12g}",
        "--P0",
        "100000",
        "--species",
        *SPECIES,
    ]
    for k, v in REAKTORO_NAME_MAP.items():
        cmd.extend(["--name-map", f"{k}={v}"])
    out = subprocess.check_output(cmd, text=True)
    data = json.loads(out)
    return {s: float(v) for s, v in zip(SPECIES, data["mu0"])}


def build_element_matrix() -> np.ndarray:
    a = np.zeros((len(ELEMENTS), len(SPECIES)), dtype=float)
    for j, s in enumerate(SPECIES):
        for i, e in enumerate(ELEMENTS):
            a[i, j] = FORMULAS[s].get(e, 0.0)
    return a


def reaction_vector(rxn: Reaction) -> np.ndarray:
    return np.array([rxn.nu.get(s, 0.0) for s in SPECIES], dtype=float)


def main() -> int:
    ap = argparse.ArgumentParser(description="Decompose mixed Hidayat-oxide + gas-basis mismatch into elemental shifts and balanced-reaction residuals.")
    ap.add_argument("--temps-c", default="350,450,550,650", help="Comma-separated temperatures in Celsius.")
    ap.add_argument("--fprops-runner", type=Path, default=default_fprops_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--reaktoro-runner", type=Path, default=default_reaktoro_runner(), help="Path to reaktoro_mu0_runner.py.")
    args = ap.parse_args()

    if not args.fprops_runner.exists():
        raise SystemExit(f"FPROPS runner not found: {args.fprops_runner}")
    if not args.reaktoro_runner.exists():
        raise SystemExit(f"Reaktoro runner not found: {args.reaktoro_runner}")

    temps_c = parse_temps(args.temps_c)
    a = build_element_matrix()

    print("Mixed oxide+gas basis decomposition")
    print("FPROPS source map:")
    print(f"  {SOURCE_MAP}")
    print("Reference comparison:")
    print("  Reaktoro SUPCRT98")
    print()

    for tc in temps_c:
        tk = tc + 273.15
        mu_a = query_fprops_mu0(args.fprops_runner, tk)
        mu_b = query_reaktoro_mu0(args.reaktoro_runner, tk)
        delta = np.array([mu_a[s] - mu_b[s] for s in SPECIES], dtype=float)
        lam, *_ = np.linalg.lstsq(a.T, delta, rcond=None)
        resid = delta - a.T.dot(lam)

        print(f"T = {tc:.2f} C")
        print(
            "  elemental shifts:"
            + "".join(f" {e}={lam[i]: .1f}" for i, e in enumerate(ELEMENTS))
            + " J/mol-atom"
        )
        print(
            "  species residuals:"
            + "".join(f" {s}={resid[i]: .1f}" for i, s in enumerate(SPECIES))
            + " J/mol"
        )
        for rxn in REACTIONS:
            nu = reaction_vector(rxn)
            ddg = float(nu.dot(delta))
            rdg = float(nu.dot(resid))
            print(f"  {rxn.name}: delta_dG={ddg/1000.0:+.3f} kJ/mol, residual_part={rdg/1000.0:+.3f} kJ/mol")
        print()

    print("Interpretation")
    print("- The elemental-shift fit is the best gauge-like alignment between the mixed FPROPS package and Reaktoro.")
    print("- Any balanced-reaction residual that remains after that fit is not fixable by simple reference-state shifts.")
    print("- Small gas-only residuals with larger oxide-gas residuals indicate the main disagreement sits in the oxide/gas model-family mismatch, not in the raw gas reference basis alone.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
