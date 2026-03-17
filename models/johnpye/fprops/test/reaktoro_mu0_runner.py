#!/usr/bin/env python3
"""
Evaluate standard chemical potentials mu0(T,P0) from Reaktoro for a species list.

This script is intended to run inside an environment where `reaktoro` is
installed. It prints JSON so it can be called by other tooling.

Example:
  python3 models/johnpye/fprops/test/reaktoro_mu0_runner.py \
    --db supcrt98 \
    --T 1073.15 --P0 100000 \
    --species Ni NiO hydrogen water \
    --name-map Ni=Nickel \
    --name-map NiO=Bunsenite \
    --name-map hydrogen='H2(g)' \
    --name-map water='H2O(g)'
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from typing import Dict


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


def parse_name_map(entries) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for e in entries:
        if "=" not in e:
            raise ValueError(f"Mapping must be 'name=value', got '{e}'")
        k, v = e.split("=", 1)
        out[k.strip()] = v.strip()
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Reaktoro mu0 JSON runner.")
    ap.add_argument("--db", default="supcrt98", help="Reaktoro database name/path for SupcrtDatabase")
    ap.add_argument("--T", type=float, required=True, help="Temperature [K]")
    ap.add_argument("--P0", type=float, required=True, help="Pressure [Pa]")
    ap.add_argument("--species", nargs="+", required=True, help="Species keys")
    ap.add_argument(
        "--name-map",
        action="append",
        default=[],
        help="Mapping key=value to Reaktoro species name. Can repeat.",
    )
    args = ap.parse_args()

    if not (args.T > 0.0 and args.P0 > 0.0):
        print("Invalid T/P0", file=sys.stderr)
        return 2

    try:
        from reaktoro import SupcrtDatabase
    except Exception as e:
        print(f"Failed to import reaktoro: {e}", file=sys.stderr)
        return 3

    rmap = dict(DEFAULT_REAKTORO_MAP)
    rmap.update(parse_name_map(args.name_map))

    db = SupcrtDatabase(args.db)
    tc = args.T - 273.15
    pbar = args.P0 / 1e5

    mu0 = []
    for s in args.species:
        rname = rmap.get(s, s)
        try:
            sp = db.species(rname)
            pr = sp.props(tc, "C", pbar, "bar")
            g0 = float(pr.G0)
            if not math.isfinite(g0):
                raise ValueError("non-finite G0")
            mu0.append(g0)
        except Exception:
            mu0.append(None)

    print(
        json.dumps(
            {
                "db": args.db,
                "T": args.T,
                "P0": args.P0,
                "species": args.species,
                "mu0": mu0,
            }
        )
    )
    return 0 if all(v is not None for v in mu0) else 1


if __name__ == "__main__":
    raise SystemExit(main())
