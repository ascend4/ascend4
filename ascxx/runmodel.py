import ascpy, pathlib, sys

def run_ascend_model(filen,model=None):
	import ascpy
	L = ascpy.Library()
	L.load(filen)
	if model is None:
		model = pathlib.Path(filen).stem
	T = L.findType(model)
	M = T.getSimulation('sim')
	M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
	# TODO: we can add a customised solverreporter here
	# TODO: we could also extend the user interface to support setting of solver parameters etc.
	# TODO: we could also implement setting of model parameters and/or parameter sweeps.
	# TODO: we could also implement running 'self_test' methods after the model has solved.
	
if __name__=="__main__":
	print("sys.argv =",sys.argv)
	try:
		if len(sys.argv)>=2:
			filen = sys.argv[1]
			model = None
			if len(sys.argv)==3:
				model = sys.argv[2]
			elif len(sys.argv)>3:
				raise RuntimeError("Invalid number of command-line arguments received",sys.argv[0]);
			run_ascend_model(filen,model)
		else:
			raise RuntimeError("Invalid number of command-line arguments received",sys.argv[0]);
		sys.exit(0)
	except Exception as e:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {str(e)}\n")
		sys.exit(1)
