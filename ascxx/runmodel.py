import argparse
import copy
import os
import pathlib
import sys
import weakref

from runresult import RunDiagnostic, RunError, RunResult
from runvalues import apply_overrides, resolve_instance

def _process_is_privileged():
	if not hasattr(os, "getuid"):
		return False
	return (
		os.getuid() == 0 or os.geteuid() == 0
		or os.getgid() == 0 or os.getegid() == 0
		or os.getuid() != os.geteuid()
		or os.getgid() != os.getegid()
	)

if _process_is_privileged():
	sys.exit("ASCEND refuses to run with root or mismatched effective user/group IDs.")

from plotutils import COLOR_CYCLE, group_series, group_ylabel

DEFAULT_INTEGRATOR = "IDA"
DEFAULT_DURATION = 100.0
DEFAULT_STEPS = 30

def _requested_vars(sim, printvars):
	for varname in printvars:
		yield varname, resolve_instance(sim, varname)


def _default_study_vars(sim):
	hooks = sim.getSolverHooks()
	if hooks is None:
		return
	for var in hooks.getStudyPrintVars(sim):
		yield str(sim.getInstanceName(var)), var


def _find_method(model_type, method_name):
	for meth in model_type.getMethods():
		if meth.getName() == method_name:
			return meth
	raise RuntimeError(f"Method '{method_name}' not found.")


def _find_optional_method(model_type, method_name):
	for meth in model_type.getMethods():
		if meth.getName() == method_name:
			return meth
	return None


def _method_names(methods):
	return [methods] if isinstance(methods, str) else (methods or [])


def _run_named_method(sim, name):
	"""Run a root or qualified submodel METHOD using the active solver hooks."""
	if "." not in name:
		sim.run(_find_method(sim.getType(), name))
		return
	path, method = name.rsplit(".", 1)
	target = resolve_instance(sim, path)
	if not target.isModel():
		raise ValueError(f"METHOD target {path!r} is not a model")
	import ascpy
	ascpy.SolverHooksManager.Instance().getHooks().assign(sim)
	sim.run(_find_method(target.getType(), method), target)


def _needs_final_solve(sim):
	return sim.isSolveDirty()


def _status_label(status):
	if status.isConverged():
		return "converged"
	if status.isReadyToSolve():
		return "ready"
	if status.isDiverged():
		return "diverged"
	if status.isInterrupted():
		return "interrupted"
	if status.hasExceededTimeLimit():
		return "time-limit"
	if status.hasExceededIterationLimit():
		return "iteration-limit"
	if status.hasResidualCalculationErrors():
		return "residual-errors"
	if status.isOverDefined():
		return "over-defined"
	if status.isUnderDefined():
		return "under-defined"
	return "not-converged"


def _simulation_status(sim):
	state = "solved"
	parts = {}
	if sim.isMethodRunning():
		state = "running-method"
	elif sim.isSolveDirty():
		state = "dirty"
	parts["state"] = state
	try:
		target = sim.getSolveTargetName()
	except Exception:
		target = ""
	if target:
		parts["target"] = str(target)
	try:
		parts["solver"] = str(sim.getSolver().getName())
	except Exception:
		pass
	try:
		parts["solver_status"] = _status_label(sim.getStatus())
	except Exception:
		pass
	return parts


class CliSolverReporter:
	def __init__(self, ascpy, sim=None, stream=None):
		class _Reporter(ascpy.SolverReporter):
			def __init__(self, owner):
				self._owner = weakref.proxy(owner)
				ascpy.SolverReporter.__init__(self)

			def report(self, status):
				return self._owner.report(status)

			def finalise(self, status):
				self._owner.finalise(status)

			def reportProgress(self, solver_name, message):
				self._owner.report_progress(solver_name, message)

		self.ascpy = ascpy
		self.sim = sim
		self.stream = stream if stream is not None else sys.stdout
		self.reporter = _Reporter(self)

	def set_sim(self, sim):
		self.sim = sim

	def report(self, status):
		try:
			iter_num = status.getIterationNum()
		except Exception:
			iter_num = None
		try:
			label = _status_label(status)
		except Exception:
			label = "unknown"
		parts = [f"solver_status={label}"]
		if iter_num is not None:
			parts.append(f"iter={iter_num}")
		print("SOLVER_STATUS: " + ", ".join(parts), file=self.stream)
		return 0

	def finalise(self, status):
		try:
			label = _status_label(status)
		except Exception:
			label = "unknown"
		print(f"SOLVER_FINAL: solver_status={label}", file=self.stream)

	def report_progress(self, solver_name, message):
		prefix = f"SOLVER_PROGRESS: solver={solver_name}" if solver_name else "SOLVER_PROGRESS:"
		print(f"{prefix}, {message}", file=self.stream)


def _same_time(a, b):
	scale = max(1.0, abs(a), abs(b))
	return abs(a - b) <= 1e-12 * scale


def _is_integrate_requested(args):
	return bool(
		args.integrate
		or args.engine is not None
		or args.start is not None
		or args.duration is not None
		or args.steps is not None
		or args.units is not None
		or args.output is not None
		or args.plot
		or args.microstates != "endpoints"
	)


def _get_units_info(inst):
	try:
		units = inst.getDisplayUnits(False)
		name = units.getName().toString()
		conversion = units.getConversion()
		if conversion == 0:
			conversion = 1.0
		return name, conversion
	except Exception:
		return "", 1.0


def _display_label(inst, sim):
	label = sim.getInstanceName(inst)
	if inst.isReal():
		units_name, _ = _get_units_info(inst)
		if units_name and units_name != "1":
			label += f" [{units_name}]"
	return label


def _display_value(inst):
	if inst.isReal():
		_, conversion = _get_units_info(inst)
		return inst.getRealValue() / conversion
	if inst.isBool():
		return "TRUE" if inst.getBoolValue() else "FALSE"
	if inst.isInt():
		return inst.getIntValue()
	if inst.isSelector():
		return "'" + str(inst.getSelectorValue()) + "'"
	if inst.isSymbol():
		return "'" + str(inst.getSymbolValue()) + "'"
	return inst.getValueAsString()


def _value_kind(inst):
	for name, check in (("real", inst.isReal), ("boolean", inst.isBool),
		("integer", inst.isInt), ("selector", inst.isSelector), ("symbol", inst.isSymbol)):
		if check():
			return name
	return "string"


def _typed_value(inst):
	if inst.isBool():
		return bool(inst.getBoolValue())
	if inst.isSelector():
		return str(inst.getSelectorValue())
	if inst.isSymbol():
		return str(inst.getSymbolValue())
	return _display_value(inst)


def _snapshot_scalar(inst):
	units, _ = _get_units_info(inst) if inst.isReal() else ("", 1.0)
	return {"value": _typed_value(inst), "units": units if units != "1" else "",
		"kind": _value_kind(inst), "display": str(inst.getValue())}


def _render_value(value, column):
	if column.get("kind") in ("symbol", "selector"):
		return "'" + value + "'"
	return value


def _format_cell(value):
	if isinstance(value, bool):
		return "TRUE" if value else "FALSE"
	if isinstance(value, int):
		return str(value)
	if isinstance(value, float):
		return f"{value:.15g}"
	return str(value)


def _mark_event_rows(rows):
	for row in rows:
		row["event"] = False
	for i in range(1, len(rows)):
		if _same_time(rows[i]["time_raw"], rows[i - 1]["time_raw"]):
			rows[i]["event"] = True
			rows[i - 1]["event"] = True


def _filter_rows(rows, microstates):
	if microstates == "all":
		return list(rows)
	if microstates == "endpoints":
		filtered = []
		i = 0
		while i < len(rows):
			j = i + 1
			while j < len(rows) and _same_time(rows[j]["time_raw"], rows[i]["time_raw"]):
				j += 1
			group = rows[i:j]
			filtered.append(group[0])
			if len(group) > 1 and group[-1]["values"] != group[0]["values"]:
				filtered.append(group[-1])
			i = j
		return filtered
	filtered = []
	i = 0
	while i < len(rows):
		j = i + 1
		while j < len(rows) and _same_time(rows[j]["time_raw"], rows[i]["time_raw"]):
			j += 1
		filtered.append(rows[j - 1])
		i = j
	return filtered


def _print_table(headers, rows):
	table_rows = [[_format_cell(v) for v in row] for row in rows]
	widths = [len(h) for h in headers]
	for row in table_rows:
		for i, cell in enumerate(row):
			if len(cell) > widths[i]:
				widths[i] = len(cell)

	def render(row):
		return "  ".join(str(cell).rjust(widths[i]) for i, cell in enumerate(row))

	print(render(headers))
	print("  ".join("-" * width for width in widths))
	for row in table_rows:
		print(render(row))


def _write_tsv(path, headers, rows):
	with open(path, "w", encoding="utf-8") as fp:
		fp.write("\t".join(headers) + "\n")
		for row in rows:
			fp.write("\t".join(_format_cell(v) for v in row) + "\n")


def _plot_rows(report):
	try:
		import matplotlib.pyplot as plt
	except Exception as e:
		raise RuntimeError(f"Plotting requested but matplotlib is unavailable ({e})")

	real_columns = [col for col in report["columns"] if col["is_real"]]
	if not real_columns:
		raise RuntimeError("No real-valued observed variables are available for plotting.")

	grouped, order = group_series(real_columns, lambda col: col["units"] if col["units"] else "")
	fig, axes = plt.subplots(len(order), 1, squeeze=False, sharex=True)
	axes = [ax[0] for ax in axes]
	x_all = [row["time"] for row in report["rows"]]
	event_rows = [row for row in report["rows"] if row.get("event")]

	for ax, group in zip(axes, order):
		entries = grouped[group]
		for i, col in enumerate(entries):
			y_all = [row["values"][col["index"]] for row in report["rows"]]
			color = COLOR_CYCLE[i % len(COLOR_CYCLE)]
			ax.plot(x_all, y_all, "-", color=color, label=col["label"])
			if event_rows:
				x_evt = [row["time"] for row in event_rows]
				y_evt = [row["values"][col["index"]] for row in event_rows]
				ax.plot(x_evt, y_evt, "o", ms=5, mfc="none", mec=color, linestyle="None")
		ax.set_ylabel(group_ylabel(entries, lambda col: col["units"], lambda col: col["label"]))
		ax.grid(True)
		if len(entries) > 1:
			ax.legend(loc="best")

	axes[-1].set_xlabel(report["time_label"])
	plt.tight_layout()
	plt.show()


class CliIntegratorReporter:
	def __init__(self, ascpy, sim, integrator, progress=False, stream=None):
		class _Reporter(ascpy.IntegratorReporterCxx):
			def __init__(self, owner, wrapped):
				self._owner = weakref.proxy(owner)
				ascpy.IntegratorReporterCxx.__init__(self, wrapped)

			def initOutput(self):
				try:
					self._owner._capture_columns()
					return 1
				except Exception as e:
					self._owner._callback_error("initOutput", e)
					return 0

			def updateStatus(self):
				try:
					self._owner._report_progress()
					return 1
				except Exception as e:
					self._owner._callback_error("updateStatus", e)
					return 0

			def recordObservedValues(self):
				try:
					self._owner._capture_row()
					return 1
				except Exception as e:
					self._owner._callback_error("recordObservedValues", e)
					return 0

			def closeOutput(self):
				try:
					self._owner._report_final()
				except Exception as e:
					self._owner._callback_error("closeOutput", e)
				return 0

		self.ascpy = ascpy
		self.sim = sim
		self.integrator = integrator
		self.progress = progress
		self.stream = stream if stream is not None else sys.stdout
		self.columns = []
		self.rows = []
		self.diagnostics = []
		self.time_label = "time"
		self.time_units = ""
		self.time_conversion = 1.0
		self.reporter = _Reporter(self, integrator)

	def _callback_error(self, callback, error):
		# Do not throw through a native integrator callback. Some engines ignore
		# callback return values, so execution also checks these diagnostics.
		self.diagnostics.append(RunDiagnostic(
			"reporter." + callback, str(error), exception_type=type(error).__name__))

	def _report_progress(self):
		if not self.progress:
			return
		parts = [f"engine={self.integrator.getName()}"]
		try:
			parts.append(f"step={self.integrator.getCurrentStep()}/{self.integrator.getNumSteps()}")
		except Exception:
			pass
		try:
			parts.append(f"t={self.integrator.getCurrentTime():.17g}")
		except Exception:
			pass
		print("INTEGRATOR_PROGRESS: " + ", ".join(parts), file=self.stream)

	def _report_final(self):
		if not self.progress:
			return
		parts = [f"engine={self.integrator.getName()}"]
		try:
			parts.append(f"step={self.integrator.getCurrentStep()}/{self.integrator.getNumSteps()}")
		except Exception:
			pass
		try:
			parts.append(f"t={self.integrator.getCurrentTime():.17g}")
		except Exception:
			pass
		print("INTEGRATOR_FINAL: " + ", ".join(parts), file=self.stream)

	def _capture_columns(self):
		if self.columns:
			return
		independent = self.integrator.getIndependentVariable()
		self.time_units, self.time_conversion = _get_units_info(independent.getInstance())
		self.time_label = str(independent.getName())
		if self.time_units and self.time_units != "1":
			self.time_label += f" [{self.time_units}]"
		nobs = self.integrator.getNumObservedItems()
		for i in range(nobs):
			inst = self.integrator.getObservedInstance(i)
			units_name, conversion = _get_units_info(inst) if inst.isReal() else ("", 1.0)
			self.columns.append({
				"index": i,
				"instance": inst,
				"label": str(self.sim.getInstanceName(inst)),
				"units": units_name if units_name != "1" else "",
				"is_real": inst.isReal(),
				"kind": _value_kind(inst),
				"conversion": conversion,
			})

	def _capture_row(self):
		self._capture_columns()
		time_raw = self.integrator.getCurrentTime()
		row = {
			"time_raw": time_raw,
			"time": time_raw / self.time_conversion,
			"values": [col["instance"].getRealValue() / col["conversion"]
				if col["is_real"] else _typed_value(col["instance"]) for col in self.columns],
			"event": False,
		}
		self.rows.append(row)

	def build_report(self, microstates="all"):
		# A report is a detached snapshot, safe even after library.clear().
		# No native calls here: failed integration may leave the system unusable.
		rows = copy.deepcopy(self.rows)
		_mark_event_rows(rows)
		rows = _filter_rows(rows, microstates)
		return {
			"time_label": self.time_label,
			"time_units": self.time_units,
			"columns": [{k: v for k, v in col.items() if k != "instance"} for col in self.columns],
			"rows": rows,
		}


class CliSolverHooks:
	def __init__(self, ascpy, suppress_integrate=False, reporter=None, progress=False, render=True):
		class _Hooks(ascpy.SolverHooks):
			def __init__(self, owner):
				self._owner = weakref.proxy(owner)
				if owner.reporter is not None:
					ascpy.SolverHooks.__init__(self, owner.reporter.reporter)
				else:
					ascpy.SolverHooks.__init__(self)

			def doObserve(self, request, sim):
				return ascpy.SolverHooks.doObserve(self, request, sim)

			def doIntegrate(self, request, sim):
				self._owner.integrate_request = {
					"start": request.getStart(),
					"stop": request.getStop(),
					"steps": request.getSteps(),
				}
				self._owner.saw_integrate_request = True
				if self._owner.suppress_integrate:
					return 0
				result = _execute_integration(
					ascpy=self._owner.ascpy,
					sim=sim,
					engine=None,
					start=None,
					duration=None,
					steps=None,
					units_token=None,
					progress=self._owner.progress,
					request_defaults=self._owner.integrate_request,
				)
				self._owner.results.append(result)
				if self._owner.render:
					try:
						render_run_result(result, show_status=False)
					except Exception as error:
						result.fail("render", error)
				self._owner.integrated = self._owner.integrated or result.ok
				# SLVREQ_INTEGRATE_FAIL (ascend/compiler/slvreq.h). Keep Python
				# exceptions from escaping through the native METHOD callback.
				return 0 if result.ok else 3

		self.ascpy = ascpy
		self.suppress_integrate = suppress_integrate
		self.reporter = reporter
		self.progress = progress
		self.render = render
		self.results = []
		self.integrate_request = None
		self.integrated = False
		self.saw_integrate_request = False
		self.hooks = _Hooks(self)

	def did_integrate(self, sim):
		return self.integrated

	def saw_integrate(self, sim):
		return self.saw_integrate_request

	def get_integrate_request(self, sim):
		return self.integrate_request


def _configure_integrator_observed(sim, integrator):
	hooks = sim.getSolverHooks()
	if hooks is None:
		return
	try:
		observed = hooks.getObservedVars(sim)
	except Exception:
		return
	if not observed:
		return
	integrator.clearObservedInstances()
	for inst in observed:
		integrator.addObservedInstance(inst)


def _execute_integration(ascpy, sim, engine=None, start=None, duration=None,
		steps=None, units_token=None, progress=False, request_defaults=None):
	result = RunResult(action="integrate", phase="prepare")
	reporter = None
	try:
		sim.build()
		integrator = ascpy.Integrator(sim)
		hooks = sim.getSolverHooks()
		method_engine = hooks.getIntegratorName(sim) if hooks is not None else None
		integrator.setEngine(engine or method_engine or DEFAULT_INTEGRATOR)
		if hooks is not None:
			hooks.applyIntegratorOptions(sim, integrator)
		integrator.findIndependentVar()
		indep_inst = integrator.getIndependentVariable().getInstance()
		units = ascpy.Units(units_token) if units_token is not None else indep_inst.getDisplayUnits(False)
		units_name = str(units.getName())

		# METHOD bounds are base-unit values; CLI numbers use selected units.
		start_value = 0.0 if start is None else start
		duration_value = DEFAULT_DURATION if duration is None else duration
		steps_value = DEFAULT_STEPS if steps is None else steps
		if request_defaults is not None:
			conversion = units.getConversion()
			if start is None:
				start_value = request_defaults["start"] / conversion
			if duration is None:
				duration_value = (request_defaults["stop"] - request_defaults["start"]) / conversion
			if steps is None:
				steps_value = request_defaults["steps"]
		if steps_value < 1:
			raise RuntimeError("Integration steps must be at least 1.")
		integrator.setLinearTimesteps(units, start_value, start_value + duration_value, steps_value)
		_configure_integrator_observed(sim, integrator)
		reporter = CliIntegratorReporter(ascpy, sim, integrator, progress=progress)
		integrator.setReporter(reporter.reporter)
		result.phase = "analyse"
		integrator.analyse()
		if integrator.getNumObservedItems() == 0 and integrator.getNumObservedVars() == 0:
			raise RuntimeError("Integration requested but no observed variables are defined. Add OBSERVE statements or obs_id defaults.")
		reporter._capture_columns()
		result.phase = "integrate"
		integrator.solve()
		result.phase = "complete"
		if units_token is None and units_name and (request_defaults is None or start is not None or duration is not None):
			result.notes.append(f"integration bounds interpreted in independent-variable display units '{units_name}'.")
	except Exception as error:
		result.fail(result.phase, error)
	finally:
		if reporter is not None:
			result.diagnostics.extend(reporter.diagnostics)
			if reporter.diagnostics:
				result.status = "failed"
				result.phase = reporter.diagnostics[0].phase
			# This is pure Python snapshotting: retain even the initial/partial
			# rows after solve raises or a native engine ignores a callback error.
			if reporter.columns or reporter.rows:
				result.tables.append(reporter.build_report())
	return result


def execute_integration(sim, *, engine=None, start=None, duration=None, steps=None,
		units=None, progress=False):
	"""Integrate a caller-owned simulation and return a detached RunResult.

	No table printing, plotting or file writes. Numerical/setup failures are
	returned as failed results, with all recorded rows retained. The caller keeps
	ownership of the simulation; native diagnostics still use ASCEND's reporter.
	"""
	import ascpy
	return _execute_integration(ascpy, sim, engine, start, duration, steps, units, progress)


def render_run_result(result, *, output=None, plot=False, microstates="endpoints", show_status=True):
	"""Present detached data, without executing ASCEND or changing the result."""
	if microstates not in ("all", "none", "endpoints"):
		raise ValueError("Unknown microstate selection: " + str(microstates))
	if output is not None and len(result.tables) > 1:
		raise ValueError("A single output path cannot hold multiple integration tables")
	for table in result.tables:
		report = dict(table, rows=_filter_rows(table["rows"], microstates))
		headers = [report["time_label"]] + [
			col["label"] + (f" [{col['units']}]" if col["units"] else "")
			for col in report["columns"]]
		rows = [[row["time"]] + [_render_value(value, col)
			for value, col in zip(row["values"], report["columns"])] for row in report["rows"]]
		if output is not None:
			_write_tsv(output, headers, rows)
		else:
			_print_table(headers, rows)
		if plot:
			_plot_rows(report)
	for name, value in result.values.items():
		print(f"{name} = {value['display']}")
	for note in result.notes:
		print("NOTE: " + note)
	if show_status:
		parts = [f"{key}={value}" for key, value in result.simulation_status.items()]
		if not result.ok:
			parts.append("run_status=failed")
		if parts:
			print("STATUS: " + ", ".join(parts))


def _run_integration(ascpy, sim, engine, start, duration, steps, units_token, output, plot, microstates, progress=False, request_defaults=None):
	"""Compatibility wrapper for the original CLI integration helper."""
	result = _execute_integration(ascpy, sim, engine, start, duration, steps, units_token, progress, request_defaults)
	render_run_result(result, output=output, plot=plot, microstates=microstates, show_status=False)
	result.raise_for_status()
	return result


def execute_model(
	filen,
	model=None,
	printvars=None,
	test=True,
	runmethod=None,
	integrate=False,
	engine=None,
	start=None,
	duration=None,
	steps=None,
	units=None,
	progress=False,
	solver_progress=False,
	overrides=None,
	setup_methods=None,
	run_on_load=True,
	solve=True,
):
	"""
	Execute the existing model workflow and return a detached RunResult.

	If the model's own methods already performed a solve/integration during
	`on_load`, no extra steady QRSlv solve is forced afterward. This function
	does not print result tables or write files. It records execution failures
	in the result rather than exiting Python. Native diagnostics are unchanged.
	Library loading still uses ASCEND's process-global library.
	Order: optional on_load, setup_methods, overrides, runmethod(s), final action.
	solve=False disables only the implicit steady solve and self_test; explicit
	METHOD solves/integrations and integrate=True still execute.
	"""

	import platform
	if platform.system() == "Windows":
		import os
		os.add_dll_directory(pathlib.Path(__file__).parent.parent)
	import ascpy
	old_hooks = ascpy.SolverHooksManager.Instance().getHooks()
	progress = bool(progress or solver_progress)
	solver_reporter = CliSolverReporter(ascpy) if progress else None
	cli_hooks = CliSolverHooks(ascpy, suppress_integrate=integrate, reporter=solver_reporter, progress=progress, render=False)
	result = RunResult(action="integrate" if integrate else ("solve" if solve else "methods"), phase="load")
	M = None
	ascpy.SolverHooksManager.Instance().setHooks(cli_hooks.hooks)

	try:
		if solver_reporter is not None:
			ascpy.setSolverProgressReporter(solver_reporter.reporter)
		filen = pathlib.Path(filen)
		L = ascpy.Library()
		L.load(str(filen))
		if model is None:
			model = filen.stem
		result.phase = "lookup"
		try:
			T = L.findType(model)
		except RuntimeError:
			for module in L.getModules():
				if pathlib.Path(module.getFilename()) == filen:
					result.notes.append(f"Module {module.getFilename()} contains: " +
						", ".join(str(typ) for typ in L.getModuleTypes(module)))
			raise

		result.phase = "instantiate"
		M = T.getSimulation("sim", False)
		if solver_reporter is not None:
			solver_reporter.set_sim(M)
		if run_on_load:
			result.phase = "on_load"
			M.runDefaultMethod()
		for method in _method_names(setup_methods):
			result.phase = "setup:" + method
			_run_named_method(M, method)
		result.phase = "overrides"
		apply_overrides(M, overrides or [])
		for method in _method_names(runmethod):
			result.phase = "method:" + method
			_run_named_method(M, method)

		if integrate:
			result.phase = "integrate"
			request_defaults = cli_hooks.get_integrate_request(M)
			integration = _execute_integration(
				ascpy=ascpy,
				sim=M,
				engine=engine,
				start=start,
				duration=duration,
				steps=steps,
				units_token=units,
				progress=progress,
				request_defaults=request_defaults,
			)
			cli_hooks.results.append(integration)
			integration.raise_for_status()
		elif cli_hooks.did_integrate(M):
			pass
		elif solve and _needs_final_solve(M):
			result.phase = "solve"
			try:
				solver = M.getSolver()
			except RuntimeError:
				solver = ascpy.Solver("QRSlv")
			if solver_reporter is not None:
				M.solve(solver, solver_reporter.reporter)
			else:
				M.solve(solver, ascpy.SolverReporter())

		result.phase = "observe"
		if printvars is not None:
			test = False
			for name, inst in _requested_vars(M, printvars):
				result.values[name] = _snapshot_scalar(inst)
		elif not integrate and not cli_hooks.did_integrate(M):
			for name, inst in _default_study_vars(M):
				result.values[name] = _snapshot_scalar(inst)

		if test and solve and not integrate and not cli_hooks.did_integrate(M):
			result.phase = "self_test"
			try:
				self_test = _find_optional_method(T, "self_test")
				if self_test is not None:
					M.run(self_test)
			except Exception as e:
				raise RuntimeError(f"While attempting to run 'self_test': {str(e)}")

		result.phase = "complete"
	except RunError as error:
		# The integration's original error and partial rows are merged below.
		if not any(error.result is integration for integration in cli_hooks.results):
			result.fail(result.phase, error)
	except Exception as error:
		result.fail(result.phase, error)
	finally:
		for integration in cli_hooks.results:
			result.action = "integrate"
			result.tables.extend(integration.tables)
			result.diagnostics.extend(integration.diagnostics)
			result.notes.extend(integration.notes)
			if not integration.ok:
				result.status = "failed"
				result.phase = integration.phase
		if M is not None:
			try:
				result.simulation_status = _simulation_status(M)
			except Exception as error:
				result.diagnostics.append(RunDiagnostic("status", str(error), severity="warning"))
		if solver_reporter is not None:
			try:
				ascpy.setSolverProgressReporter(None)
			except Exception:
				pass
		ascpy.SolverHooksManager.Instance().setHooks(old_hooks)
		if M is not None:
			# Native callbacks must not retain our temporary Python hook object.
			try:
				old_hooks.assign(M)
				M.invalidateSystem()
			except Exception as error:
				result.fail("cleanup", error)
	return result


def run_ascend_model(filen, model=None, printvars=None, test=True, runmethod=None,
		integrate=False, engine=None, start=None, duration=None, steps=None, units=None,
		output=None, plot=False, microstates="endpoints", progress=False, solver_progress=False,
		overrides=None, setup_methods=None, run_on_load=True, solve=True):
	"""CLI-compatible execution/presentation; failures raise RunError with .result."""
	result = execute_model(filen, model=model, printvars=printvars, test=test,
		runmethod=runmethod, integrate=integrate, engine=engine, start=start,
		duration=duration, steps=steps, units=units, progress=progress, solver_progress=solver_progress,
		overrides=overrides, setup_methods=setup_methods, run_on_load=run_on_load, solve=solve)
	try:
		render_run_result(result, output=output, plot=plot, microstates=microstates)
	except Exception as error:
		result.fail("render", error)
	result.raise_for_status()
	return result


if __name__ == "__main__":
	p = argparse.ArgumentParser(description="Solve or integrate ASCEND models via the command line.",
		epilog="Execution order (independent of flag placement): on_load, setup methods, "
		"all --set overrides, run methods, final solve/integration, reporting/self_test.")
	p.add_argument("file", type=pathlib.Path, help="ASCEND model file to be opened")
	p.add_argument("--model", "-m", help="Name of MODEL to instantiate (defaults to filename without extension)")
	p.add_argument("--no-on-load", action="store_true", help="Skip the default on_load METHOD")
	p.add_argument("--setup-method", action="append", help="Run METHOD before overrides (repeatable, in supplied order)")
	p.add_argument("--set", dest="overrides", action="append", metavar="PATH=VALUE", help="Assign a scalar after setup, before run methods; e.g. 'T=873.15{K}'. Repeatable; does not FIX/FREE")
	p.add_argument("-r", "--run-method", dest="runmethod", action="append", help="Run METHOD after overrides and before the final action (repeatable, in supplied order)")
	p.add_argument("--no-solve", action="store_true", help="Skip the implicit final steady solve and self_test; explicit METHOD actions still execute")
	p.add_argument("-p", "--print", dest="printvars", action="append", nargs="+", help="Variables to print (can be used multiple times). Implies --no-test.")
	p.add_argument("--no-test", "-n", action="store_false", help="Suppress running of 'self_test' method after solving")
	p.add_argument("--integrate", "--int", "-i", action="store_true", help="Run via the integrator API instead of steady-state solve")
	p.add_argument("--engine", "-e", help=f"Integrator engine to use (default when integrating: {DEFAULT_INTEGRATOR})")
	p.add_argument("--start", "-s", type=float, help="Integration start value, in the selected units")
	p.add_argument("--duration", "-d", type=float, help=f"Integration duration in the selected units (default when integrating: {DEFAULT_DURATION:g})")
	p.add_argument("--steps", type=int, help=f"Number of output/reporting steps (default when integrating: {DEFAULT_STEPS})")
	p.add_argument("--units", "-u", help="Units token for integration bounds, eg 's' or 'h'")
	p.add_argument("--output", "-o", type=pathlib.Path, help="Write integration results as TSV to this file")
	p.add_argument("--plot", action="store_true", help="Plot observed real-valued variables against the independent variable after integration")
	p.add_argument("--progress", action="store_true", help="Print solver/integrator progress messages during runs")
	p.add_argument(
		"--microstates",
		nargs="?",
		choices=("none", "endpoints", "all"),
		const="all",
		default="endpoints",
		help="Include extra same-time event output rows. Default: endpoints; bare --microstates means all.",
	)
	args = p.parse_args()
	printvars = None
	if args.printvars:
		printvars = [name for group in args.printvars for name in group]

	try:
		run_ascend_model(
			filen=args.file,
			model=args.model,
			printvars=printvars,
			test=args.no_test,
			runmethod=args.runmethod,
			overrides=args.overrides,
			setup_methods=args.setup_method,
			run_on_load=not args.no_on_load,
			solve=not args.no_solve,
			integrate=_is_integrate_requested(args),
			engine=args.engine,
			start=args.start,
			duration=args.duration,
			steps=args.steps,
			units=args.units,
			output=args.output,
			plot=args.plot,
			microstates=args.microstates,
			progress=args.progress,
		)
		sys.exit(0)
	except RunError as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(2 if e.result.phase == "lookup" else 1)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
