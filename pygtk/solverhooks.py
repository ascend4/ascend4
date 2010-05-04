import ascpy
import time
import gtk
import gtk.glade
from solverreporter import *

class SolverHooksPython(ascpy.SolverHooks):
	def __init__(self):
		print "PYTHON SOLVER HOOKS"
		ascpy.SolverHooks.__init__(self,None)
	def setSolver(self,solvername,sim):
		sim.setSolver(ascpy.Solver(solvername))
		print "PYTHON: SOLVER is now %s" % sim.getSolver().getName()	
		return 0
	def setOption(self,optionname,val,sim):
		try:
			PP = sim.getParameters()
		except Exception,e:
			print "PYTHON ERROR: ",str(e)
			return ascpy.SLVREQ_OPTIONS_UNAVAILABLE
		try:
			for P in PP:
				if P.getName()==optionname:
					try:
						P.setValueValue(val)
						sim.setParameters(PP)
						print "PYTHON: SET",optionname,"to",repr(val)
						return 0
					except Exception,e:
						print "PYTHON ERROR: ",str(e)
						return ascpy.SLVREQ_WRONG_OPTION_VALUE_TYPE
			return ascpy.SLVREQ_INVALID_OPTION_NAME
		except Exception,e:
			print "PYTHON ERROR: ",str(e)
			return ascpy.SLVREQ_INVALID_OPTION_NAME
	def doSolve(self,inst,sim):
		try:
			print "PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName()
			sim.solve(sim.getSolver(),ascpy.SolverReporter())
		except Exception,e:
			print "PYTHON ERROR:",str(e)
			return 3
		return 0

class SolverHooksPythonBrowser(SolverHooksPython):
	def __init__(self,browser):
		self.browser = browser
		SolverHooksPython.__init__(self)
	def doSolve(self,inst,sim):
		try:
			if self.browser.prefs.getBoolPref("SolverReporter","show_popup",True):
				reporter = PopupSolverReporter(self.browser,sim.getNumVars())
			else:
				reporter = SimpleSolverReporter(self.browser)
		except Exception,e:
			print "PYTHON ERROR:",str(e)
			return 4
		try:
			print "PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName()
			sim.solve(sim.getSolver(),reporter)
		except Exception,e:
			print "PYTHON ERROR:",str(e)
			return 3
		return 0

