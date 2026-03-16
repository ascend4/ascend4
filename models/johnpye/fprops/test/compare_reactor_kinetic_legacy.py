#!/usr/bin/env python3
import math
import os
import sys

try:
	import os as _os
	_dlflags = _os.RTLD_GLOBAL | _os.RTLD_NOW
	sys.setdlopenflags(_dlflags)
except Exception:
	pass

import ascpy


def array_child(array_instance, child_name):
	for child in array_instance.getChildren():
		if str(child.getName()) == child_name:
			return child
	raise KeyError(child_name)


def solve_model(lib, filename, modelname):
	lib.load(filename)
	t = lib.findType(modelname)
	sim = t.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver("QRSlv"))
	sim.solve(sim.getSolver(), ascpy.SolverReporter())
	return sim


def report_delta(label, oldval, newval):
	diff = newval - oldval
	rel = diff / oldval if oldval else float("nan")
	print(f"{label}: legacy={oldval:.12g} new={newval:.12g} diff={diff:.12g} rel={rel:.12g}")


def main():
	lib = ascpy.Library()
	old = solve_model(lib, "models/reactor.a4l", "test_single_phase_cstr")
	new = solve_model(lib, "models/johnpye/fprops/reactive_kinetic_demo.a4c", "test_reactive_kinetic_single_phase")

	old_oct = array_child(old.output.f, 'n_octane').to("kmol/h")
	old_but = array_child(old.output.f, 'n_butane').to("kmol/h")
	old_but1 = array_child(old.output.f, 'butene_1').to("kmol/h")
	old_T = old.output.T.to("K")
	old_rf = array_child(old.kinetics.rate, 'forward').to("mol/m^3/s")
	old_rb = array_child(old.kinetics.rate, 'backward').to("mol/m^3/s")

	new_oct = array_child(new.R.outlet.f, 'n_octane').to("kmol/h")
	new_but = array_child(new.R.outlet.f, 'n_butane').to("kmol/h")
	new_but1 = array_child(new.R.outlet.f, 'butene_1').to("kmol/h")
	new_T = new.R.outlet.T.to("K")
	new_rf = array_child(new.R.kinetics.rate, 'forward').to("mol/m^3/s")
	new_rb = array_child(new.R.kinetics.rate, 'backward').to("mol/m^3/s")

	report_delta("octane_out_kmol_h", old_oct, new_oct)
	report_delta("n_butane_out_kmol_h", old_but, new_but)
	report_delta("butene_1_out_kmol_h", old_but1, new_but1)
	report_delta("temperature_K", old_T, new_T)
	report_delta("forward_rate_mol_m3_s", old_rf, new_rf)
	report_delta("backward_rate_mol_m3_s", old_rb, new_rb)

	# Legacy reactor uses the old vapor-mixture thermo stack, so exact equality is
	# not expected. The useful regression is that the new reactor stays close on
	# the same chemistry and closure.
	assert abs(new_oct - old_oct) < 1.0, "octane outlet differs too much from legacy reactor"
	assert abs(new_but - old_but) < 1.0, "n_butane outlet differs too much from legacy reactor"
	assert abs(new_but1 - old_but1) < 1.0, "butene_1 outlet differs too much from legacy reactor"
	assert abs(new_T - old_T) < 1e-6, "temperature closure no longer matches legacy reactor"
	assert abs(new_rf - old_rf) / max(abs(old_rf), 1e-12) < 0.1, "forward rate differs too much from legacy reactor"
	assert abs(new_rb - old_rb) / max(abs(old_rb), 1e-12) < 0.1, "backward rate differs too much from legacy reactor"


if __name__ == "__main__":
	main()
