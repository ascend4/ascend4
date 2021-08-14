import threading

from gi.repository import GObject

from solverreporter import *


class SolverHooksPython(ascpy.SolverHooks):
	def __init__(self):
		print("PYTHON SOLVER HOOKS")
		ascpy.SolverHooks.__init__(self,None)
	def setSolver(self,solvername,sim):
		sim.setSolver(ascpy.Solver(solvername))
		print("PYTHON: SOLVER is now %s" % sim.getSolver().getName())	
		return 0
	def setOption(self,optionname,val,sim):
		try:
			PP = sim.getParameters()
		except Exception as e:
			print("PYTHON ERROR: ",str(e))
			return ascpy.SLVREQ_OPTIONS_UNAVAILABLE
		try:
			for P in PP:
				if P.getName()==optionname:
					try:
						P.setValueValue(val)
						sim.setParameters(PP)
						print("PYTHON: SET",optionname,"to",repr(val))
						return 0
					except Exception as e:
						print("PYTHON ERROR: ",str(e))
						return ascpy.SLVREQ_WRONG_OPTION_VALUE_TYPE
			return ascpy.SLVREQ_INVALID_OPTION_NAME
		except Exception as e:
			print("PYTHON ERROR: ",str(e))
			return ascpy.SLVREQ_INVALID_OPTION_NAME
	def doSolve(self,inst,sim):
		try:
			print("PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName())
			sim.solve(sim.getSolver(),ascpy.SolverReporter())
		except Exception as e:
			print("PYTHON ERROR:",str(e))
			return 3
		return 0

class SolverHooksPythonBrowser(SolverHooksPython):
	def __init__(self,browser):
		self.browser = browser
		self.solve_interrupt = False
		SolverHooksPython.__init__(self)
	def doSolve(self,inst,sim):
		try:
			if self.browser.prefs.getBoolPref("SolverReporter","show_popup",True):
				reporter = PopupSolverReporter(self.browser, sim)
			else:
				reporter = SimpleSolverReporter(self.browser)
		except Exception as e:
			print("PYTHON ERROR:",str(e))
			return 4

		print("PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName())
		thread = threading.Thread(target=self.do_solve_thread, args=(sim, reporter))
		thread.daemon = True
		thread.start()
		# FIXME improve in case of error in solving
		# unfortunately there is no possibility to get result from async task without waiting
		# so we assume everything is fine
		return 0

	# the same functions like in gtkbrowser, but a little bit other implementation
	def do_solve_update(self, reporter, status):
		self.solve_interrupt = reporter.report(status)
		return False

	def do_solve_finish(self, reporter, status):
		reporter.finalise(status)
		return False

	def do_solve_thread(self, sim, reporter):
		try:
			sim.presolve(sim.getSolver())
			status = sim.getStatus()
			while status.isReadyToSolve() and not self.solve_interrupt:
				res = sim.iterate()
				status.getSimulationStatus(sim)
				GObject.idle_add(self.do_solve_update, reporter, status)
				# need more time than in gtkbrowser to update gui
				# probably because it's hook
				time.sleep(0.02)
				if res != 0:
					break
			GObject.idle_add(self.do_solve_finish, reporter, status)
			sim.postsolve(status)
		except Exception as e:
			print("PYTHON ERROR:", str(e))

