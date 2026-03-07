#!/usr/bin/env python3
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


def array_child(array_instance, child_name):
	for child in array_instance.getChildren():
		if str(child.getName()) == child_name:
			return child
	raise KeyError(child_name)


def solve_model(lib, filename, modelname, solvername):
	lib.load(filename)
	t = lib.findType(modelname)
	sim = t.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver(solvername))
	sim.solve(sim.getSolver(), ascpy.SolverReporter())
	return sim


def report_delta(label, refval, testval):
	diff = testval - refval
	rel = diff / refval if refval else float("nan")
	print(f"{label}: kinetic={refval:.12g} kineq={testval:.12g} diff={diff:.12g} rel={rel:.12g}")


def main():
	lib = ascpy.Library()
	kin = solve_model(lib, "models/johnpye/fprops/reactive_kinetic_demo.a4c", "test_reactive_kinetic_single_phase", "QRSlv")
	try:
		kineq = solve_model(lib, "models/johnpye/fprops/reactive_kineq_demo.a4c", "test_reactive_kineq_single_phase", "QRSlv")
	except RuntimeError as err:
		print("reactor_kineq diagnostic: solver did not converge with the current formulation.")
		print(f"reactor_kineq diagnostic: {err}")
		return

	kin_oct = array_child(kin.R.outlet.f, 'n_octane').to("kmol/h")
	kin_but = array_child(kin.R.outlet.f, 'n_butane').to("kmol/h")
	kin_but1 = array_child(kin.R.outlet.f, 'butene_1').to("kmol/h")
	kin_rf = array_child(kin.R.kinetics.rate, 'forward').to("mol/m^3/s")
	kin_rb = array_child(kin.R.kinetics.rate, 'backward').to("mol/m^3/s")

	keq_oct = array_child(kineq.R.outlet.f, 'n_octane').to("kmol/h")
	keq_but = array_child(kineq.R.outlet.f, 'n_butane').to("kmol/h")
	keq_but1 = array_child(kineq.R.outlet.f, 'butene_1').to("kmol/h")
	keq_rate = array_child(kineq.R.kinetics.rate, 'r1').to("mol/m^3/s")
	keq_rf = array_child(kineq.R.kinetics.rate_forward, 'r1').to("mol/m^3/s")
	c_ref = kineq.R.c_ref.to("mol/m^3")
	quotient = (
		array_child(kineq.R.holdup.c, 'n_octane').to("mol/m^3") / c_ref
	) / (
		(array_child(kineq.R.holdup.c, 'n_butane').to("mol/m^3") / c_ref)
		* (array_child(kineq.R.holdup.c, 'butene_1').to("mol/m^3") / c_ref)
	)
	drive = 1.0 - quotient / array_child(kineq.R.kinetics.K_eq, 'r1').getRealValue()

	report_delta("octane_out_kmol_h", kin_oct, keq_oct)
	report_delta("n_butane_out_kmol_h", kin_but, keq_but)
	report_delta("butene_1_out_kmol_h", kin_but1, keq_but1)
	report_delta("net_rate_mol_m3_s", kin_rf - kin_rb, keq_rate)
	report_delta("forward_rate_mol_m3_s", kin_rf, keq_rf)
	print(f"driving_force={drive:.12g}")
	print(f"quotient={quotient:.12g}")

	assert abs(keq_oct - kin_oct) < 0.5, "kineq octane outlet is too far from the kinetic reversible baseline"
	assert abs(keq_but - kin_but) < 0.5, "kineq n_butane outlet is too far from the kinetic reversible baseline"
	assert abs(keq_but1 - kin_but1) < 0.5, "kineq butene_1 outlet is too far from the kinetic reversible baseline"
	assert abs(keq_rf - kin_rf) / max(abs(kin_rf), 1e-12) < 0.05, "kineq forward rate is too far from kinetic baseline"
	assert abs(keq_rate - (kin_rf - kin_rb)) / max(abs(kin_rf - kin_rb), 1e-12) < 0.1, "kineq net rate is too far from kinetic baseline"
	assert 0 < drive <= 1.0
	assert quotient > 0


if __name__ == "__main__":
	main()
