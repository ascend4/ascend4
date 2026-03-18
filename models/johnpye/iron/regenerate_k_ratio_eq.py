#!/usr/bin/env python3
"""
Regenerate stepwise H2O/H2 equilibrium-ratio data for the TGA screen.

This helper reuses the existing Fe-O-H FPROPS diagnostics to compute:

1. hematite | magnetite:
      3 Fe2O3 + H2 <-> 2 Fe3O4 + H2O
2. magnetite | wustite:
      boundary-derived (p_H2O / p_H2)_eq from the current wustite|spinel model
3. wustite | iron:
      boundary-derived (p_H2O / p_H2)_eq from the current Fe|wustite model

It then fits:

    ln(K_ratio_eq) = A - B / T[K]

for convenient use inside `models/johnpye/iron/tga.a4c`.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from dataclasses import asdict, dataclass
from pathlib import Path


THIS_DIR = Path(__file__).resolve().parent
FPROPS_TEST_DIR = THIS_DIR.parent / "fprops" / "test"
sys.path.insert(0, str(FPROPS_TEST_DIR))

from feoh_hydrogen_boundary import (  # type: ignore
    default_runner,
    gas_ratio_logs,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
    query_mu0,
)


R = 8.31446261815324


@dataclass(frozen=True)
class DataPoint:
    temperature_k: float
    k1_hematite_magnetite: float
    k2_magnetite_wustite: float
    k3_wustite_iron: float


@dataclass(frozen=True)
class FitResult:
    name: str
    a: float
    b: float
    max_pct_err: float


def parse_temps_c(text: str) -> list[float]:
    temps: list[float] = []
    for tok in text.split(","):
        tok = tok.strip()
        if tok:
            temps.append(float(tok))
    if not temps:
        raise ValueError("no temperatures supplied")
    return temps


def k1_ratio_hematite_magnetite(
    runner: Path,
    source_mix: str,
    temperature_k: float,
) -> float:
    mu = query_mu0(
        runner,
        source_mix,
        temperature_k,
        ["Fe2O3", "Fe3O4", "hydrogen", "water"],
    )
    delta_g = (
        2.0 * mu["Fe3O4"]
        + mu["water"]
        - 3.0 * mu["Fe2O3"]
        - mu["hydrogen"]
    )
    return math.exp(-delta_g / (R * temperature_k))


def k_ratio_from_boundary(
    runner: Path,
    source_gas: str,
    temperature_k: float,
    boundary_fn,
) -> float:
    _x, lam_o = boundary_fn(temperature_k)
    mu = query_mu0(runner, source_gas, temperature_k, ["hydrogen", "water"])
    log10_h2o_h2, _ = gas_ratio_logs(mu["hydrogen"], mu["water"], lam_o, temperature_k)
    return 10.0 ** log10_h2o_h2


def fit_ln_k(temperatures_k: list[float], values: list[float]) -> FitResult:
    xs = [1.0 / t for t in temperatures_k]
    ys = [math.log(v) for v in values]
    n = len(xs)
    sx = sum(xs)
    sy = sum(ys)
    sxx = sum(x * x for x in xs)
    sxy = sum(x * y for x, y in zip(xs, ys))
    m = (n * sxy - sx * sy) / (n * sxx - sx * sx)
    c = (sy - m * sx) / n
    a = c
    b = -m
    max_pct_err = 0.0
    for temperature_k, value in zip(temperatures_k, values):
        fitted = math.exp(a - b / temperature_k)
        pct_err = abs((fitted / value - 1.0) * 100.0)
        max_pct_err = max(max_pct_err, pct_err)
    return FitResult(name="", a=a, b=b, max_pct_err=max_pct_err)


def write_csv(path: Path, rows: list[DataPoint]) -> None:
    with path.open("w", newline="", encoding="ascii") as fp:
        writer = csv.writer(fp)
        writer.writerow(
            [
                "temperature_k",
                "k1_hematite_magnetite",
                "k2_magnetite_wustite",
                "k3_wustite_iron",
            ]
        )
        for row in rows:
            writer.writerow(
                [
                    f"{row.temperature_k:.2f}",
                    f"{row.k1_hematite_magnetite:.12g}",
                    f"{row.k2_magnetite_wustite:.12g}",
                    f"{row.k3_wustite_iron:.12g}",
                ]
            )


def write_json(path: Path, rows: list[DataPoint], fits: list[FitResult], meta: dict[str, str]) -> None:
    payload = {
        "meta": meta,
        "rows": [asdict(r) for r in rows],
        "fits": [asdict(f) for f in fits],
    }
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="ascii")


def main() -> int:
    ap = argparse.ArgumentParser(description="Regenerate K_ratio_eq(T) data for the iron TGA screen.")
    ap.add_argument(
        "--temps-c",
        default="600,700,800,900,1000,1100,1200",
        help="Comma-separated temperatures in Celsius.",
    )
    ap.add_argument(
        "--gas-source",
        default="helmholtz+ref0:",
        help="Gas mu0 source for hydrogen and water.",
    )
    ap.add_argument(
        "--hematite-source",
        default="Fe2O3=hidayat_2015;Fe3O4=hidayat_2015;*=helmholtz+ref0:",
        help="Mixed mu0 source string for the hematite/magnetite step.",
    )
    ap.add_argument(
        "--runner",
        type=Path,
        default=default_runner(),
        help="Path to eqm_mu0_runner.",
    )
    ap.add_argument(
        "--csv-out",
        type=Path,
        help="Optional CSV output path for the raw K_ratio_eq(T) data.",
    )
    ap.add_argument(
        "--json-out",
        type=Path,
        help="Optional JSON output path for the raw data and fit summary.",
    )
    args = ap.parse_args()

    temperatures_k = [tc + 273.15 for tc in parse_temps_c(args.temps_c)]
    rows: list[DataPoint] = []
    for temperature_k in temperatures_k:
        rows.append(
            DataPoint(
                temperature_k=temperature_k,
                k1_hematite_magnetite=k1_ratio_hematite_magnetite(
                    args.runner, args.hematite_source, temperature_k
                ),
                k2_magnetite_wustite=k_ratio_from_boundary(
                    args.runner,
                    args.gas_source,
                    temperature_k,
                    oxygen_potential_at_wustite_spinel_boundary,
                ),
                k3_wustite_iron=k_ratio_from_boundary(
                    args.runner,
                    args.gas_source,
                    temperature_k,
                    oxygen_potential_at_fe_wustite_boundary,
                ),
            )
        )

    fit1 = fit_ln_k(temperatures_k, [r.k1_hematite_magnetite for r in rows])
    fit2 = fit_ln_k(temperatures_k, [r.k2_magnetite_wustite for r in rows])
    fit3 = fit_ln_k(temperatures_k, [r.k3_wustite_iron for r in rows])
    fit1 = FitResult("k1_hematite_magnetite", fit1.a, fit1.b, fit1.max_pct_err)
    fit2 = FitResult("k2_magnetite_wustite", fit2.a, fit2.b, fit2.max_pct_err)
    fit3 = FitResult("k3_wustite_iron", fit3.a, fit3.b, fit3.max_pct_err)
    fits = [fit1, fit2, fit3]

    print("K_ratio_eq(T) data for models/johnpye/iron/tga.a4c")
    print(f"runner = {args.runner}")
    print(f"gas source = {args.gas_source}")
    print(f"hematite source = {args.hematite_source}")
    print()
    print("T_K,K1_hematite_magnetite,K2_magnetite_wustite,K3_wustite_iron")
    for row in rows:
        print(
            f"{row.temperature_k:.2f},"
            f"{row.k1_hematite_magnetite:.12g},"
            f"{row.k2_magnetite_wustite:.12g},"
            f"{row.k3_wustite_iron:.12g}"
        )

    print()
    print("ASCEND coefficient block")
    print("(* regenerated by models/johnpye/iron/regenerate_k_ratio_eq.py *)")
    for idx, fit in enumerate(fits, start=1):
        print(f"K_eq_lnA[{idx}] := {fit.a:.16g};")
    for idx, fit in enumerate(fits, start=1):
        print(f"K_eq_lnB[{idx}] := {fit.b:.16g};")

    print()
    print("Fit summary")
    for fit in fits:
        print(
            f"{fit.name}: "
            f"A={fit.a:.16g}, "
            f"B={fit.b:.16g}, "
            f"max_pct_err={fit.max_pct_err:.6f}"
        )

    if args.csv_out:
        write_csv(args.csv_out, rows)
        print()
        print(f"wrote CSV: {args.csv_out}")

    if args.json_out:
        meta = {
            "runner": str(args.runner),
            "gas_source": args.gas_source,
            "hematite_source": args.hematite_source,
        }
        write_json(args.json_out, rows, fits, meta)
        print(f"wrote JSON: {args.json_out}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
