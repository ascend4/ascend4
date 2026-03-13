#!/usr/bin/env python3
import json
import math
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
RUNNER = ROOT / "models/johnpye/fprops/test/eqm_case_runner"
OUTFILE = ROOT / "ammonia_eqm_vs_hr_helmholtz_ref0.tsv"

TEMPS_K = list(range(473, 1274, 100))
PRESSURES_ATM = [1.0, 30.0, 100.0, 200.0]
P0_ATM = 1.0
P0_PA = 1.0e5
SOURCE = "helmholtz+ref0:"
ALGORITHM = "auto_nullspace"


def hr_log10_ka(temp_k: float) -> float:
    return (
        2.1
        + (1.0 / 4.571)
        * (9591.0 / temp_k - 0.00046 * temp_k + 0.85e-6 * temp_k * temp_k)
        - 4.98 * math.log10(temp_k) / 1.985
    )


def hr_xi(temp_k: float, pressure_atm: float) -> float:
    ka = 10.0 ** hr_log10_ka(temp_k)

    def f(xi: float) -> float:
        return (
            ka
            * ((1.0 - xi) / 2.0) ** 0.5
            * (3.0 * (1.0 - xi) / 2.0) ** 1.5
            * (pressure_atm / P0_ATM)
            - xi * (2.0 - xi)
        )

    lo = 1e-12
    hi = 1.0 - 1e-12
    flo = f(lo)
    fhi = f(hi)
    if flo == 0.0:
        return lo
    if fhi == 0.0:
        return hi
    if flo * fhi > 0.0:
        raise RuntimeError(
            f"HR root not bracketed at T={temp_k:g} K, P={pressure_atm:g} atm"
        )
    for _ in range(200):
        mid = 0.5 * (lo + hi)
        fmid = f(mid)
        if abs(fmid) < 1e-14:
            return mid
        if flo * fmid <= 0.0:
            hi = mid
            fhi = fmid
        else:
            lo = mid
            flo = fmid
    return 0.5 * (lo + hi)


def hr_nh3_percent(temp_k: float, pressure_atm: float) -> float:
    xi = hr_xi(temp_k, pressure_atm)
    return 100.0 * xi / (2.0 - xi)


def run_eqm(temp_k: float, pressure_atm: float) -> dict:
    pressure_pa = pressure_atm * 101325.0
    cmd = [
        str(RUNNER),
        "ammonia_synthesis",
        f"{temp_k:.12g}",
        f"{pressure_pa:.12g}",
        ALGORITHM,
        SOURCE,
    ]
    out = subprocess.check_output(cmd, cwd=ROOT, text=True)
    json_start = out.find("{")
    if json_start < 0:
        raise RuntimeError(f"runner did not emit JSON: {out}")
    data = json.loads(out[json_start:])
    if data["status"] not in (0, 1, 6):
        raise RuntimeError(
            f"eqm failed at T={temp_k:g} K, P={pressure_atm:g} atm with status {data['status']}"
        )
    return data


def main() -> int:
    rows = []
    max_abs_diff = -1.0
    max_row = None

    for temp_k in TEMPS_K:
        for pressure_atm in PRESSURES_ATM:
            hr_pct = hr_nh3_percent(temp_k, pressure_atm)
            eqm = run_eqm(temp_k, pressure_atm)
            eqm_pct = 100.0 * eqm["y"][2]
            abs_diff = eqm_pct - hr_pct
            rel_diff = abs_diff / hr_pct * 100.0 if hr_pct != 0.0 else float("nan")
            row = {
                "T_K": temp_k,
                "P_atm": pressure_atm,
                "HR_NH3_percent": hr_pct,
                "EQM_NH3_percent": eqm_pct,
                "abs_diff_pct_points": abs_diff,
                "rel_diff_percent": rel_diff,
            }
            rows.append(row)
            if abs(abs_diff) > max_abs_diff:
                max_abs_diff = abs(abs_diff)
                max_row = row

    with OUTFILE.open("w", encoding="ascii") as f:
        f.write(
            "T_K\tP_atm\tHR_NH3_percent\tEQM_NH3_percent\t"
            "abs_diff_pct_points\trel_diff_percent\n"
        )
        for row in rows:
            f.write(
                f"{row['T_K']}\t{row['P_atm']:.0f}\t"
                f"{row['HR_NH3_percent']:.12g}\t{row['EQM_NH3_percent']:.12g}\t"
                f"{row['abs_diff_pct_points']:.12g}\t{row['rel_diff_percent']:.12g}\n"
            )

    print(
        "max_abs_diff_pct_points="
        f"{max_abs_diff:.6g} at T={max_row['T_K']} K, P={max_row['P_atm']:.0f} atm"
    )
    print(f"wrote {OUTFILE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
