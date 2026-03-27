import pathlib, sys, argparse, re

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

def _get_instance_units(inst):
	try:
		return inst.getDisplayUnits(False)
	except Exception:
		return inst.getType().getDeclaredUnits()

def _is_real_instance(inst):
	return inst.isReal()

def _get_instance_value(inst):
	if inst.isSelector():
		return str(inst.getSelectorValue())
	if inst.isSymbol():
		return str(inst.getSymbolValue())
	if inst.isBool():
		return bool(inst.getBoolValue())
	if inst.isInt():
		return int(inst.getIntValue())
	return inst.getRealValue()

def _get_integrator_observed_instances(sim, integrator):
	hooks = sim.getSolverHooks()
	if hooks is not None:
		observed = hooks.getObservedVars(sim)
		if len(observed):
			return observed
	return [integrator.getObservedInstance(i) for i in range(integrator.getNumObservedItems())]

def _get_integrator_output(sim, integrator):
	indep = integrator.getIndependentVariable()
	indep_units = _get_instance_units(indep.getInstance())
	headers = [f"{indep.getName()} [{indep_units.getName().toString()}]"]
	conversions = [indep_units.getConversion()]
	observed = [integrator.getObservedInstance(i) for i in range(integrator.getNumObservedItems())]
	for inst in observed:
		name = sim.getInstanceName(inst)
		if _is_real_instance(inst):
			units = _get_instance_units(inst)
			headers.append(f"{name} [{units.getName().toString()}]")
			conversions.append(units.getConversion())
		else:
			headers.append(f"{name}")
			conversions.append(None)

	rows = []
	if integrator.getNumObservedVars() == integrator.getNumObservedItems():
		for data in integrator.getObservations():
			obs_vals = list(data[:-1])
			indep_val = data[-1]
			row = [indep_val / conversions[0]]
			for i, value in enumerate(obs_vals):
				conv = conversions[i + 1]
				row.append(value / conv if conv not in (None, 0) else value)
			rows.append(row)
	return headers, rows

def _write_integrator_table(filep, headers, rows):
	filep.write("\t".join(headers) + "\n")
	for row in rows:
		filep.write("\t".join([_format_integrator_value(value) for value in row]) + "\n")

def _format_integrator_value(value):
	if isinstance(value, float):
		return f"{value:.15g}"
	if isinstance(value, bool):
		return "TRUE" if value else "FALSE"
	if isinstance(value, str):
		return value
	return str(value)

def integrate_ascend_model(filen,model=None,engine="IDA",start=None,duration=100.0,steps=30,units=None,output=None,plot=False):
	"""
	Run an ASCEND model through the integrator API from the command line.

	For now this path is explicit rather than slvreq-driven: the integration
	engine and bounds are supplied on the CLI.
	"""

	import platform
	if platform.system()=="Windows":
		import os,pathlib
		os.add_dll_directory(pathlib.Path(__file__).parent.parent)
	import ascpy

	class TabularIntegratorReporter(ascpy.IntegratorReporterCxx):
		def __init__(self, integrator, filep):
			ascpy.IntegratorReporterCxx.__init__(self, integrator)
			self.filep = filep
			self.headers = []
			self.conversions = []
			self.observed = []
			self.rows = []
		def initOutput(self):
			indep = self.getIntegrator().getIndependentVariable()
			indep_units = _get_instance_units(indep.getInstance())
			self.headers = [f"{indep.getName()} [{indep_units.getName().toString()}]"]
			self.conversions = [indep_units.getConversion()]
			self.observed = [self.getIntegrator().getObservedInstance(i) for i in range(self.getIntegrator().getNumObservedItems())]
			for inst in self.observed:
				if _is_real_instance(inst):
					units = _get_instance_units(inst)
					self.headers.append(f"{M.getInstanceName(inst)} [{units.getName().toString()}]")
					self.conversions.append(units.getConversion())
				else:
					self.headers.append(f"{M.getInstanceName(inst)}")
					self.conversions.append(None)
			self.filep.write("\t".join(self.headers) + "\n")
			self.filep.flush()
			return 1
		def closeOutput(self):
			self.filep.flush()
			return 0
		def updateStatus(self):
			return 1
		def recordObservedValues(self):
			I = self.getIntegrator()
			row = [I.getCurrentTime() / self.conversions[0]]
			for i, inst in enumerate(self.observed):
				value = _get_instance_value(inst)
				conv = self.conversions[i + 1]
				if conv not in (None, 0):
					value = value / conv
				row.append(value)
			self.rows.append(row)
			self.filep.write("\t".join([_format_integrator_value(value) for value in row]) + "\n")
			self.filep.flush()
			if I.getNumObservedVars() > 0:
				I.saveObservations()
			return 1

	L = ascpy.Library()
	L.load(str(filen))
	if model is None:
		model = filen.stem
	T = L.findType(model)
	M = T.getSimulation('sim',True)
	M.build()

	I = ascpy.Integrator(M)
	I.setEngine(engine)
	for inst in _get_integrator_observed_instances(M, I):
		I.addObservedInstance(inst)
	I.findIndependentVar()

	indep_inst = I.getIndependentVariable().getInstance()
	bounds_units = _get_instance_units(indep_inst) if units is None else ascpy.Units(units)
	if start is None:
		start = indep_inst.getRealValue() / bounds_units.getConversion()

	I.setLinearTimesteps(bounds_units, float(start), float(start) + float(duration), int(steps))
	I.analyse()

	if output is None and not plot:
		reporter = ascpy.IntegratorReporterConsole(I)
	elif output is None:
		reporter = TabularIntegratorReporter(I, sys.stdout)
	else:
		reporter = TabularIntegratorReporter(I, open(output, "w"))

	I.setReporter(reporter)
	I.solve()

	if plot:
		if isinstance(reporter, TabularIntegratorReporter):
			headers, rows = reporter.headers, reporter.rows
		else:
			headers, rows = _get_integrator_output(M, I)
		real_indices = [idx for idx, inst in enumerate(_get_integrator_observed_instances(M, I), start=1) if _is_real_instance(inst)]
		if len(real_indices) < 1:
			raise RuntimeError("No observed variables are available to plot.")
		import matplotlib.pyplot as plt
		x = [row[0] for row in rows]
		yindex = real_indices[0]
		y = [row[yindex] for row in rows]
		plt.plot(x, y, "-o")
		plt.xlabel(headers[0])
		plt.ylabel(headers[yindex])
		plt.grid(True)
		plt.show()

def run_ascend_model(filen,model=None,printvars=None,test=True,runmethod=None):
	"""
	This function (and the associated command-line argument parser) is for
	easing the job of quickly running ASCEND models from the command line.
	
	`filen`: name of the .a4c/.a4l model file to load.
	`model`: name of the model to instantiate. Defaults to the filename stem.
	`printvars`: a list of variable names which, if present, will be printed out (in dev). Forces test=False.
	`test`: whether or not to run the `self_test` method, if it exists. Defaults true.
	`runmethod`: optional method name to run after `on_load` and before the final solve.
	"""
	
	import platform
	if platform.system()=="Windows":
		import os,pathlib
		os.add_dll_directory(pathlib.Path(__file__).parent.parent)
	import ascpy
	L = ascpy.Library()
	#print(f"FILEN = {filen}")
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
		
	M = T.getSimulation('sim',True) # run default method = True
	if runmethod is not None:
		M.run(_find_method(T, runmethod))
	try:
		solver = M.getSolver()
	except RuntimeError:
		solver = ascpy.Solver("QRSlv")
	if _needs_final_solve(M):
		M.solve(solver,ascpy.SolverReporter())
	
	if printvars is not None:
		test = False
		_print_requested_vars(M, printvars)
	else:
		_print_default_study_vars(M)
	
	if test:
		try:
			self_test = _find_optional_method(T, "self_test")
			if self_test is not None:
				M.run(self_test)
		except Exception as e:
			raise RuntimeError(f"While attempting to run 'self_test': {str(e)}")

	_print_simstatus(M)
	
	# TODO: we can add a customised solverreporter here
	# TODO: we could also extend the user interface to support setting of solver parameters etc.
	# TODO: we could also implement setting of fixed model values and/or running parameter sweeps.
	# TODO: examine how SlvReq can be used to allow the model file to select its own solver.
	# TODO: what about integrator models?
	# TODO: implement a custom ErrorReporter to suppress ASC_USER_NOTE and ASC_PROG_NOTE by default.


if __name__=="__main__":
	p = argparse.ArgumentParser(description='Solve ASCEND models via the command line.')
	p.add_argument('file',type=pathlib.Path,help='ASCEND model file to be opened')
	p.add_argument('--model','-m', help="Name of MODEL to instantiate (defaults to filename without extension)");
	p.add_argument('--integrate','-i', action='store_true', help="Run via the integrator API instead of the steady-state solver.")
	p.add_argument('--engine','-e', default='IDA', help="Integrator engine to use with --integrate (default: IDA).")
	p.add_argument('--start','-s', type=float, help="Integration start value, in the selected units.")
	p.add_argument('--duration','-d', type=float, default=100.0, help="Integration duration, in the selected units (default: 100).")
	p.add_argument('--steps', type=int, default=30, help="Number of output/reporting steps for --integrate (default: 30).")
	p.add_argument('--units','-u', help="Units token for integration bounds, eg 's' or 'h'. Defaults to the independent variable display units.")
	p.add_argument('--output','-o', type=pathlib.Path, help="Write integration results as TSV to this file.")
	p.add_argument('--plot', action='store_true', help="Plot the first observed variable against the independent variable after integration.")
	p.add_argument('-r', '--run-method', dest='runmethod', help="Run METHOD after 'on_load' and before the final solve");
	p.add_argument('-p', '--print', dest='printvars', action='extend', nargs='+', help='Variables to print (can be used multiple times). Implies --no-test.')
	p.add_argument('--no-test','-n',action='store_false', help="Suppress running of 'self_test' method after solving");
	args = p.parse_args()
		
	#print("sys.argv =",sys.argv)
	try:
		if args.integrate:
			integrate_ascend_model(
				filen=args.file,
				model=args.model,
				engine=args.engine,
				start=args.start,
				duration=args.duration,
				steps=args.steps,
				units=args.units,
				output=args.output,
				plot=args.plot,
			)
		else:
			run_ascend_model(filen=args.file,model=args.model,printvars=args.printvars,test=args.no_test,runmethod=args.runmethod)
		sys.exit(0)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
