import argparse
import pathlib
import re
import sys
import time

from plotutils import COLOR_CYCLE, group_series, group_ylabel

DEFAULT_INTEGRATOR = "IDA"
DEFAULT_DURATION = 100.0
DEFAULT_STEPS = 30


def build_progress_reporter(ascpy, progress_delay=10.0, progress_interval=5.0):
	class ConsoleProgressReporter(ascpy.SolverReporter):
		def __init__(self):
			self._start = time.perf_counter()
			self._last_emit = 0.0
			self._progress_delay = max(0.0, float(progress_delay))
			self._progress_interval = max(0.2, float(progress_interval))
			ascpy.SolverReporter.__init__(self)

		def _elapsed(self):
			return time.perf_counter() - self._start

		def _should_emit(self):
			elapsed = self._elapsed()
			if elapsed < self._progress_delay:
				return False
			if (elapsed - self._last_emit) < self._progress_interval:
				return False
			self._last_emit = elapsed
			return True

		def report(self, status):
			if self._should_emit():
				parts = [f"t={self._elapsed():.1f}s"]
				try:
					if status.isMIP():
						if status.hasMipNodeCount():
							parts.append(f"nodes={status.getMipNodeCount()}")
						if status.hasMipGap():
							parts.append(f"gap={status.getMipGap():.4g}")
						if status.hasMipPrimalBound():
							parts.append(f"primal={status.getMipPrimalBound():.6g}")
						if status.hasMipDualBound():
							parts.append(f"dual={status.getMipDualBound():.6g}")
					elif status.hasLpObjective():
						parts.append(f"obj={status.getLpObjective():.6g}")
				except Exception:
					pass
				sys.stderr.write("progress: %s\n" % ", ".join(parts))
				sys.stderr.flush()
			return False

		def finalise(self, status):
			elapsed = self._elapsed()
			if elapsed >= self._progress_delay:
				sys.stderr.write(f"progress: finished in {elapsed:.1f}s\n")
				sys.stderr.flush()

		def reportProgress(self, solver_name, message):
			if self._should_emit():
				msg = str(message).strip()
				if msg:
					sys.stderr.write(f"progress: [{solver_name}] {msg}\n")
					sys.stderr.flush()

	return ConsoleProgressReporter()


def _print_requested_vars(sim, printvars):
	re1 = re.compile(r"^[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*(\.[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*)*$")
	for varname in printvars:
		if not re1.match(varname):
			raise RuntimeError(f"Requested variable name '{varname}' does not match allowable pattern.")
		var = eval(f"sim.{varname}")
		print(f"{var} = {var.getValue()}")


def _print_default_study_vars(sim):
	hooks = sim.getSolverHooks()
	if hooks is None:
		return
	for var in hooks.getStudyPrintVars(sim):
		print(f"{sim.getInstanceName(var)} = {var.getValue()}")


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


def _print_simstatus(sim):
	state = "solved"
	parts = []
	if sim.isMethodRunning():
		state = "running-method"
	elif sim.isSolveDirty():
		state = "dirty"
	parts.append(f"state={state}")
	try:
		target = sim.getSolveTargetName()
	except Exception:
		target = ""
	if target:
		parts.append(f"target={target}")
	try:
		parts.append(f"solver={sim.getSolver().getName()}")
	except Exception:
		pass
	try:
		parts.append(f"solver_status={_status_label(sim.getStatus())}")
	except Exception:
		pass
	print("STATUS: " + ", ".join(parts))


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
	def __init__(self, ascpy, sim, integrator):
		class _Reporter(ascpy.IntegratorReporterCxx):
			def __init__(self, owner, wrapped):
				self._owner = owner
				ascpy.IntegratorReporterCxx.__init__(self, wrapped)

			def initOutput(self):
				try:
					self._owner._capture_columns()
					return 1
				except Exception as e:
					sys.stderr.write(f"runmodel.py: integrator initOutput failed: {e}\n")
					return 0

			def updateStatus(self):
				return 1

			def recordObservedValues(self):
				try:
					self._owner._capture_row()
					return 1
				except Exception as e:
					sys.stderr.write(f"runmodel.py: integrator recordObservedValues failed: {e}\n")
					return 0

			def closeOutput(self):
				return 0

		self.ascpy = ascpy
		self.sim = sim
		self.integrator = integrator
		self.columns = []
		self.rows = []
		self.reporter = _Reporter(self, integrator)

	def _capture_columns(self):
		if self.columns:
			return
		nobs = self.integrator.getNumObservedItems()
		for i in range(nobs):
			inst = self.integrator.getObservedInstance(i)
			units_name, _ = _get_units_info(inst)
			self.columns.append({
				"index": i,
				"instance": inst,
				"label": self.sim.getInstanceName(inst),
				"units": units_name if units_name != "1" else "",
				"is_real": inst.isReal(),
			})

	def _capture_row(self):
		self._capture_columns()
		indep = self.integrator.getIndependentVariable().getInstance()
		_, conversion = _get_units_info(indep)
		time_raw = self.integrator.getCurrentTime()
		row = {
			"time_raw": time_raw,
			"time": time_raw / conversion,
			"values": [_display_value(col["instance"]) for col in self.columns],
			"event": False,
		}
		self.rows.append(row)

	def build_report(self, microstates):
		indep = self.integrator.getIndependentVariable().getInstance()
		time_units, _ = _get_units_info(indep)
		time_label = self.integrator.getIndependentVariable().getName()
		if time_units and time_units != "1":
			time_label += f" [{time_units}]"
		rows = list(self.rows)
		_mark_event_rows(rows)
		rows = _filter_rows(rows, microstates)
		return {
			"time_label": time_label,
			"columns": self.columns,
			"rows": rows,
		}


class CliSolverHooks:
	def __init__(self, ascpy, suppress_integrate=False):
		class _Hooks(ascpy.SolverHooks):
			def __init__(self, owner):
				self._owner = owner
				ascpy.SolverHooks.__init__(self)

			def setIntegrator(self, integratorname, sim):
				res = ascpy.SolverHooks.setIntegrator(self, integratorname, sim)
				if res == 0:
					self._owner.integrator_name = integratorname
				return res

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
				start = request.getStart()
				stop = request.getStop()
				steps = request.getSteps()
				_run_integration(
					ascpy=self._owner.ascpy,
					sim=sim,
					engine=self._owner.integrator_name or DEFAULT_INTEGRATOR,
					start=start,
					duration=stop - start,
					steps=steps,
					units_token=None,
					output=None,
					plot=False,
					microstates="endpoints",
				)
				self._owner.integrated = True
				return 0

		self.ascpy = ascpy
		self.suppress_integrate = suppress_integrate
		self.integrator_name = None
		self.integrate_request = None
		self.integrated = False
		self.saw_integrate_request = False
		self.hooks = _Hooks(self)

	def did_integrate(self, sim):
		return self.integrated

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


def _run_integration(ascpy, sim, engine, start, duration, steps, units_token, output, plot, microstates):
	sim.build()
	integrator = ascpy.Integrator(sim)
	integrator.setEngine(engine or DEFAULT_INTEGRATOR)
	integrator.findIndependentVar()
	indep = integrator.getIndependentVariable()
	indep_inst = indep.getInstance()
	indep_units = indep_inst.getDisplayUnits(False)

	start_value = 0.0 if start is None else start
	duration_value = DEFAULT_DURATION if duration is None else duration
	steps_value = DEFAULT_STEPS if steps is None else steps
	if steps_value < 1:
		raise RuntimeError("Integration steps must be at least 1.")

	if units_token is not None:
		units_name = units_token
		units = ascpy.Units(units_token)
	else:
		units = indep_units
		units_name = units.getName().toString()

	integrator.setLinearTimesteps(units, start_value, start_value + duration_value, steps_value)
	_configure_integrator_observed(sim, integrator)
	reporter = CliIntegratorReporter(ascpy, sim, integrator)
	integrator.setReporter(reporter.reporter)
	integrator.analyse()

	if integrator.getNumObservedItems() == 0 and integrator.getNumObservedVars() == 0:
		raise RuntimeError("Integration requested but no observed variables are defined. Add OBSERVE statements or obs_id defaults.")

	reporter._capture_columns()
	integrator.solve()
	report = reporter.build_report(microstates)

	headers = [report["time_label"]]
	headers.extend(
		col["label"] + (f" [{col['units']}]" if col["units"] else "")
		for col in report["columns"]
	)
	table_rows = [[row["time"]] + row["values"] for row in report["rows"]]
	_print_table(headers, table_rows)

	if output is not None:
		_write_tsv(output, headers, table_rows)

	if plot:
		_plot_rows(report)

	if units_token is None and units_name:
		print(f"NOTE: integration bounds interpreted in independent-variable display units '{units_name}'.")


def run_ascend_model(
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
	output=None,
	plot=False,
	microstates="endpoints",
	progress_delay=10.0,
	progress_interval=5.0,
):
	"""
	Run an ASCEND model from the command line.

	If the model's own methods already performed a solve/integration during
	`on_load`, no extra steady QRSlv solve is forced afterward.
	"""

	import platform
	if platform.system() == "Windows":
		import os
		os.add_dll_directory(pathlib.Path(__file__).parent.parent)
	import ascpy
	old_hooks = ascpy.SolverHooksManager.Instance().getHooks()
	cli_hooks = CliSolverHooks(ascpy, suppress_integrate=integrate)
	ascpy.SolverHooksManager.Instance().setHooks(cli_hooks.hooks)

	try:
		L = ascpy.Library()
		L.load(str(filen))
		if model is None:
			model = filen.stem
		try:
			T = L.findType(model)
		except RuntimeError as e:
			print(e)
			from pathlib import Path
			for M in L.getModules():
				if Path(M.getFilename()) == Path(filen):
					print(f"Module {M.getFilename()} contains:")
					for m in L.getModuleTypes(M):
						print(f"  {m}")
			sys.exit(2)

		M = T.getSimulation("sim", True)
		if runmethod is not None:
			M.run(_find_method(T, runmethod))

		if integrate:
			request_defaults = cli_hooks.get_integrate_request(M)
			effective_engine = engine
			if effective_engine is None and request_defaults is not None:
				effective_engine = cli_hooks.integrator_name
			effective_start = start
			effective_duration = duration
			effective_steps = steps
			if request_defaults is not None:
				if effective_start is None:
					effective_start = request_defaults["start"]
				if effective_duration is None:
					effective_duration = request_defaults["stop"] - request_defaults["start"]
				if effective_steps is None:
					effective_steps = request_defaults["steps"]
			_run_integration(
				ascpy=ascpy,
				sim=M,
				engine=effective_engine,
				start=effective_start,
				duration=effective_duration,
				steps=effective_steps,
				units_token=units,
				output=output,
				plot=plot,
				microstates=microstates,
			)
		elif cli_hooks.did_integrate(M):
			pass
		elif _needs_final_solve(M):
			try:
				solver = M.getSolver()
			except RuntimeError:
				solver = ascpy.Solver("QRSlv")
			M.solve(
				solver,
				build_progress_reporter(
					ascpy,
					progress_delay=progress_delay,
					progress_interval=progress_interval,
				),
			)

		if printvars is not None:
			test = False
			_print_requested_vars(M, printvars)
		elif not integrate and not cli_hooks.did_integrate(M):
			_print_default_study_vars(M)

		if test and not integrate and not cli_hooks.did_integrate(M):
			try:
				self_test = _find_optional_method(T, "self_test")
				if self_test is not None:
					M.run(self_test)
			except Exception as e:
				raise RuntimeError(f"While attempting to run 'self_test': {str(e)}")

		_print_simstatus(M)
	finally:
		ascpy.SolverHooksManager.Instance().setHooks(old_hooks)


if __name__ == "__main__":
	p = argparse.ArgumentParser(description="Solve or integrate ASCEND models via the command line.")
	p.add_argument("file", type=pathlib.Path, help="ASCEND model file to be opened")
	p.add_argument("--model", "-m", help="Name of MODEL to instantiate (defaults to filename without extension)")
	p.add_argument("-r", "--run-method", dest="runmethod", help="Run METHOD after 'on_load' and before the final action")
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
	p.add_argument("--progress-delay", type=float, default=10.0, help="Seconds before emitting solver progress (default: 10).")
	p.add_argument("--progress-interval", type=float, default=5.0, help="Minimum seconds between progress lines (default: 5).")
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
			integrate=_is_integrate_requested(args),
			engine=args.engine,
			start=args.start,
			duration=args.duration,
			steps=args.steps,
			units=args.units,
			output=args.output,
			plot=args.plot,
			microstates=args.microstates,
			progress_delay=args.progress_delay,
			progress_interval=args.progress_interval,
		)
		sys.exit(0)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
