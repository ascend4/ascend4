#!/usr/bin/env python3
"""
Run the first dynamic TGA screening model with IDA.

This script is intentionally minimal. It exists because `./a4 run`
currently performs only a final algebraic solve, whereas `tgadyn.a4c`
needs the Integrator API for time marching.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

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


def get_obs_names(integrator: ascpy.Integrator) -> list[str]:
	return [
		str(integrator.getObservedVariable(i).getName())
		for i in range(integrator.getNumObservedVars())
	]


def main() -> int:
	ap = argparse.ArgumentParser(description="Integrate the dynamic TGA screening model with IDA.")
	ap.add_argument(
		"--model-file",
		default="models/johnpye/iron/tgadyn.a4c",
		help="ASCEND model file to load.",
	)
	ap.add_argument(
		"--model",
		default="tga_reduction_screen_dyn",
		help="MODEL name to instantiate.",
	)
	ap.add_argument(
		"--run-method",
		help="Optional ASCEND METHOD to run after on_load and before integration.",
	)
	ap.add_argument(
		"--t-end-min",
		type=float,
		default=20.0,
		help="End time for the integration window in minutes.",
	)
	ap.add_argument(
		"--nsteps",
		type=int,
		default=200,
		help="Number of linearly spaced output steps.",
	)
	ap.add_argument(
		"--initial-step-s",
		type=float,
		default=1e-3,
		help="Initial IDA substep in seconds.",
	)
	ap.add_argument(
		"--min-step-s",
		type=float,
		default=1e-6,
		help="Minimum IDA substep in seconds.",
	)
	ap.add_argument(
		"--max-step-s",
		type=float,
		default=5.0,
		help="Maximum IDA substep in seconds.",
	)
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
	integrator.setLinearTimesteps(ascpy.Units("s"), 0.0, args.t_end_min * 60.0, args.nsteps)
	integrator.setInitialSubStep(args.initial_step_s)
	integrator.setMinSubStep(args.min_step_s)
	integrator.setMaxSubStep(args.max_step_s)
	integrator.setMaxSubSteps(100000)
	integrator.analyse()
	integrator.setReporter(ascpy.IntegratorReporterConsole(integrator))
	integrator.solve()

	obs_names = get_obs_names(integrator)
	print("independent=" + str(integrator.getIndependentVariable().getName()))
	print("observed=" + ",".join(obs_names))

	print(f"final_t_s={sim.t.getRealValue():.12g}")
	print(f"final_reduction_degree={sim.reduction_degree.getRealValue():.12g}")
	print(f"final_m_sample_kg={sim.m_sample.getRealValue():.12g}")
	print(f"final_y_H2_react={sim.y_H2_react.getRealValue():.12g}")
	print(f"final_y_H2O_react={sim.y_H2O_react.getRealValue():.12g}")
	print(f"final_J_transport={sim.J_transport.getRealValue():.12g}")
	print(f"t_RD50_s={sim.t_RD50.getRealValue():.12g}")
	print(f"rd50_reached={sim.rd50_reached}")

	obs = integrator.getObservations()
	print(f"num_observations={len(obs)}")
	if obs:
		print("last_observation=" + ",".join(f"{v:.12g}" for v in obs[-1]))

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
