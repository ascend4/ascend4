#!/usr/bin/env python3
"""
Low-temperature mixed-gas equilibrium regression for FPROPS reduced solvers.

How to run:
  1) Build runner:
       scons models/johnpye/fprops/test/eqm_case_runner -j4
  2) Run regression:
       python3 models/johnpye/fprops/test/eqm_lowt_regression.py
  3) Enable active-set trace while running:
       FPROPS_EQM_ACTIVESET_TRACE=1 \
       python3 models/johnpye/fprops/test/eqm_lowt_regression.py
"""

import argparse
import json
import math
import subprocess
import sys

import cantera as ct

P0 = 1e5
CASE = "co_co2_h2o_h2_o2"
SPECIES_CT = ["CO", "CO2", "H2O", "H2", "O2"]
INIT_CT = {"CO2": 1.0, "H2": 1.0}
NU_WGS = [1.0, -1.0, 1.0, -1.0, 0.0]


def parse_csv_floats(s):
    return [float(x.strip()) for x in s.split(",") if x.strip()]


def parse_csv_text(s):
    return [x.strip() for x in s.split(",") if x.strip()]


def composition_string(init_moles):
    tot = sum(init_moles.values())
    if not (tot > 0):
        raise ValueError("non-positive initial moles")
    return ", ".join(f"{k}:{v / tot:.17g}" for k, v in init_moles.items())


def build_gas(mechanism):
    all_species = ct.Species.list_from_file(mechanism)
    wanted = set(SPECIES_CT)
    selected = [sp for sp in all_species if sp.name in wanted]
    found = {sp.name for sp in selected}
    missing = [sp for sp in SPECIES_CT if sp not in found]
    if missing:
        raise RuntimeError(f"Missing species in mechanism '{mechanism}': {missing}")
    return ct.Solution(thermo="ideal-gas", species=selected)


def log10k_from_y(y, nu, p):
    s = 0.0
    for yi, nui in zip(y, nu):
        if yi <= 0.0:
            return math.nan
        a = yi * p / P0
        if a <= 0.0:
            return math.nan
        s += nui * math.log10(a)
    return s


def run_fprops_case(runner, algorithm, source, t, p):
    cmd = [runner, CASE, f"{t:.17g}", f"{p:.17g}", algorithm, source]
    proc = subprocess.run(cmd, text=True, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"runner failed ({proc.returncode}) at T={t} P={p} alg={algorithm}\n{proc.stderr}"
        )
    lines = proc.stdout.strip().splitlines()
    if not lines:
        raise RuntimeError("runner returned no output")
    return json.loads(lines[-1])


def run_cantera_case(gas, t, p):
    gas.TPX = t, p, composition_string(INIT_CT)
    gas.equilibrate("TP", solver="gibbs", rtol=1e-12, max_steps=20000, max_iter=500)
    y = [gas[s].X[0] for s in SPECIES_CT]
    return {
        "y": y,
        "log10k": log10k_from_y(y, NU_WGS, p),
    }


def max_absdiff(a, b):
    return max(abs(x - y) for x, y in zip(a, b))


def is_ok_status(status):
    return status in (0, 1, 6)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", default="models/johnpye/fprops/test/eqm_case_runner")
    ap.add_argument("--source", default="Moran and Shapiro")
    ap.add_argument("--mechanism", default="gri30.yaml")
    ap.add_argument("--algorithms", default="reduced,auto_reduced")
    ap.add_argument("--temperatures", default="340,320,310,300,298")
    ap.add_argument("--pressure", type=float, default=100000.0)
    ap.add_argument("--max-dlog10k", type=float, default=5e-3)
    ap.add_argument("--max-dy", type=float, default=1e-4)
    ap.add_argument("--max-y-o2", type=float, default=1e-20)
    args = ap.parse_args()

    algs = parse_csv_text(args.algorithms)
    temps = parse_csv_floats(args.temperatures)
    gas = build_gas(args.mechanism)

    nfail = 0
    print("algorithm,T[K],status,log10K_fprops,log10K_cantera,delta_log10K,max_dy,y_o2")

    for alg in algs:
        logk_by_t = {}
        for t in temps:
            fp = run_fprops_case(args.runner, alg, args.source, t, args.pressure)
            ctres = run_cantera_case(gas, t, args.pressure)

            status = fp["status"]
            yf = fp.get("y")
            logkf = fp.get("log10K", math.nan)
            yc = ctres["y"]
            logkc = ctres["log10k"]

            dy = math.nan
            dlogk = math.nan
            y_o2 = math.nan

            if yf is not None:
                dy = max_absdiff(yf, yc)
                dlogk = logkf - logkc
                try:
                    idx_o2 = fp["species"].index("oxygen")
                    y_o2 = yf[idx_o2]
                except Exception:
                    y_o2 = math.nan

            print(
                f"{alg},{t:.6g},{status},"
                f"{logkf if math.isfinite(logkf) else 'nan'},"
                f"{logkc if math.isfinite(logkc) else 'nan'},"
                f"{dlogk if math.isfinite(dlogk) else 'nan'},"
                f"{dy if math.isfinite(dy) else 'nan'},"
                f"{y_o2 if math.isfinite(y_o2) else 'nan'}"
            )

            if not is_ok_status(status):
                sys.stderr.write(f"[FAIL] {alg} T={t}: status={status}\n")
                nfail += 1
                continue
            if not (math.isfinite(dlogk) and abs(dlogk) <= args.max_dlog10k):
                sys.stderr.write(
                    f"[FAIL] {alg} T={t}: |dlog10K|={abs(dlogk) if math.isfinite(dlogk) else 'nan'}\n"
                )
                nfail += 1
            if not (math.isfinite(dy) and dy <= args.max_dy):
                sys.stderr.write(
                    f"[FAIL] {alg} T={t}: max|dy|={dy if math.isfinite(dy) else 'nan'}\n"
                )
                nfail += 1
            if not (math.isfinite(y_o2) and y_o2 <= args.max_y_o2):
                sys.stderr.write(
                    f"[FAIL] {alg} T={t}: y_O2={y_o2 if math.isfinite(y_o2) else 'nan'} exceeds {args.max_y_o2}\n"
                )
                nfail += 1

            logk_by_t[t] = logkf

        # Monotonic trend: as T decreases, log10K becomes more negative.
        t_desc = sorted(logk_by_t.keys(), reverse=True)
        for i in range(1, len(t_desc)):
            t_hi = t_desc[i - 1]
            t_lo = t_desc[i]
            if not (logk_by_t[t_lo] <= logk_by_t[t_hi] + 1e-10):
                sys.stderr.write(
                    f"[FAIL] {alg}: non-monotonic log10K ({t_hi}->{t_lo}): "
                    f"{logk_by_t[t_hi]} -> {logk_by_t[t_lo]}\n"
                )
                nfail += 1

    if nfail:
        sys.stderr.write(f"[INFO] low-T regression failed with {nfail} issue(s)\n")
        return 1

    print("[PASS] low-T regression checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
