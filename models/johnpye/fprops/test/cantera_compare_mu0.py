#!/usr/bin/env python3
"""
Compare ideal-gas mu0(T,P0) between FPROPS and Cantera species by species.

How to run:
  1) Build the FPROPS mu0 runner:
       scons models/johnpye/fprops/test/eqm_mu0_runner -j4
  2) Run a comparison sweep:
       python3 models/johnpye/fprops/test/cantera_compare_mu0.py \
         --runner models/johnpye/fprops/test/eqm_mu0_runner \
         --source "Moran and Shapiro" \
         --temperatures 298,500,1000 \
         --p0 100000
"""

import argparse
import json
import math
import subprocess
import sys

import cantera as ct


FPROPS_TO_CT = {
    "hydrogen": "H2",
    "oxygen": "O2",
    "water": "H2O",
    "carbonmonoxide": "CO",
    "carbondioxide": "CO2",
    "nitrogen": "N2",
    "argon": "AR",
    "methane": "CH4",
    "nitric_oxide": "NO",
    "nitrogen_dioxide": "NO2",
    "nitrous_oxide": "N2O",
}

REACTIONS = {
    "h2o_dissociation": {"hydrogen": 1.0, "oxygen": 0.5, "water": -1.0},
    "co2_dissociation": {"carbonmonoxide": 1.0, "oxygen": 0.5, "carbondioxide": -1.0},
    "wgs": {"carbonmonoxide": 1.0, "water": 1.0, "carbondioxide": -1.0, "hydrogen": -1.0},
}


def parse_csv_floats(s):
    return [float(x.strip()) for x in s.split(",") if x.strip()]


def parse_csv_str(s):
    return [x.strip() for x in s.split(",") if x.strip()]


def cantera_mu0(gas, species_name, T, P0):
    gas.TPX = T, P0, f"{species_name}:1.0"
    k = gas.species_index(species_name)
    # Cantera chemical potentials are J/kmol; convert to J/mol.
    return gas.chemical_potentials[k] / 1000.0


def run_fprops_mu0(runner, source, T, P0, species):
    cmd = [runner, source, f"{T:.17g}", f"{P0:.17g}"] + species
    proc = subprocess.run(cmd, text=True, capture_output=True)
    if not proc.stdout.strip():
        raise RuntimeError(f"mu0 runner produced no output for T={T}")
    data = json.loads(proc.stdout.strip().splitlines()[-1])
    if len(data.get("mu0", [])) != len(species):
        raise RuntimeError("mu0 runner output length mismatch")
    return data["mu0"], proc.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", default="models/johnpye/fprops/test/eqm_mu0_runner")
    ap.add_argument("--source", default="Moran and Shapiro")
    ap.add_argument("--mechanism", default="gri30.yaml")
    ap.add_argument("--temperatures", default="298,500,700,1000,1200")
    ap.add_argument("--p0", type=float, default=100000.0)
    ap.add_argument(
        "--species",
        default="hydrogen,oxygen,water,carbonmonoxide,carbondioxide,nitrogen,argon,methane,nitric_oxide,nitrogen_dioxide,nitrous_oxide",
    )
    ap.add_argument("--fail-on-errors", action="store_true")
    args = ap.parse_args()

    temps = parse_csv_floats(args.temperatures)
    species = parse_csv_str(args.species)
    gas = ct.Solution(args.mechanism)
    R = 8.31446261815324

    print("T[K],species_fprops,species_cantera,mu0_fprops[J/mol],mu0_cantera[J/mol],delta[J/mol],delta_over_RT")
    nerr = 0
    mu_by_T = {}
    for T in temps:
        mu_by_T[T] = {"fprops": {}, "cantera": {}}
        try:
            mu_fprops, rc = run_fprops_mu0(args.runner, args.source, T, args.p0, species)
            if rc != 0:
                nerr += 1
        except Exception as e:
            sys.stderr.write(f"[WARN] FPROPS runner failed at T={T}: {e}\n")
            nerr += len(species)
            continue

        for sp_fprops, mu_f in zip(species, mu_fprops):
            sp_ct = FPROPS_TO_CT.get(sp_fprops, sp_fprops)
            if mu_f is None:
                nerr += 1
                print(f"{T:.6g},{sp_fprops},{sp_ct},nan,nan,nan,nan")
                continue
            mu_by_T[T]["fprops"][sp_fprops] = mu_f
            try:
                mu_c = cantera_mu0(gas, sp_ct, T, args.p0)
            except Exception as e:
                sys.stderr.write(f"[WARN] Cantera species lookup failed for {sp_fprops}->{sp_ct} at T={T}: {e}\n")
                nerr += 1
                print(f"{T:.6g},{sp_fprops},{sp_ct},{mu_f},nan,nan,nan")
                continue
            mu_by_T[T]["cantera"][sp_fprops] = mu_c

            delta = mu_f - mu_c
            d_rt = delta / (R * T)
            print(f"{T:.6g},{sp_fprops},{sp_ct},{mu_f:.12g},{mu_c:.12g},{delta:.12g},{d_rt:.12g}")

    print("")
    print("reaction,T[K],deltaG_fprops[J/mol],deltaG_cantera[J/mol],delta_log10K")
    for T in temps:
        for rname, nu in REACTIONS.items():
            ok = True
            dg_f = 0.0
            dg_c = 0.0
            for sp, coeff in nu.items():
                if sp not in mu_by_T[T]["fprops"] or sp not in mu_by_T[T]["cantera"]:
                    ok = False
                    break
                dg_f += coeff * mu_by_T[T]["fprops"][sp]
                dg_c += coeff * mu_by_T[T]["cantera"][sp]
            if not ok:
                continue
            dlog10k = -(dg_f - dg_c) / (R * T * math.log(10.0))
            print(f"{rname},{T:.6g},{dg_f:.12g},{dg_c:.12g},{dlog10k:.12g}")

    if nerr:
        sys.stderr.write(f"[INFO] completed with {nerr} mu0 issues\n")
        if args.fail_on_errors:
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
