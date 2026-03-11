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
	p.add_argument('-r', '--run-method', dest='runmethod', help="Run METHOD after 'on_load' and before the final solve");
	p.add_argument('-p', '--print', dest='printvars', action='extend', nargs='+', help='Variables to print (can be used multiple times). Implies --no-test.')
	p.add_argument('--no-test','-n',action='store_false', help="Suppress running of 'self_test' method after solving");
	args = p.parse_args()
		
	#print("sys.argv =",sys.argv)
	try:
		run_ascend_model(filen=args.file,model=args.model,printvars=args.printvars,test=args.no_test,runmethod=args.runmethod)
		sys.exit(0)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
