#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


def find_method(model_type, name: str):
	for method in model_type.getMethods():
		if method.getName() == name:
			return method
	raise RuntimeError(f"Method '{name}' not found on model '{model_type.getName()}'")


def main() -> int:
	ap = argparse.ArgumentParser(description="Probe ASCEND IDA analyse/solve on a model.")
	ap.add_argument("--model-file", required=True)
	ap.add_argument("--model", required=True)
	ap.add_argument("--run-method")
	ap.add_argument("--t-end-s", type=float, default=10.0)
	ap.add_argument("--nsteps", type=int, default=20)
	ap.add_argument("--solve", action="store_true", help="Call solve after analyse.")
	args = ap.parse_args()

	lib = ascpy.Library()
	lib.load(args.model_file)
	model_type = lib.findType(args.model)
	sim = model_type.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver("QRSlv"))

	if args.run_method:
		sim.run(find_method(model_type, args.run_method))

	integrator = ascpy.Integrator(sim)
	integrator.setEngine("IDA")
	integrator.setLinearTimesteps(ascpy.Units("s"), 0.0, args.t_end_s, args.nsteps)
	integrator.setInitialSubStep(1e-3)
	integrator.setMaxSubStep(1.0)
	integrator.setMaxSubSteps(10000)
	integrator.setParameter("autodiff", False)

	print("probe_stage=analyse", flush=True)
	integrator.analyse()
	print("probe_stage=analyse_ok", flush=True)

	if args.solve:
		integrator.setReporter(ascpy.IntegratorReporterConsole(integrator))
		print("probe_stage=solve", flush=True)
		integrator.solve()
		print("probe_stage=solve_ok", flush=True)

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
