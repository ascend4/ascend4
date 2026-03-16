#!/usr/bin/env python3
"""
Benchmark the FPROPS ethanol-water UNIFAC TPz flash against experimental
binary VLE tie-line data.

For each experimental point (T, x, y) at fixed pressure, a midpoint overall
composition is constructed as:

    z = beta * y + (1 - beta) * x

with beta = 0.5 by default. The FPROPS TPz flash is then solved at that T, P,
and z, and the returned liquid/vapor compositions are compared with the
experimental x and y.
"""

from __future__ import annotations

import argparse
import csv
import math
import pathlib
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass


ROOT = pathlib.Path(__file__).resolve().parents[4]
DATAFILE = pathlib.Path(__file__).resolve().with_name("ethanol_water_vle_101kPa_liu2012.csv")
A4 = ROOT / "a4"


@dataclass
class VLEPoint:
    source: str
    doi: str
    pressure_kPa: float
    T_K: float
    x_ethanol: float
    y_ethanol: float


def load_points(path: pathlib.Path) -> list[VLEPoint]:
    points: list[VLEPoint] = []
    with path.open(newline="", encoding="ascii") as f:
        reader = csv.DictReader(f)
        for row in reader:
            x = float(row["x_ethanol"])
            y = float(row["y_ethanol"])
            # Pure endpoints are not informative for the two-phase TPz check.
            if (x == 0.0 and y == 0.0) or (x == 1.0 and y == 1.0):
                continue
            points.append(
                VLEPoint(
                    source=row["source"],
                    doi=row["doi"],
                    pressure_kPa=float(row["pressure_kPa"]),
                    T_K=float(row["T_K"]),
                    x_ethanol=x,
                    y_ethanol=y,
                )
            )
    return points


def build_model_text(T_K: float, P_kPa: float, z_ethanol: float) -> str:
    z_water = 1.0 - z_ethanol
    return f"""REQUIRE "johnpye/fprops/unifac_flash.a4l";

MODEL point REFINES cmumodel;
\tcd IS_A components_data(['water','ethanol'], 'water');
\tfp IS_A fprops_unifac_flash_state(cd);
\tx_ethanol ALIASES fp.x['ethanol'];
\ty_ethanol ALIASES fp.y['ethanol'];
\tbeta ALIASES fp.beta;
METHODS
\tMETHOD default_self;
\t\tRUN fp.default_self;
\tEND default_self;
\tMETHOD values;
\t\tfp.T := {T_K:.12g} {{K}};
\t\tfp.P := {P_kPa:.12g} {{kPa}};
\t\tfp.z['water'] := {z_water:.16g};
\t\tfp.z['ethanol'] := {z_ethanol:.16g};
\tEND values;
\tMETHOD specify;
\t\tFIX fp.T;
\t\tFIX fp.P;
\t\tFIX fp.z[cd.other_components];
\tEND specify;
\tMETHOD on_load;
\t\tRUN default_self;
\t\tRUN values;
\t\tRUN specify;
\t\tSOLVER QRSlv;
\t\tOPTION convopt 'RELNOM_SCALE';
\t\tSOLVE;
\tEND on_load;
END point;
"""


def run_point(T_K: float, P_kPa: float, z_ethanol: float) -> tuple[float, float, float]:
    with tempfile.TemporaryDirectory(prefix="fprops_unifac_tpz_") as d:
        modelpath = pathlib.Path(d) / "point.a4c"
        modelpath.write_text(build_model_text(T_K, P_kPa, z_ethanol), encoding="ascii")
        cmd = [
            str(A4),
            "run",
            str(modelpath),
            "--model",
            "point",
            "-p",
            "beta",
            "x_ethanol",
            "y_ethanol",
        ]
        proc = subprocess.run(
            cmd,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        if proc.returncode != 0:
            raise RuntimeError(proc.stdout.strip())

        matches = dict(re.findall(r"^(beta|x_ethanol|y_ethanol)\s*=\s*([-+0-9.eE]+)$", proc.stdout, re.M))
        try:
            beta = float(matches["beta"])
            x_ethanol = float(matches["x_ethanol"])
            y_ethanol = float(matches["y_ethanol"])
        except KeyError as e:
            raise RuntimeError(f"Did not find expected outputs in solver output:\n{proc.stdout}") from e
        return beta, x_ethanol, y_ethanol


def summarize(values: list[float]) -> tuple[float, float]:
    if not values:
        return 0.0, 0.0
    rms = math.sqrt(sum(v * v for v in values) / len(values))
    maxabs = max(abs(v) for v in values)
    return rms, maxabs


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=pathlib.Path, default=DATAFILE)
    parser.add_argument("--beta", type=float, default=0.5, help="Tie-line interpolation fraction for z = beta*y + (1-beta)*x")
    parser.add_argument("--limit", type=int, default=0, help="Limit number of points for debugging")
    parser.add_argument("--assert-combined-rms-max", type=float, default=None)
    parser.add_argument("--assert-combined-max-max", type=float, default=None)
    args = parser.parse_args()

    if not 0.0 < args.beta < 1.0:
        raise SystemExit("--beta must be strictly between 0 and 1")

    points = load_points(args.data)
    if args.limit > 0:
        points = points[: args.limit]

    x_errors: list[float] = []
    y_errors: list[float] = []

    print(f"# FPROPS UNIFAC TPz benchmark")
    print(f"# data = {args.data}")
    print(f"# beta = {args.beta}")
    print("T_K,z_ethanol,x_exp,x_pred,dx,y_exp,y_pred,dy,beta_pred")

    for p in points:
        z_ethanol = args.beta * p.y_ethanol + (1.0 - args.beta) * p.x_ethanol
        beta_pred, x_pred, y_pred = run_point(p.T_K, p.pressure_kPa, z_ethanol)
        dx = x_pred - p.x_ethanol
        dy = y_pred - p.y_ethanol
        x_errors.append(dx)
        y_errors.append(dy)
        print(
            f"{p.T_K:.4f},{z_ethanol:.6f},{p.x_ethanol:.6f},{x_pred:.6f},{dx:+.6f},"
            f"{p.y_ethanol:.6f},{y_pred:.6f},{dy:+.6f},{beta_pred:.6f}"
        )

    x_rms, x_max = summarize(x_errors)
    y_rms, y_max = summarize(y_errors)
    both_rms, both_max = summarize(x_errors + y_errors)
    print(f"# x_ethanol rms_abs_err = {x_rms:.6f}")
    print(f"# x_ethanol max_abs_err = {x_max:.6f}")
    print(f"# y_ethanol rms_abs_err = {y_rms:.6f}")
    print(f"# y_ethanol max_abs_err = {y_max:.6f}")
    print(f"# combined rms_abs_err = {both_rms:.6f}")
    print(f"# combined max_abs_err = {both_max:.6f}")
    if args.assert_combined_rms_max is not None and both_rms > args.assert_combined_rms_max:
        print(
            f"ERROR: combined rms_abs_err {both_rms:.6f} exceeds limit {args.assert_combined_rms_max:.6f}",
            file=sys.stderr,
        )
        return 1
    if args.assert_combined_max_max is not None and both_max > args.assert_combined_max_max:
        print(
            f"ERROR: combined max_abs_err {both_max:.6f} exceeds limit {args.assert_combined_max_max:.6f}",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        raise SystemExit(1)
