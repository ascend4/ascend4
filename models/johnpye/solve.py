try:
	from solverreporter import *
	have_solverreporter = True
except ImportError:
	pass
	have_solverreporter = False

import extpy

def solve(self):
	""" run the active solver on the current model (reporting to the status bar only) """

	assert self.__class__.__name__=="Instance"
	assert self.isModel()
	print "SELF IS A MODEL"

	browser = extpy.getbrowser()
	if browser and have_solverreporter:
		solver = browser.solver;
		reporter = SimpleSolverReporter(browser)
	else:
		# default to the QRSlv solver. @TODO make this configurable somehow.
		solver = ascpy.Solver("QRSlv")
		reporter = ascpy.SolverReporter()

	# the 'sim' object is registered in simulation.cpp each time a method is to be run
	# (an exception is thrown if not available (eg if C++ not being used)
	sim = ascpy.Registry().getSimulation("sim")

	sim.build()
	sim.solve(solver,reporter)

extpy.registermethod(solve)
