#!/usr/bin/env python3
"""
First-pass Fe-O-H comparison between:

  - FPROPS Tier 3 boundaries
  - Reaktoro stoichiometric surrogates

This is not a final validation tool. It compares:

  FPROPS Fe|wustite          vs Reaktoro Fe|FeO       when available
  FPROPS wustite|spinel      vs Reaktoro FeO|Magnetite

using Reaktoro gas standard-state data for H2(g) and H2O(g).

Run inside a Reaktoro-capable environment, for example:

    eval "$(micromamba shell hook --shell bash)"
    micromamba activate
    python3 models/johnpye/fprops/test/feoh_reaktoro_compare.py
"""

from __future__ import annotations

import argparse
import math
import sys

from feoh_hydrogen_boundary import (
    R,
    gas_ratio_logs,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
)


def parse_temps_c(text: str) -> list[float]:
    out: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            out.append(float(tok))
    if not out:
        raise ValueError("no temperatures provided")
    return out


def require_reaktoro():
    try:
        from reaktoro import PhreeqcDatabase, SupcrtDatabase
    except Exception as e:
        print(f"Failed to import reaktoro: {e}", file=sys.stderr)
        raise SystemExit(3)
    return PhreeqcDatabase, SupcrtDatabase


def reaktoro_mu0(db, name: str, tk: float) -> float:
    tc = tk - 273.15
    pbar = 1.0
    sp = db.species(name)
    return float(sp.props(tc, "C", pbar, "bar").G0)


def reaktoro_log10_h2o_h2_fe_feo(db, tk: float) -> float:
    mu_fe = reaktoro_mu0(db, "Fe", tk)
    mu_feo = reaktoro_mu0(db, "FeO", tk)
    mu_h2 = reaktoro_mu0(db, "H2(g)", tk)
    mu_h2o = reaktoro_mu0(db, "H2O(g)", tk)
    dg = (mu_fe + mu_h2o) - (mu_feo + mu_h2)
    return -dg / (R * tk * math.log(10.0))


def reaktoro_log10_h2o_h2_feo_magnetite(db, tk: float) -> float:
    mu_feo = reaktoro_mu0(db, "Ferrous-Oxide", tk)
    mu_mag = reaktoro_mu0(db, "Magnetite", tk)
    mu_h2 = reaktoro_mu0(db, "H2(g)", tk)
    mu_h2o = reaktoro_mu0(db, "H2O(g)", tk)
    dg = (3.0 * mu_feo + mu_h2o) - (mu_mag + mu_h2)
    return -dg / (R * tk * math.log(10.0))


def fprops_log10_h2o_h2_fe_wustite_with_reaktoro_gas(db, tk: float) -> float:
    _x, lam_o = oxygen_potential_at_fe_wustite_boundary(tk)
    mu_h2 = reaktoro_mu0(db, "H2(g)", tk)
    mu_h2o = reaktoro_mu0(db, "H2O(g)", tk)
    log10_h2o_h2, _ = gas_ratio_logs(mu_h2, mu_h2o, lam_o, tk)
    return log10_h2o_h2


def fprops_log10_h2o_h2_wustite_spinel_with_reaktoro_gas(db, tk: float) -> float:
    _x, lam_o = oxygen_potential_at_wustite_spinel_boundary(tk)
    mu_h2 = reaktoro_mu0(db, "H2(g)", tk)
    mu_h2o = reaktoro_mu0(db, "H2O(g)", tk)
    log10_h2o_h2, _ = gas_ratio_logs(mu_h2, mu_h2o, lam_o, tk)
    return log10_h2o_h2


def main() -> int:
    ap = argparse.ArgumentParser(description="First-pass FPROPS vs Reaktoro Fe-O-H comparison.")
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000,1100,1200",
        help="Comma-separated temperatures in Celsius.",
    )
    args = ap.parse_args()

    PhreeqcDatabase, SupcrtDatabase = require_reaktoro()
    db_fe = PhreeqcDatabase("llnl.dat")
    db_oxide = SupcrtDatabase("supcrt98")
    temps_c = parse_temps_c(args.temps_c)

    print("FPROPS vs Reaktoro first-pass Fe-O-H comparison")
    print('Reaktoro oxide-side source: SupcrtDatabase("supcrt98")')
    print('Reaktoro Fe-side source: PhreeqcDatabase("llnl.dat") when temperature <= 350 C')
    print()
    print("Fe-side comparison: FPROPS Fe|wustite vs Reaktoro Fe|FeO")
    print(f"{'T[C]':>6} {'FPROPS':>12} {'Reaktoro':>12} {'delta':>12} {'status':>14}")
    for tc in temps_c:
        tk = tc + 273.15
        if tc > 350.0:
            print(f"{tc:6.0f} {'n/a':>12} {'n/a':>12} {'n/a':>12} {'llnl>350C':>14}")
            continue
        fprops = fprops_log10_h2o_h2_fe_wustite_with_reaktoro_gas(db_fe, tk)
        reaktoro = reaktoro_log10_h2o_h2_fe_feo(db_fe, tk)
        print(f"{tc:6.0f} {fprops:12.6f} {reaktoro:12.6f} {fprops - reaktoro:12.6f} {'ok':>14}")

    print()
    print("Oxide-side comparison: FPROPS wustite|spinel vs Reaktoro FeO|Magnetite")
    print(f"{'T[C]':>6} {'FPROPS':>12} {'Reaktoro':>12} {'delta':>12}")
    for tc in temps_c:
        tk = tc + 273.15
        fprops = fprops_log10_h2o_h2_wustite_spinel_with_reaktoro_gas(db_oxide, tk)
        reaktoro = reaktoro_log10_h2o_h2_feo_magnetite(db_oxide, tk)
        print(f"{tc:6.0f} {fprops:12.6f} {reaktoro:12.6f} {fprops - reaktoro:12.6f}")

    print()
    print("Interpretation")
    print("- A high-temperature Fe-side external comparison is not available from the installed Reaktoro databases here: supcrt98 lacks metallic Fe, while llnl.dat is only usable up to about 350 C.")
    print("- The oxide-side comparison is the best direct Reaktoro checkpoint currently available.")
    print("- It is still only a first surrogate because FPROPS uses nonstoichiometric wustite and spinel, while the Reaktoro side uses stoichiometric FeO and Magnetite.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
