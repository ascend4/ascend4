import pathlib, sys, argparse, re, time


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

def run_ascend_model(
	filen,
	model=None,
	printvars=None,
	test=True,
	progress_delay=10.0,
	progress_interval=5.0
):
	"""
	This function (and the associated command-line argument parser) is for
	easing the job of quickly running ASCEND models from the command line.
	
	`filen`: name of the .a4c/.a4l model file to load.
	`model`: name of the model to instantiate. Defaults to the filename stem.
	`printvars`: a list of variable names which, if present, will be printed out (in dev). Forces test=False.
	`test`: whether or not to run the `self_test` method, if it exists. Defaults true.
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
	try:
		solver = M.getSolver()
	except RuntimeError:
		solver = ascpy.Solver("QRSlv")
	M.solve(solver,build_progress_reporter(
		ascpy,
		progress_delay=progress_delay,
		progress_interval=progress_interval
	))
	
	if printvars is not None:
		test = False
		re1 = re.compile(r"^[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*(\.[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*)*$")
		for varname in printvars:
			if not re1.match(varname):
				raise RuntimeError(f"Requested variable name '{varname}' does not match allowable pattern.")
			var = eval(f"M.{varname}")
			print(f"{var} = {var.getValue()}")
	
	if test:
		try:
			for meth in T.getMethods():
				if meth.getName() == "self_test":
					M.run(meth)
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
	p.add_argument('-p', '--print', dest='printvars', action='extend', nargs='+', help='Variables to print (can be used multiple times). Implies --no-test.')
	p.add_argument('--no-test','-n',action='store_false', help="Suppress running of 'self_test' method after solving");
	p.add_argument('--progress-delay', type=float, default=10.0, help='Seconds before emitting solver progress (default: 10).')
	p.add_argument('--progress-interval', type=float, default=5.0, help='Minimum seconds between progress lines (default: 5).')
	args = p.parse_args()
		
	#print("sys.argv =",sys.argv)
	try:
		run_ascend_model(
			filen=args.file,
			model=args.model,
			printvars=args.printvars,
			test=args.no_test,
			progress_delay=args.progress_delay,
			progress_interval=args.progress_interval
		)
		sys.exit(0)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
