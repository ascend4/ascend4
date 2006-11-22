from solverreporter import *
import extpy
browser = extpy.getbrowser()
if browser==None:
	raise ImportError

def solve(self):
	""" run the active solver on the current model (reporting to the status bar only) """
	browser.sim.build()
	browser.sim.solve(browser.solver,SimpleSolverReporter(browser))

extpy.registermethod(solve)
