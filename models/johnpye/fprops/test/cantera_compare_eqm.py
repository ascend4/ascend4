#!/usr/bin/env python3
"""
Compare selected ideal-gas equilibrium cases between FPROPS and Cantera.

How to run:
  1) Build the FPROPS runner:
       scons models/johnpye/fprops/test/eqm_case_runner -j4
  2) Run a comparison sweep:
       python3 models/johnpye/fprops/test/cantera_compare_eqm.py \
         --runner models/johnpye/fprops/test/eqm_case_runner \
         --source "Moran and Shapiro" \
         --algorithm auto_nullspace \
         --temperatures 500,1000 \
         --pressures 100000,1000000
  3) Optional: fail process on any failed point:
       ... --fail-on-errors
"""

import argparse
import json
import math
import subprocess
import sys

import cantera as ct


CASE_DEFS = {
    "h2o_dissociation": {
        "species_fprops": ["hydrogen", "oxygen", "water"],
        "species_ct": ["H2", "O2", "H2O"],
        "init_moles_ct": {"H2O": 1.0},
        "nu": [1.0, 0.5, -1.0],
    },
    "co2_dissociation": {
        "species_fprops": ["carbonmonoxide", "oxygen", "carbondioxide"],
        "species_ct": ["CO", "O2", "CO2"],
        "init_moles_ct": {"CO2": 1.0},
        "nu": [1.0, 0.5, -1.0],
    },
    "wgs": {
        "species_fprops": ["carbonmonoxide", "water", "carbondioxide", "hydrogen"],
        "species_ct": ["CO", "H2O", "CO2", "H2"],
        "init_moles_ct": {"CO": 1.0, "H2O": 1.0},
        "nu": [1.0, 1.0, -1.0, -1.0],
    },
    "wgs_inert_n2": {
        "species_fprops": ["carbonmonoxide", "water", "carbondioxide", "hydrogen", "nitrogen"],
        "species_ct": ["CO", "H2O", "CO2", "H2", "N2"],
        "init_moles_ct": {"CO": 1.0, "H2O": 1.0, "N2": 1.0},
        "nu": [1.0, 1.0, -1.0, -1.0, 0.0],
    },
    "co_co2_h2o_h2_o2": {
        "species_fprops": ["carbonmonoxide", "carbondioxide", "water", "hydrogen", "oxygen"],
        "species_ct": ["CO", "CO2", "H2O", "H2", "O2"],
        "init_moles_ct": {"CO2": 1.0, "H2": 1.0},
        "nu": [1.0, -1.0, 1.0, -1.0, 0.0],
    },
}

P0 = 1e5


def parse_csv_floats(s):
    out = []
    for x in s.split(","):
        x = x.strip()
        if not x:
            continue
        out.append(float(x))
    return out


def parse_csv_cases(s):
    out = []
    for x in s.split(","):
        x = x.strip()
        if not x:
            continue
        if x not in CASE_DEFS:
            raise ValueError(f"Unknown case '{x}'")
        out.append(x)
    return out


def build_subset_gas(mechanism, species_names):
    all_species = ct.Species.list_from_file(mechanism)
    wanted = set(species_names)
    selected = [sp for sp in all_species if sp.name in wanted]
    found = {sp.name for sp in selected}
    missing = [sp for sp in species_names if sp not in found]
    if missing:
        raise RuntimeError(f"Missing species in mechanism '{mechanism}': {missing}")
    return ct.Solution(thermo="ideal-gas", species=selected)


def composition_string(init_moles):
    tot = sum(init_moles.values())
    if not (tot > 0):
        raise ValueError("non-positive initial moles")
    parts = []
    for k, v in init_moles.items():
        parts.append(f"{k}:{v / tot:.17g}")
    return ", ".join(parts)


def log10K_from_y(y, nu, p):
    s = 0.0
    for yi, nui in zip(y, nu):
        if yi <= 0.0:
            return math.nan
        a = yi * p / P0
        if a <= 0.0:
            return math.nan
        s += nui * math.log10(a)
    return s


def run_fprops_case(runner, case_name, T, P, algorithm, source):
    cmd = [
        runner, case_name, f"{T:.17g}", f"{P:.17g}", algorithm, source
    ]
    proc = subprocess.run(cmd, text=True, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"FPROPS runner failed ({proc.returncode}) for {case_name} T={T} P={P}\n{proc.stderr}"
        )
    txt = proc.stdout.strip().splitlines()
    if not txt:
        raise RuntimeError("FPROPS runner returned no output")
    return json.loads(txt[-1])


def run_cantera_case(gas, case, T, P, solver):
    gas.TPX = T, P, composition_string(case["init_moles_ct"])
    gas.equilibrate("TP", solver=solver, rtol=1e-12, max_steps=20000, max_iter=500)
    y = [gas[sp].X[0] for sp in case["species_ct"]]
    return {
        "y": y,
        "log10K": log10K_from_y(y, case["nu"], P),
    }


def max_abs_diff(a, b):
    return max(abs(x - y) for x, y in zip(a, b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", default="models/johnpye/fprops/test/eqm_case_runner")
    ap.add_argument("--source", default="Moran and Shapiro")
    ap.add_argument("--algorithm", default="auto_nullspace")
    ap.add_argument("--mechanism", default="gri30.yaml")
    ap.add_argument("--solver", default="gibbs", choices=["gibbs", "vcs", "auto"])
    ap.add_argument("--temperatures", default="298,500,700,1000,1200")
    ap.add_argument("--pressures", default="100000,101325,1000000")
    ap.add_argument(
        "--cases",
        default="h2o_dissociation,co2_dissociation,wgs,wgs_inert_n2,co_co2_h2o_h2_o2",
    )
    ap.add_argument("--fail-on-errors", action="store_true")
    args = ap.parse_args()

    temps = parse_csv_floats(args.temperatures)
    press = parse_csv_floats(args.pressures)
    cases = parse_csv_cases(args.cases)

    gases = {}
    for cname in cases:
        gases[cname] = build_subset_gas(args.mechanism, CASE_DEFS[cname]["species_ct"])

    print(
        "case,T[K],P[Pa],status_fprops,log10K_fprops,log10K_cantera,delta_log10K,max_dy_species"
    )
    nfail = 0
    for cname in cases:
        case = CASE_DEFS[cname]
        gas = gases[cname]
        for T in temps:
            for P in press:
                status = "ERR"
                logkf = math.nan
                logkc = math.nan
                dlogk = math.nan
                maxdy = math.nan
                try:
                    f = run_fprops_case(
                        args.runner, cname, T, P, args.algorithm, args.source
                    )
                    status = str(f["status"])
                    if f.get("y") is not None:
                        yf = f["y"]
                        logkf = f.get("log10K", math.nan)
                    else:
                        yf = None
                    c = run_cantera_case(gas, case, T, P, args.solver)
                    yc = c["y"]
                    logkc = c["log10K"]
                    if yf is not None and all(isinstance(v, (int, float)) for v in yf):
                        maxdy = max_abs_diff(yf, yc)
                    if math.isfinite(logkf) and math.isfinite(logkc):
                        dlogk = logkf - logkc
                    if not (f["status"] in (0, 1, 6)):
                        nfail += 1
                except Exception as e:
                    nfail += 1
                    sys.stderr.write(f"[WARN] {cname} T={T} P={P}: {e}\n")

                print(
                    f"{cname},{T:.6g},{P:.6g},{status},"
                    f"{logkf if math.isfinite(logkf) else 'nan'},"
                    f"{logkc if math.isfinite(logkc) else 'nan'},"
                    f"{dlogk if math.isfinite(dlogk) else 'nan'},"
                    f"{maxdy if math.isfinite(maxdy) else 'nan'}"
                )

    if nfail:
        sys.stderr.write(f"[INFO] completed with {nfail} failed points\n")
        if args.fail_on_errors:
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
