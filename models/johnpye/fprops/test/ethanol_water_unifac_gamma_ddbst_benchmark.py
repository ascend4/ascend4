#!/usr/bin/env python3
"""
Benchmark the FPROPS UNIFAC gamma(T,x) path against the public DDBST
original-UNIFAC ethanol-water table at 298 K.
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
DATAFILE = pathlib.Path(__file__).resolve().with_name("ethanol_water_gamma_ddbst_298K.csv")
A4 = ROOT / "a4"


@dataclass
class GammaPoint:
	source: str
	url: str
	T_K: float
	x_ethanol: float
	gamma_ethanol: float
	gamma_water: float


def load_points(path: pathlib.Path) -> list[GammaPoint]:
	points: list[GammaPoint] = []
	with path.open(newline="", encoding="ascii") as f:
		reader = csv.DictReader(f)
		for row in reader:
			points.append(
				GammaPoint(
					source=row["source"],
					url=row["url"],
					T_K=float(row["T_K"]),
					x_ethanol=float(row["x_ethanol"]),
					gamma_ethanol=float(row["gamma_ethanol"]),
					gamma_water=float(row["gamma_water"]),
				)
			)
	return points


def build_model_text(T_K: float, x_ethanol: float) -> str:
	x_water = 1.0 - x_ethanol
	return f"""REQUIRE "johnpye/fprops/unifac_flash.a4l";

MODEL point REFINES cmumodel;
\tcd IS_A components_data(['water','ethanol'], 'water');
\tug IS_A fprops_unifac_gamma_state(cd);
\tgamma_ethanol ALIASES ug.gamma['ethanol'];
\tgamma_water ALIASES ug.gamma['water'];
METHODS
\tMETHOD default_self;
\t\tRUN ug.default_self;
\tEND default_self;
\tMETHOD values;
\t\tug.T := {T_K:.12g} {{K}};
\t\tug.x['water'] := {x_water:.16g};
\t\tug.x['ethanol'] := {x_ethanol:.16g};
\tEND values;
\tMETHOD specify;
\t\tFIX ug.T;
\t\tFIX ug.x[cd.other_components];
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


def run_point(T_K: float, x_ethanol: float) -> tuple[float, float]:
	with tempfile.TemporaryDirectory(prefix="fprops_unifac_gamma_") as d:
		modelpath = pathlib.Path(d) / "point.a4c"
		modelpath.write_text(build_model_text(T_K, x_ethanol), encoding="ascii")
		cmd = [
			str(A4),
			"run",
			str(modelpath),
			"--model",
			"point",
			"-p",
			"gamma_ethanol",
			"gamma_water",
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
		matches = dict(re.findall(r"^(gamma_ethanol|gamma_water)\s*=\s*([-+0-9.eE]+)$", proc.stdout, re.M))
		try:
			return float(matches["gamma_ethanol"]), float(matches["gamma_water"])
		except KeyError as e:
			raise RuntimeError(f"Did not find expected outputs in solver output:\n{proc.stdout}") from e


def summarize(values: list[float]) -> tuple[float, float]:
	if not values:
		return 0.0, 0.0
	rms = math.sqrt(sum(v * v for v in values) / len(values))
	maxabs = max(abs(v) for v in values)
	return rms, maxabs


def main() -> int:
	parser = argparse.ArgumentParser()
	parser.add_argument("--data", type=pathlib.Path, default=DATAFILE)
	parser.add_argument("--limit", type=int, default=0, help="Limit number of points for debugging")
	parser.add_argument("--assert-combined-rms-max", type=float, default=None)
	parser.add_argument("--assert-combined-max-max", type=float, default=None)
	args = parser.parse_args()

	points = load_points(args.data)
	if args.limit > 0:
		points = points[: args.limit]

	ethanol_errors: list[float] = []
	water_errors: list[float] = []

	print("# FPROPS UNIFAC gamma benchmark")
	print(f"# data = {args.data}")
	print("T_K,x_ethanol,gamma_ethanol_ref,gamma_ethanol_pred,d_ethanol,gamma_water_ref,gamma_water_pred,d_water")

	for p in points:
		gamma_ethanol_pred, gamma_water_pred = run_point(p.T_K, p.x_ethanol)
		d_ethanol = gamma_ethanol_pred - p.gamma_ethanol
		d_water = gamma_water_pred - p.gamma_water
		ethanol_errors.append(d_ethanol)
		water_errors.append(d_water)
		print(
			f"{p.T_K:.4f},{p.x_ethanol:.6f},{p.gamma_ethanol:.6f},{gamma_ethanol_pred:.6f},{d_ethanol:+.6f},"
			f"{p.gamma_water:.6f},{gamma_water_pred:.6f},{d_water:+.6f}"
		)

	ethanol_rms, ethanol_max = summarize(ethanol_errors)
	water_rms, water_max = summarize(water_errors)
	both_rms, both_max = summarize(ethanol_errors + water_errors)
	print(f"# gamma_ethanol rms_abs_err = {ethanol_rms:.6f}")
	print(f"# gamma_ethanol max_abs_err = {ethanol_max:.6f}")
	print(f"# gamma_water rms_abs_err = {water_rms:.6f}")
	print(f"# gamma_water max_abs_err = {water_max:.6f}")
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
