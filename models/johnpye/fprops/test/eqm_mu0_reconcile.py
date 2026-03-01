#!/usr/bin/env python3
"""
Element-potential reconciliation diagnostic for chemical-potential sources.

This script quantifies how much of a source mismatch can be explained as a
reference/basis shift of the form

    delta_mu_i(T) ~ sum_e a[e,i] * lambda_e(T)

where:
  - delta_mu_i(T) = mu_i^A(T) - mu_i^B(T)
  - a[e,i] is the element matrix (elements x species)
  - lambda_e(T) are fitted elemental potential shifts.

Outputs per temperature:
  - species-level delta_mu
  - fitted lambda_e
  - residuals r_i = delta_mu_i - (A^T lambda)_i
  - reaction mismatch split:
      delta_dG = nu . delta_mu
      residual_part = nu . r
    (the A^T lambda part does not affect balanced reactions).

How to run:
  1) Build FPROPS mu0 runner:
       scons models/johnpye/fprops/test/eqm_mu0_runner -j4

  2) Compare two FPROPS sources (default Ni/NiO/H2/H2O species):
       python3 models/johnpye/fprops/test/eqm_mu0_reconcile.py \
         --a 'fprops:Moran and Shapiro' \
         --b 'fprops:oecd_nea_tdb_vol6_nickel'

  3) Compare FPROPS vs Reaktoro (if installed in this Python env):
       python3 models/johnpye/fprops/test/eqm_mu0_reconcile.py \
         --a 'fprops:Moran and Shapiro' \
         --b 'reaktoro:supcrt98'
"""

from __future__ import annotations

import argparse
import json
import math
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

import numpy as np


DEFAULT_SPECIES = ["Ni", "NiO", "hydrogen", "water"]
DEFAULT_REACTION_NU = {
    "Ni": 1.0,
    "NiO": -1.0,
    "hydrogen": -1.0,
    "water": 1.0,
}

DEFAULT_FORMULA_MAP = {
    "Ni": "Ni",
    "NiO": "NiO",
    "Fe": "Fe",
    "FeO": "FeO",
    "Fe2O3": "Fe2O3",
    "Fe3O4": "Fe3O4",
    "hydrogen": "H2",
    "oxygen": "O2",
    "water": "H2O",
    "carbondioxide": "CO2",
    "carbonmonoxide": "CO",
    "methane": "CH4",
    "nitrogen": "N2",
}

DEFAULT_REAKTORO_MAP = {
    "Ni": "Nickel",
    "NiO": "Bunsenite",
    "hydrogen": "H2(g)",
    "water": "H2O(g)",
    "oxygen": "O2(g)",
    "nitrogen": "N2(g)",
    "carbondioxide": "CO2(g)",
    "carbonmonoxide": "CO(g)",
    "methane": "CH4(g)",
}


@dataclass(frozen=True)
class Provider:
    kind: str
    source: str


def parse_provider(s: str) -> Provider:
    if ":" not in s:
        raise ValueError(f"Provider must be '<kind>:<source>', got '{s}'")
    kind, source = s.split(":", 1)
    kind = kind.strip().lower()
    source = source.strip()
    if kind not in ("fprops", "reaktoro"):
        raise ValueError(f"Unsupported provider kind '{kind}'")
    if not source:
        raise ValueError("Provider source cannot be empty")
    return Provider(kind=kind, source=source)


def parse_formula(formula: str) -> Dict[str, float]:
    """
    Parse simple chemical formulae (e.g., H2O, Fe2O3, CH4, NiO).
    """
    token_re = re.compile(r"([A-Z][a-z]?)([0-9]*\.?[0-9]*)")
    out: Dict[str, float] = {}
    pos = 0
    for m in token_re.finditer(formula):
        if m.start() != pos:
            raise ValueError(f"Unsupported formula syntax near '{formula[pos:]}'")
        elem = m.group(1)
        cnt = float(m.group(2)) if m.group(2) else 1.0
        out[elem] = out.get(elem, 0.0) + cnt
        pos = m.end()
    if pos != len(formula):
        raise ValueError(f"Unsupported trailing formula syntax near '{formula[pos:]}'")
    if not out:
        raise ValueError(f"Empty/invalid formula '{formula}'")
    return out


def parse_name_map(entries: Iterable[str]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for e in entries:
        if "=" not in e:
            raise ValueError(f"Mapping must be 'name=value', got '{e}'")
        k, v = e.split("=", 1)
        out[k.strip()] = v.strip()
    return out


def parse_temps(text: str) -> List[float]:
    vals: List[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if not tok:
            continue
        vals.append(float(tok))
    if not vals:
        raise ValueError("No temperatures provided")
    return vals


def query_fprops_mu0(runner: Path, source: str, tk: float, p0: float, species: List[str]) -> Dict[str, float]:
    cmd = [str(runner), source, f"{tk:.12g}", f"{p0:.12g}", *species]
    out = subprocess.check_output(cmd, text=True)
    data = json.loads(out)
    mu = data["mu0"]
    if len(mu) != len(species):
        raise RuntimeError("Unexpected mu0 length from eqm_mu0_runner")
    out_map: Dict[str, float] = {}
    for s, v in zip(species, mu):
        if v is None or not math.isfinite(float(v)):
            raise RuntimeError(f"FPROPS mu0 missing/non-finite for '{s}' at T={tk:.2f} K")
        out_map[s] = float(v)
    return out_map


def query_reaktoro_mu0(db_name: str, tk: float, p0: float, species: List[str], rmap: Dict[str, str]) -> Dict[str, float]:
    try:
        from reaktoro import SupcrtDatabase
    except Exception as e:
        raise RuntimeError(
            "Reaktoro provider requested but module import failed. "
            "Install/use an environment with `reaktoro`."
        ) from e

    db = SupcrtDatabase(db_name)
    tc = tk - 273.15
    pbar = p0 / 1e5
    out_map: Dict[str, float] = {}
    for s in species:
        rname = rmap.get(s, s)
        sp = db.species(rname)
        pr = sp.props(tc, "C", pbar, "bar")
        g0 = float(pr.G0)
        if not math.isfinite(g0):
            raise RuntimeError(f"Reaktoro G0 non-finite for '{s}' ({rname}) at T={tk:.2f} K")
        out_map[s] = g0
    return out_map


def query_provider_mu0(
    provider: Provider,
    runner: Path,
    tk: float,
    p0: float,
    species: List[str],
    reaktoro_name_map: Dict[str, str],
) -> Dict[str, float]:
    if provider.kind == "fprops":
        return query_fprops_mu0(runner, provider.source, tk, p0, species)
    if provider.kind == "reaktoro":
        return query_reaktoro_mu0(provider.source, tk, p0, species, reaktoro_name_map)
    raise RuntimeError(f"Unhandled provider kind '{provider.kind}'")


def build_formula_map(species: List[str], user_formula_map: Dict[str, str]) -> Dict[str, Dict[str, float]]:
    out: Dict[str, Dict[str, float]] = {}
    for s in species:
        f = user_formula_map.get(s, DEFAULT_FORMULA_MAP.get(s, s))
        out[s] = parse_formula(f)
    return out


def build_element_matrix(species: List[str], comp: Dict[str, Dict[str, float]], elements: List[str]) -> np.ndarray:
    a = np.zeros((len(elements), len(species)), dtype=float)
    for j, s in enumerate(species):
        for i, e in enumerate(elements):
            a[i, j] = comp[s].get(e, 0.0)
    return a


def infer_elements(comp: Dict[str, Dict[str, float]]) -> List[str]:
    elems = sorted({e for v in comp.values() for e in v})
    return elems


def reaction_vector(species: List[str], user_nu: Dict[str, float]) -> np.ndarray:
    return np.array([user_nu.get(s, 0.0) for s in species], dtype=float)


def fmt_vec(v: np.ndarray, names: List[str], unit: str) -> str:
    parts = []
    for n, x in zip(names, v):
        parts.append(f"{n}={x: .6e} {unit}")
    return ", ".join(parts)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Fit elemental-potential reconciliation (A^T lambda) to source delta_mu."
    )
    ap.add_argument("--a", default="fprops:Moran and Shapiro", help="Provider A, format kind:source")
    ap.add_argument("--b", default="fprops:oecd_nea_tdb_vol6_nickel", help="Provider B, format kind:source")
    ap.add_argument("--runner", type=Path, default=Path(__file__).resolve().parent / "eqm_mu0_runner")
    ap.add_argument("--p0", type=float, default=1e5, help="Standard pressure [Pa]")
    ap.add_argument(
        "--temps-c",
        default="400,600,800,900,1000,1200",
        help="Comma-separated temperatures [degC]",
    )
    ap.add_argument(
        "--species",
        default=",".join(DEFAULT_SPECIES),
        help="Comma-separated FPROPS species list.",
    )
    ap.add_argument(
        "--formula",
        action="append",
        default=[],
        help="Species formula override, can repeat: --formula 'hydrogen=H2'",
    )
    ap.add_argument(
        "--elements",
        default="",
        help="Comma-separated element order. Default: inferred from formulae.",
    )
    ap.add_argument(
        "--nu",
        action="append",
        default=[],
        help="Reaction stoich override, can repeat: --nu 'Ni=1' --nu 'NiO=-1' ...",
    )
    ap.add_argument(
        "--reaktoro-name",
        action="append",
        default=[],
        help="Reaktoro species mapping, can repeat: --reaktoro-name 'Ni=Nickel'",
    )
    ap.add_argument(
        "--strict-resid",
        type=float,
        default=0.0,
        help="If >0, nonzero exit when residual RMS exceeds this [J/mol] at any T.",
    )
    args = ap.parse_args()

    a = parse_provider(args.a)
    b = parse_provider(args.b)
    temps_k = [t + 273.15 for t in parse_temps(args.temps_c)]
    species = [s.strip() for s in args.species.split(",") if s.strip()]
    if not species:
        raise ValueError("No species provided")

    formula_map = dict(DEFAULT_FORMULA_MAP)
    formula_map.update(parse_name_map(args.formula))
    comp = build_formula_map(species, formula_map)
    if args.elements.strip():
        elements = [e.strip() for e in args.elements.split(",") if e.strip()]
    else:
        elements = infer_elements(comp)
    if not elements:
        raise ValueError("No elements defined/inferred")

    user_nu = dict(DEFAULT_REACTION_NU)
    user_nu.update({k: float(v) for k, v in parse_name_map(args.nu).items()})
    nu = reaction_vector(species, user_nu)
    has_reaction = np.any(np.abs(nu) > 0.0)

    rmap = dict(DEFAULT_REAKTORO_MAP)
    rmap.update(parse_name_map(args.reaktoro_name))

    if a.kind == "fprops" or b.kind == "fprops":
        if not args.runner.exists():
            print(f"FPROPS runner not found: {args.runner}", file=sys.stderr)
            return 2

    amat = build_element_matrix(species, comp, elements)  # (ne, ns)
    bmat = amat.T  # (ns, ne)

    print(f"Provider A: {a.kind}:{a.source}")
    print(f"Provider B: {b.kind}:{b.source}")
    print(f"Species: {', '.join(species)}")
    print(f"Elements: {', '.join(elements)}")
    print()

    worst_rms = 0.0
    for tk in temps_k:
        mu_a = query_provider_mu0(a, args.runner, tk, args.p0, species, rmap)
        mu_b = query_provider_mu0(b, args.runner, tk, args.p0, species, rmap)
        va = np.array([mu_a[s] for s in species], dtype=float)
        vb = np.array([mu_b[s] for s in species], dtype=float)
        delta = va - vb

        lam, *_ = np.linalg.lstsq(bmat, delta, rcond=None)
        delta_hat = bmat @ lam
        resid = delta - delta_hat

        rms = float(np.sqrt(np.mean(resid * resid)))
        max_abs = float(np.max(np.abs(resid)))
        worst_rms = max(worst_rms, rms)
        explain = float(np.linalg.norm(delta_hat) / np.linalg.norm(delta)) if np.linalg.norm(delta) > 0 else 0.0

        print(f"T = {tk:.2f} K ({tk - 273.15:.2f} C)")
        print("  delta_mu(A-B): " + fmt_vec(delta, species, "J/mol"))
        print("  lambda fit   : " + fmt_vec(lam, elements, "J/mol-atom"))
        print("  residual     : " + fmt_vec(resid, species, "J/mol"))
        print(f"  residual RMS = {rms:.6e} J/mol, max|r| = {max_abs:.6e} J/mol")
        print(f"  ||A^T lambda|| / ||delta_mu|| = {explain:.6f}")

        if has_reaction:
            dg_a = float(np.dot(nu, va))
            dg_b = float(np.dot(nu, vb))
            ddg = dg_a - dg_b
            ddg_resid = float(np.dot(nu, resid))
            print(f"  reaction delta_dG(A-B) = {ddg:.6e} J/mol")
            print(f"  reaction residual part = {ddg_resid:.6e} J/mol")
        print()

    if args.strict_resid > 0.0 and worst_rms > args.strict_resid:
        print(
            f"FAILED: worst residual RMS {worst_rms:.6e} J/mol exceeds "
            f"strict limit {args.strict_resid:.6e} J/mol",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
