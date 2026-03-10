import threading
import loading
import ascpy
from gi.repository import GObject

from solverreporter import *
from study import StudyWin

SLVREQ_NOT_IMPLEMENTED = 8
SLVREQ_STUDY_NONE = 0


class SolverHooksPython(ascpy.SolverHooks):
	def __init__(self):
		loading.print_status("","Loaded python solver hooks")
		ascpy.SolverHooks.__init__(self,None)
		self._solver_name = {}
		self._solver_options = {}
		self._default_solver_name = None
		self._default_solver_options = {}
	def _simkey(self, sim):
		try:
			return sim.getName()
		except Exception:
			return str(sim.this)
	def _get_option_store(self, sim):
		return self._solver_options.setdefault(self._simkey(sim), {})
	def _snapshot_parameter(self, param):
		if param.isInt():
			return ("int", param.getIntValue())
		if param.isBool():
			return ("bool", param.getBoolValue())
		if param.isReal():
			return ("real", param.getRealValue())
		if param.isStr():
			return ("str", param.getStrValue())
		raise RuntimeError("Unsupported solver parameter type")
	def _apply_parameter_value(self, param, stored):
		kind, value = stored
		if kind == "int":
			param.setIntValue(value)
		elif kind == "bool":
			param.setBoolValue(value)
		elif kind == "real":
			param.setRealValue(value)
		elif kind == "str":
			param.setStrValue(value)
		else:
			raise RuntimeError("Unsupported stored solver option type")
	def _apply_stored_options(self, sim):
		store = self._solver_options.get(self._simkey(sim), self._default_solver_options)
		if not store:
			return
		PP = sim.getParameters()
		for P in PP:
			name = P.getName()
			if name in store:
				self._apply_parameter_value(P, store[name])
		sim.setParameters(PP)
	def _build_for_target(self, sim, inst):
		if inst is None:
			sim.build()
			return
		try:
			target = ascpy.Registry().getInstance("slvreq_target")
			sim.build(target)
		except Exception:
			try:
				sim.build(inst)
			except TypeError:
				sim.build(ascpy.Instance(inst))
	def _set_solver_param(self, sim, optionname, val, remember=True):
		try:
			PP = sim.getParameters()
		except Exception:
			return
		try:
			for P in PP:
				if P.getName() == optionname:
					if isinstance(val, ascpy.Value):
						P.setValueValue(val)
					elif P.isInt():
						P.setIntValue(val)
					elif P.isBool():
						P.setBoolValue(val)
					elif P.isReal():
						P.setRealValue(val)
					elif P.isStr():
						P.setStrValue(val)
					else:
						return
					sim.setParameters(PP)
					if remember:
						stored = self._snapshot_parameter(P)
						self._get_option_store(sim)[optionname] = stored
						self._default_solver_options[optionname] = stored
					return
		except Exception:
			return
	def setSolver(self,solvername,sim):
		self._solver_name[self._simkey(sim)] = solvername
		self._default_solver_name = solvername
		sim.setSolver(ascpy.Solver(solvername))
		if solvername.lower() == "highs":
			# Avoid GUI crashes from high-frequency progress callbacks.
			self._set_solver_param(sim, "progress_callbacks", False)
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
						stored = self._snapshot_parameter(P)
						self._get_option_store(sim)[optionname] = stored
						self._default_solver_options[optionname] = stored
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
			self._build_for_target(sim, inst)
			solvername = self._solver_name.get(self._simkey(sim), self._default_solver_name)
			if solvername is not None:
				sim.setSolver(ascpy.Solver(solvername))
			self._apply_stored_options(sim)
			print("PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName())
			sim.solve(sim.getSolver(),ascpy.SolverReporter())
		except Exception as e:
			print("PYTHON ERROR:",str(e))
			return 3
		return 0
	def doStudy(self,request,sim):
		print("PYTHON: STUDY is not implemented for this solver hook")
		return SLVREQ_NOT_IMPLEMENTED
	def deleteSystem(self, sim):
		try:
			sim.invalidateSystem()
		except Exception as e:
			print("PYTHON ERROR:",str(e))
			return 1
		return 0

class SolverHooksPythonBrowser(SolverHooksPython):
	def __init__(self,browser):
		self.browser = browser
		self.solve_interrupt = False
		SolverHooksPython.__init__(self)
	def doSolve(self,inst,sim):
		try:
			self._build_for_target(sim, inst)
			solvername = self._solver_name.get(self._simkey(sim), self._default_solver_name)
			if solvername is not None:
				sim.setSolver(ascpy.Solver(solvername))
			self._apply_stored_options(sim)
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
	def _get_study_observer(self):
		if self.browser.currentobservertab is None or self.browser.currentobservertab not in self.browser.tabs:
			observer = self.browser.create_observer()
		else:
			observer = self.browser.tabs[self.browser.currentobservertab]
		try:
			self.browser.maintabs.set_current_page(observer.tab)
		except Exception:
			pass
		return observer

	def doStudy(self, request, sim):
		try:
			observer = self._get_study_observer()
			for inst in request.getObserved():
				observer.add_instance(inst)
			observer.sync()

			if request.hasFilename():
				self.browser.reporter.reportNote(
					"STUDY FILE output is not implemented in the GTK browser; results remain in the active Observer."
				)

			if not request.hasVary() or request.getMode() == SLVREQ_STUDY_NONE:
				self.browser.reporter.reportNote("Observer populated from METHOD STUDY.")
				return 0

			dia = StudyWin(self.browser, request.getVary())
			dia.configure_from_request(request)
			if request.getNow():
				if not dia.run_now():
					return 1
				return 0
			dia.run()
			return 0
		except Exception as e:
			print("PYTHON ERROR:", str(e))
			return 1
	def deleteSystem(self, sim):
		try:
			sim.invalidateSystem()
			if hasattr(self.browser, "disable_on_sim_delete"):
				self.browser.disable_on_sim_delete()
		except Exception as e:
			print("PYTHON ERROR:", str(e))
			return 1
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
			ascpy.setSolverProgressReporter(reporter)
			ascpy.setSolverInterrupt(False)
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
		finally:
			try:
				ascpy.setSolverInterrupt(False)
				ascpy.setSolverProgressReporter(None)
			except Exception:
				pass
