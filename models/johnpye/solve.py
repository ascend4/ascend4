from solverreporter import *
import extpy
browser = extpy.getbrowser()

def solve(self):
	""" run the active solver on the current model (reporting to the status bar only) """
	browser.sim.solve(browser.solver,SimpleSolverReporter(browser))

extpy.registermethod(solve)
