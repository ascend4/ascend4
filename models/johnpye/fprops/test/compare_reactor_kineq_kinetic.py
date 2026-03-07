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


def maybe_child(array_instance, child_name):
	for child in array_instance.getChildren():
		if str(child.getName()) == child_name:
			return child
	return None


def set_solver_int_param(sim, name, value):
	for p in sim.getParameters():
		if p.getName() == name:
			p.setIntValue(value)
			return True
	return False


def build_model(lib, filename, modelname, solvername):
	lib.load(filename)
	t = lib.findType(modelname)
	sim = t.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver(solvername))
	return sim


def solve_model(lib, filename, modelname, solvername):
	sim = build_model(lib, filename, modelname, solvername)
	sim.solve(sim.getSolver(), ascpy.SolverReporter())
	return sim


def set_array_value(array_instance, child_name, value, units=None):
	child = array_child(array_instance, child_name)
	if units is None:
		child.setRealValue(value)
	else:
		child.setRealValueWithUnits(value, units)


def copy_kinetic_state_to_kineq(kin, kineq):
	for nm in ["n_butane", "butene_1", "n_octane"]:
		set_array_value(kineq.R.inlet.f, nm, array_child(kin.R.inlet.f, nm).to("kmol/h"), "kmol/h")
		set_array_value(kineq.R.outlet.f, nm, array_child(kin.R.outlet.f, nm).to("kmol/h"), "kmol/h")
		set_array_value(kineq.R.inlet.y, nm, array_child(kin.R.inlet.y, nm).getRealValue())
		set_array_value(kineq.R.outlet.y, nm, array_child(kin.R.outlet.y, nm).getRealValue())
		set_array_value(kineq.R.holdup.y, nm, array_child(kin.R.holdup.y, nm).getRealValue())
		set_array_value(kineq.R.holdup.n, nm, array_child(kin.R.holdup.n, nm).to("mol"), "mol")
		set_array_value(kineq.R.holdup.c, nm, array_child(kin.R.holdup.c, nm).to("mol/m^3"), "mol/m^3")
	kineq.R.inlet.flow.setRealValueWithUnits(kin.R.inlet.flow.to("mol/s"), "mol/s")
	kineq.R.outlet.flow.setRealValueWithUnits(kin.R.outlet.flow.to("mol/s"), "mol/s")
	kineq.R.inlet.H_flow.setRealValueWithUnits(kin.R.inlet.H_flow.to("W"), "W")
	kineq.R.outlet.H_flow.setRealValueWithUnits(kin.R.outlet.H_flow.to("W"), "W")
	kineq.R.holdup.c_tot.setRealValueWithUnits(kin.R.holdup.c_tot.to("mol/m^3"), "mol/m^3")
	kineq.R.inlet.T.setRealValueWithUnits(kin.R.inlet.T.to("K"), "K")
	kineq.R.inlet.P.setRealValueWithUnits(kin.R.inlet.P.to("Pa"), "Pa")
	kineq.R.outlet.T.setRealValueWithUnits(kin.R.outlet.T.to("K"), "K")
	kineq.R.outlet.P.setRealValueWithUnits(kin.R.outlet.P.to("Pa"), "Pa")
	kineq.R.Volume.setRealValueWithUnits(kin.R.Volume.to("m^3"), "m^3")
	kineq.R.DeltaP.setRealValueWithUnits(kin.R.DeltaP.to("Pa"), "Pa")
	kineq.R.holdup.T.setRealValueWithUnits(kin.R.holdup.T.to("K"), "K")
	kineq.R.holdup.P.setRealValueWithUnits(kin.R.holdup.P.to("Pa"), "Pa")
	kineq.R.holdup.state.H.setRealValueWithUnits(kin.R.holdup.state.H.to("J/mol"), "J/mol")
	kineq.R.holdup.N.setRealValueWithUnits(kin.R.holdup.N.to("mol"), "mol")
	kineq.R.holdup.V.setRealValueWithUnits(kin.R.holdup.V.to("m^3"), "m^3")
	kineq.R.holdup.V_molar.setRealValueWithUnits(kin.R.holdup.V_molar.to("m^3/mol"), "m^3/mol")
	kineq.R.holdup.H.setRealValueWithUnits(kin.R.holdup.H.to("J"), "J")
	if maybe_child(kineq.R, "Qdot") is not None:
		kineq.R.Qdot.setRealValueWithUnits(kin.R.Qdot.to("W"), "W")
	array_child(kineq.R.kinetics.rate_forward, "r1").setRealValueWithUnits(array_child(kin.R.kinetics.rate, "forward").to("mol/m^3/s"), "mol/m^3/s")
	if maybe_child(kineq.R.kinetics, "rate_reverse") is not None:
		array_child(kineq.R.kinetics.rate_reverse, "r1").setRealValueWithUnits(array_child(kin.R.kinetics.rate, "backward").to("mol/m^3/s"), "mol/m^3/s")
	array_child(kineq.R.kinetics.rate, "r1").setRealValueWithUnits(
		array_child(kin.R.kinetics.rate, "forward").to("mol/m^3/s")
		- array_child(kin.R.kinetics.rate, "backward").to("mol/m^3/s"),
		"mol/m^3/s",
	)


def try_solve_kineq(lib, kin, modelname):
	t = lib.findType(modelname)
	kineq = t.getSimulation(f"sim_{modelname}", True)
	kineq.setSolver(ascpy.Solver("QRSlv"))
	set_solver_int_param(kineq, "iterationlimit", 1000)
	copy_kinetic_state_to_kineq(kin, kineq)
	array_child(kineq.R.kinetics.K_eq, "r1").setRealValue(0.01)
	kineq.solve(kineq.getSolver(), ascpy.SolverReporter())
	return kineq


def report_delta(label, refval, testval):
	diff = testval - refval
	rel = diff / refval if refval else float("nan")
	print(f"{label}: kinetic={refval:.12g} kineq={testval:.12g} diff={diff:.12g} rel={rel:.12g}")


def main():
	lib = ascpy.Library()
	lib.load("models/johnpye/fprops/reactive_kinetic_demo.a4c")
	lib.load("models/johnpye/fprops/reactive_kineq_demo.a4c")
	tkin = lib.findType("test_reactive_kinetic_single_phase")
	kin = tkin.getSimulation("sim_kinetic", True)
	kin.setSolver(ascpy.Solver("QRSlv"))
	set_solver_int_param(kin, "iterationlimit", 1000)
	kin.solve(kin.getSolver(), ascpy.SolverReporter())
	kineq = try_solve_kineq(lib, kin, "test_reactive_kineq_single_phase")

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
	keq_rr = array_child(kineq.R.kinetics.rate_reverse, 'r1').to("mol/m^3/s")
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
	report_delta("reverse_rate_mol_m3_s", kin_rb, keq_rr)
	print(f"driving_force={drive:.12g}")
	print(f"quotient={quotient:.12g}")

	assert abs(keq_oct - kin_oct) < 1e-8, "kineq octane outlet no longer matches the initialized kinetic reversible baseline"
	assert abs(keq_but - kin_but) < 1e-8, "kineq n_butane outlet no longer matches the initialized kinetic reversible baseline"
	assert abs(keq_but1 - kin_but1) < 1e-8, "kineq butene_1 outlet no longer matches the initialized kinetic reversible baseline"
	assert abs(keq_rf - kin_rf) / max(abs(kin_rf), 1e-12) < 1e-10, "kineq forward rate no longer matches the kinetic reversible baseline"
	assert abs(keq_rr - kin_rb) / max(abs(kin_rb), 1e-12) < 1e-10, "kineq reverse rate no longer matches the kinetic reversible baseline"
	assert abs(keq_rate - (kin_rf - kin_rb)) / max(abs(kin_rf - kin_rb), 1e-12) < 1e-10, "kineq net rate no longer matches the kinetic reversible baseline"
	assert 0 < drive < 1.0
	assert 0 < quotient < 0.01


if __name__ == "__main__":
	main()
