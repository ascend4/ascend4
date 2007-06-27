import ascpy
import extpy

def solvernotes(self):
	""" use the NOTES DB to set parameters applicable to the current active solver """
	print "SETUP_SOLVER..."
	sim = ascpy.Registry().getSimulation('sim')

	browser = extpy.getbrowser()
	if browser:
		reporter = browser.reporter
	else:
		print "Using console solver reporter"
		reporter = ascpy.getReporter()

	if browser:
		print "Using browser.solver"
		solver = browser.solver
	else:
		print "Using hardwired default solver, QRSlv"
		solver = ascpy.Solver("QRSlv")
	

	db = ascpy.Library().getAnnotationDatabase()

	print "GOT SIM..."
	
	if not sim:
		reporter.reportError("No simulation present yet")
		return

	if not solver:
		reporter.reportError("No solver yet")
		return

	# the simulation needs to be built at this point, else solver could not
	# be selected, and hence solver params could not be assigned.
	sim.build()
	
	# use the solver selected in the browser
	sim.setSolver(solver)

	solvername = solver.getName()
	reporter.reportNote("Active solver is '%s'" % solvername)

	notes = db.getNotes(self.getType(),ascpy.SymChar(solvername))

	print "GETTING SOLVER PARAMS..."

	params = sim.getParameters()

	print "DONE SOLVER PARAMS"

	print params

	paramnames = [p.getName() for p in params]

	print "DONE PARAMS"

	for i in range(0,len(notes)):
		note = notes[i]

		if note.getId()==None:
			browser.reporter.reportNote("Empty note ID...")
			continue
		n = note.getId()
		param = None
		for p in params:
			if p.getName()==n: 
				param = p
		if param:
			if param.isInt():
				v = int( note.getText() )
				param.setIntValue(v)
			elif param.isReal():
				v = float( note.getText() )
				param.setRealValue(v)
			elif param.isStr():
				v = note.getText()
				param.setStrValue(v)
			elif param.isBool():
				v = bool( note.getText() )
				param.setBoolValue(v)
			else:
				raise Exception("unknown parameter type")
			reporter.reportNote("Set %s = %s" % (param.getName(),v))
		else:
			reporter.reportWarning("Ignoring unrecognised parameter '%s' for solver '%s' (from solver notes)\n%s:%d" % (n,solvername,note.getFilename(),note.getLineNumber()))

extpy.registermethod(solvernotes)
