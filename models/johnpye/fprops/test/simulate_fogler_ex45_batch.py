#!/usr/bin/env python3
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


def main():
	lib = ascpy.Library()
	lib.load("models/johnpye/fprops/fogler_ex45_batch.a4c")
	sim = lib.findType("fogler_ex45_batch").getSimulation("sim", True)
	sim.setSolver(ascpy.Solver("QRSlv"))
	sim.t.setDisplayUnitsOverride("min", True, True)

	I = ascpy.Integrator(sim)
	I.setEngine("IDA")
	I.setLinearTimesteps(ascpy.Units("s"), 0.0, 20.0 * 60.0, 200)
	I.setInitialSubStep(1e-4 * 60.0)
	I.setMinSubStep(1e-6 * 60.0)
	I.setMaxSubStep(0.1 * 60.0)
	I.setMaxSubSteps(10000)
	I.analyse()
	I.setReporter(ascpy.IntegratorReporterConsole(I))
	I.solve()

	indep = str(I.getIndependentVariable().getName())
	names = [
		str(I.getObservedVariable(i).getName())
		for i in range(I.getNumObservedVars())
	]
	print(f"independent={indep}")
	print("observed=" + ",".join(names))

	final_t_s = sim.t.getRealValue()
	final_t_min = final_t_s / 60.0
	x_final = sim.X.getRealValue()
	xeb = sim.Xeb.getRealValue()
	xtarget = sim.Xtarget.getRealValue()
	print(f"final_t_s={final_t_s:.12g} s")
	print(f"final_t_min={final_t_min:.12g} min")
	print(f"final_X={x_final:.12g}")
	print(f"Xeb={xeb:.12g}")
	print(f"Xtarget={xtarget:.12g}")

	obs = I.getObservations()
	print(f"num_observations={len(obs)}")
	if obs:
		last = obs[-1]
		print("last_observation=" + ",".join(f"{v:.12g}" for v in last))

	assert 0.43 < xeb < 0.45, "batch equilibrium conversion should match Fogler's ~0.44"
	assert 0.34 < xtarget < 0.36, "80% equilibrium target should be ~0.352"
	assert x_final > 0.4, "20 min batch run should approach equilibrium closely"
	assert x_final < xeb + 1e-4, "conversion should not overshoot equilibrium materially"


if __name__ == "__main__":
	main()
