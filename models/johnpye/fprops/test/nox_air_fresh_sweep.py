#!/usr/bin/env python3
import argparse
import math
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


MODEL_FILE = "models/johnpye/fprops/reactive_equil_nox_air_demo.a4c"
MODEL_NAME = "reactive_equil_nox_air_debug"


def status_label(status):
	if status.isConverged():
		return "converged"
	if status.isReadyToSolve():
		return "ready"
	if status.isDiverged():
		return "diverged"
	if status.isInterrupted():
		return "interrupted"
	if status.hasExceededTimeLimit():
		return "time-limit"
	if status.hasExceededIterationLimit():
		return "iteration-limit"
	if status.hasResidualCalculationErrors():
		return "residual-errors"
	if status.isOverDefined():
		return "over-defined"
	if status.isUnderDefined():
		return "under-defined"
	return "not-converged"


def parse_temps(spec):
	if not spec:
		return [1100.0, 1000.0, 900.0, 800.0, 700.0, 650.0, 600.0, 550.0, 500.0, 400.0, 350.0, 300.0]
	temps = []
	for part in spec.split(","):
		part = part.strip()
		if not part:
			continue
		temps.append(float(part))
	return temps


def array_child(array_instance, child_name):
	for child in array_instance.getChildren():
		if str(child.getName()) == child_name:
			return child
	raise KeyError(child_name)


def set_solver_int_param(sim, name, value):
	for p in sim.getParameters():
		if p.getName() == name:
			p.setIntValue(value)
			return True
	return False


def build_simulation(lib, model_type, sim_name, iterationlimit):
	sim = model_type.getSimulation(sim_name, False)
	sim.setSolver(ascpy.Solver("QRSlv"))
	set_solver_int_param(sim, "iterationlimit", iterationlimit)
	return sim


def configure_case(sim, model_type, temp_k, prime_hot):
	sim.run(model_type.getMethod("default_self"))
	if prime_hot:
		sim.run(model_type.getMethod("solve_case"))
	sim.T_reactor.setRealValueWithUnits(temp_k, "K")


def solve_case(sim, model_type):
	sim.run(model_type.getMethod("solve_case"))
	return sim.getStatus()


def value_mol_s(array_instance, child_name):
	return array_child(array_instance, child_name).to("mol/s")


def value_ppm(instance):
	return instance.getRealValue()


def emit_row(temp_k, mode, sim, status, exc_text=None):
	row = {
		"T_K": temp_k,
		"mode": mode,
		"status": status_label(status) if status is not None else "exception",
		"NO_mol_s": math.nan,
		"NO2_mol_s": math.nan,
		"CO_mol_s": math.nan,
		"H2_mol_s": math.nan,
		"NO_ppm": math.nan,
		"NO2_ppm": math.nan,
		"CO_ppm": math.nan,
		"H2_ppm": math.nan,
		"Qdot_W": math.nan,
		"detail": exc_text or "",
	}
	if sim is not None:
		try:
			row["NO_mol_s"] = value_mol_s(sim.R.outlet.f, "NO")
			row["NO2_mol_s"] = value_mol_s(sim.R.outlet.f, "NO2")
			row["CO_mol_s"] = value_mol_s(sim.R.outlet.f, "CO")
			row["H2_mol_s"] = value_mol_s(sim.R.outlet.f, "H2")
			row["NO_ppm"] = value_ppm(sim.no_out_ppm)
			row["NO2_ppm"] = value_ppm(sim.no2_out_ppm)
			row["CO_ppm"] = value_ppm(sim.co_out_ppm)
			row["H2_ppm"] = value_ppm(sim.h2_out_ppm)
			row["Qdot_W"] = sim.R.Qdot.to("W")
		except Exception as exc:
			if not row["detail"]:
				row["detail"] = f"value-read-failed: {exc}"
	return row


def print_rows(rows):
	headers = [
		"T_K", "mode", "status", "NO_mol_s", "NO2_mol_s", "CO_mol_s", "H2_mol_s",
		"NO_ppm", "NO2_ppm", "CO_ppm", "H2_ppm", "Qdot_W", "detail",
	]
	print("\t".join(headers))
	for row in rows:
		values = []
		for key in headers:
			val = row[key]
			if isinstance(val, float):
				values.append(f"{val:.15g}")
			else:
				values.append(str(val))
		print("\t".join(values))


def run_fresh(lib, model_type, temps, iterationlimit, prime_hot):
	rows = []
	for i, temp_k in enumerate(temps):
		sim = None
		try:
			sim = build_simulation(lib, model_type, f"fresh_{i}", iterationlimit)
			configure_case(sim, model_type, temp_k, prime_hot)
			status = solve_case(sim, model_type)
			rows.append(emit_row(temp_k, "fresh_hot" if prime_hot else "fresh", sim, status))
		except Exception as exc:
			status = sim.getStatus() if sim is not None else None
			rows.append(emit_row(temp_k, "fresh_hot" if prime_hot else "fresh", sim, status, str(exc)))
	return rows


def run_reuse(lib, model_type, temps, iterationlimit):
	rows = []
	sim = build_simulation(lib, model_type, "reuse", iterationlimit)
	try:
		sim.run(model_type.getMethod("default_self"))
		for temp_k in temps:
			try:
				sim.T_reactor.setRealValueWithUnits(temp_k, "K")
				status = solve_case(sim, model_type)
				rows.append(emit_row(temp_k, "reuse", sim, status))
			except Exception as exc:
				rows.append(emit_row(temp_k, "reuse", sim, sim.getStatus(), str(exc)))
	finally:
		pass
	return rows


def main():
	parser = argparse.ArgumentParser(description="Run NOx-air demo as fresh points or a reused descending path.")
	parser.add_argument("--model-file", default=MODEL_FILE)
	parser.add_argument("--model", default=MODEL_NAME)
	parser.add_argument("--temps", default="")
	parser.add_argument("--mode", choices=["fresh", "reuse", "both"], default="both")
	parser.add_argument("--iterationlimit", type=int, default=1000)
	parser.add_argument("--prime-hot", action="store_true",
		help="Run solve_case once at the default 3000 K state before setting the requested temperature.")
	args = parser.parse_args(sys.argv[1:])

	lib = ascpy.Library()
	lib.load(args.model_file)
	model_type = lib.findType(args.model)
	temps = parse_temps(args.temps)

	rows = []
	if args.mode in ("fresh", "both"):
		rows.extend(run_fresh(lib, model_type, temps, args.iterationlimit, args.prime_hot))
	if args.mode in ("reuse", "both"):
		rows.extend(run_reuse(lib, model_type, temps, args.iterationlimit))
	print_rows(rows)


if __name__ == "__main__":
	main()
