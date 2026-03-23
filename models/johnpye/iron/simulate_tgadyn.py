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


def find_observation_index(obs_names: list[str], suffix: str) -> int:
	target = suffix[1:] if suffix.startswith(".") else suffix
	for idx, name in enumerate(obs_names):
		if name.endswith(suffix) or name == target or name.split(".")[-1] == target:
			return idx
	raise RuntimeError(f"Observed variable ending with '{suffix}' not found; got {obs_names!r}")


def interpolate_crossing_time_s(
	observations: list[list[float]] | list[tuple[float, ...]],
	time_idx: int,
	value_idx: int,
	target: float,
) -> float | None:
	prev_t = None
	prev_v = None
	for row in observations:
		t = float(row[time_idx])
		v = float(row[value_idx])
		if v >= target:
			if prev_t is None or prev_v is None:
				return t
			if v == prev_v:
				return t
			f = (target - prev_v) / (v - prev_v)
			return prev_t + f * (t - prev_t)
		prev_t = t
		prev_v = v
	return None


def array_child(array_instance, child_name: str):
	for child in array_instance.getChildren():
		if str(child.getName()) == child_name:
			return child
	raise KeyError(child_name)


def copy_named_real(src, dst, name: str) -> None:
	getattr(dst, name).setRealValue(getattr(src, name).getRealValue())


def copy_real_tree(src, dst) -> None:
	if src.isArray() or src.isCompound() or src.isModel():
		src_children = list(src.getChildren())
		dst_children = list(dst.getChildren())
		for src_child, dst_child in zip(src_children, dst_children):
			copy_real_tree(src_child, dst_child)
		return
	if src.isReal() and not dst.isConst():
		dst.setRealValue(src.getRealValue())
		return
	if src.isBool() and not dst.isConst():
		dst.setBoolValue(src.getBoolValue())


def copy_initializer_state(init_sim, dyn_sim) -> None:
	scalars = [
		"T",
		"p",
		"A_crucible",
		"L_freeboard",
		"m_sample_init",
		"m_sample",
		"rho_bulk_bed",
		"H_bed",
		"eps_bed",
		"tau_bed",
		"k_fb_multiplier",
		"k_bed_multiplier",
		"y_H2_bulk",
		"y_H2O_bulk",
		"y_Ar_bulk",
		"y_H2_react",
		"y_H2O_react",
		"y_Ar_react",
		"c_tot",
		"D_screen",
		"D_eff_bed",
		"k_fb",
		"k_bed",
		"k_overall",
		"R_fb",
		"R_bed",
		"R_overall",
		"J_transport",
		"J_freeboard_cap",
		"J_bed_cap",
		"J_overall_cap",
		"transport_screen_freeboard",
		"transport_screen_bed",
		"transport_screen_overall",
		"MW_Fe2O3",
		"MW_Fe3O4",
		"MW_FeO",
		"MW_Fe",
		"n0_Fe2O3",
		"n0_per_area",
		"n_Fe2O3",
		"n_Fe3O4",
		"n_FeO",
		"n_Fe",
		"oxygen_remaining",
		"reduction_degree",
		"J_reaction",
	]
	for name in scalars:
		copy_named_real(init_sim, dyn_sim, name)

	step_arrays = [
		"k0",
		"E_a",
		"k_step",
		"K_eq_lnA",
		"K_eq_lnB",
		"K_ratio_eq",
		"availability",
		"drive",
		"rate_forward",
		"rate_reverse",
		"rate_net",
	]
	for array_name in step_arrays:
		src_array = getattr(init_sim, array_name)
		dst_array = getattr(dyn_sim, array_name)
		for idx in ("1", "2", "3"):
			array_child(dst_array, idx).setRealValue(array_child(src_array, idx).getRealValue())

	copy_real_tree(init_sim.tr, dyn_sim.tr)


def main() -> int:
	ap = argparse.ArgumentParser(description="Integrate the dynamic TGA screening model with IDA.")
	ap.add_argument(
		"--model-file",
		default="models/johnpye/iron/tgadyn.a4c",
		help="ASCEND model file to load.",
	)
	ap.add_argument(
		"--init-model-file",
		default="models/johnpye/iron/tga.a4c",
		help="ASCEND model file used for steady algebraic initialization.",
	)
	ap.add_argument(
		"--model",
		default="tga_reduction_screen_dyn",
		help="MODEL name to instantiate.",
	)
	ap.add_argument(
		"--init-model",
		default="tga_reduction_screen",
		help="Steady MODEL name to instantiate for algebraic initialization.",
	)
	ap.add_argument(
		"--run-method",
		help="Optional ASCEND METHOD to run after base initialization and before ode_init. Prefer no-solve prep methods such as prep_sahar_873K or prep_sahar_1073K.",
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
	lib.load(args.init_model_file)
	lib.load(args.model_file)
	init_model_type = lib.findType(args.init_model)
	model_type = lib.findType(args.model)

	sim_init = init_model_type.getSimulation("sim_init", True)
	sim_init.setSolver(ascpy.Solver("QRSlv"))
	if args.run_method:
		sim_init.run(find_method(init_model_type, args.run_method))
	print("integrator_stage=steady_init", flush=True)
	sim_init.solve(sim_init.getSolver(), ascpy.SolverReporter())

	sim = model_type.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver("QRSlv"))

	if args.run_method:
		sim.run(find_method(model_type, args.run_method))
	print("integrator_stage=copy_init", flush=True)
	copy_initializer_state(sim_init, sim)

	integrator = ascpy.Integrator(sim)
	integrator.setEngine("IDA")
	integrator.setLinearTimesteps(ascpy.Units("s"), 0.0, args.t_end_min * 60.0, args.nsteps)
	integrator.setInitialSubStep(args.initial_step_s)
	integrator.setMinSubStep(args.min_step_s)
	integrator.setMaxSubStep(args.max_step_s)
	integrator.setMaxSubSteps(100000)
	integrator.setParameter("autodiff", False)
	print("integrator_stage=analyse", flush=True)
	integrator.analyse()
	integrator.setReporter(ascpy.IntegratorReporterConsole(integrator))
	print("integrator_stage=solve", flush=True)
	solve_error = None
	try:
		integrator.solve()
	except RuntimeError as exc:
		solve_error = exc
	print("integrator_stage=complete", flush=True)

	obs_names = get_obs_names(integrator)
	print("independent=" + str(integrator.getIndependentVariable().getName()))
	print("observed=" + ",".join(obs_names))

	print(f"final_t_s={sim.t.getRealValue():.12g}")
	print(f"final_t_min={sim.t.getRealValue() / 60.0:.12g}")
	print(f"final_reduction_degree={sim.reduction_degree.getRealValue():.12g}")
	print(f"final_m_sample_kg={sim.m_sample.getRealValue():.12g}")
	print(f"final_y_H2_react={sim.y_H2_react.getRealValue():.12g}")
	print(f"final_y_H2O_react={sim.y_H2O_react.getRealValue():.12g}")
	print(f"final_J_transport={sim.J_transport.getRealValue():.12g}")

	obs = integrator.getObservations()
	print(f"num_observations={len(obs)}")
	rd_idx = find_observation_index(obs_names, ".reduction_degree")
	time_idx = len(obs_names)
	t_rd50_s = interpolate_crossing_time_s(obs, time_idx, rd_idx, 0.5)
	if solve_error is None:
		print("integration_status=success")
	else:
		print("integration_status=failed")
		print(f"integration_error={solve_error}")
	if t_rd50_s is None:
		print("t_RD50_s=nan")
		print("t_RD50_min=nan")
		print("rd50_reached=False")
	else:
		print(f"t_RD50_s={t_rd50_s:.12g}")
		print(f"t_RD50_min={t_rd50_s / 60.0:.12g}")
		print("rd50_reached=True")
	if obs:
		print("last_observation=" + ",".join(f"{v:.12g}" for v in obs[-1]))

	if solve_error is not None and t_rd50_s is None:
		return 1
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
