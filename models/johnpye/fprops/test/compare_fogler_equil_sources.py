#!/usr/bin/env python3
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


def set_solver_int_param(sim, name, value):
	for p in sim.getParameters():
		if p.getName() == name:
			p.setIntValue(value)
			return True
	return False


def solve_equil(lib, modelname):
	t = lib.findType(modelname)
	sim = t.getSimulation(f"sim_{modelname}", True)
	sim.setSolver(ascpy.Solver("QRSlv"))
	set_solver_int_param(sim, "iterationlimit", 1000)
	sim.solve(sim.getSolver(), ascpy.SolverReporter())
	return sim.Xeq.getRealValue()


def main():
	lib = ascpy.Library()
	lib.load("models/johnpye/fprops/reactive_fogler_demo.a4c")

	x_rpp = solve_equil(lib, "reactor_equil_fogler_n2o4_demo")
	x_ideal = solve_equil(lib, "reactor_equil_fogler_n2o4_ideal_demo")
	x_pengrob = solve_equil(lib, "reactor_equil_fogler_n2o4_pengrob_demo")
	x_fogler = 0.51

	print(f"Fogler target Xeq={x_fogler:.12g}")
	print(f"RPP/auto equilibrium Xeq={x_rpp:.12g}")
	print(f"ideal+ref0:RPP equilibrium Xeq={x_ideal:.12g}")
	print(f"pengrob+ref0:RPP equilibrium Xeq={x_pengrob:.12g}")
	print(f"delta(RPP, ideal)={x_rpp - x_ideal:.12g}")
	print(f"delta(ideal, pengrob)={x_ideal - x_pengrob:.12g}")
	print(f"delta(ideal, Fogler)={x_ideal - x_fogler:.12g}")

	assert abs(x_rpp - x_ideal) < 1e-12, "plain RPP should match ideal+ref0:RPP on the current equilibrium path"
	assert x_pengrob < x_ideal - 0.05, "explicit Peng-Robinson path should differ materially from the ideal-RPP path for this case"
	assert abs(x_ideal - x_fogler) > 0.05, "database-derived ideal equilibrium should remain visibly distinct from Fogler's textbook Kc basis"


if __name__ == "__main__":
	main()
