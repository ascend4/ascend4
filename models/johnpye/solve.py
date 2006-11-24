from solverreporter import *
import extpy

def solve(self):
	""" run the active solver on the current model (reporting to the status bar only) """

	assert self.__class__.__name__=="Instance"
	assert self.isModel()
	print "SELF IS A MODEL"

	browser = extpy.getbrowser()
	if browser:
		solver = browser.solver;
		reporter = SimpleSolverReporter(browser)
	else:
		solver = ascpy.Solver("QRSlv")
		reporter = ascpy.SolverReporter()

	sim = ascpy.Registry().getSimulation("sim")
	sim.build()
	sim.solve(solver,reporter)

extpy.registermethod(solve)
