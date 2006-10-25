import ascpy
import extpy
browser = extpy.getbrowser()

def listnotes(self):
	""" make a list of NOTES for the present model """
	self = ascpy.Registry().getInstance('context')

	db = browser.library.getAnnotationDatabase()
	notes = db.getNotes(self.getType(),ascpy.SymChar("solver"))

	for i in range(1,len(notes)):
		mm = notes[i].getMethod()
		ll = notes[i].getLanguage()
		ii = notes[i].getId()
		tt = notes[i].getText()
		s = "type = %s, method = %s, lang = %s, id = %s, text = %s" % (notes[i].getType(), mm, ll, ii, tt)
		print "NOTES:",s
		browser.reporter.reportNote(s)

def setup_solver(self):
	""" use the NOTES DB to set parameters applicable to the current active solver """
	print "SETUP_SOLVER..."
	self = ascpy.Registry().getInstance('context')
	sim = ascpy.Registry().getSimulation('sim')

	reporter = browser.reporter

	db = browser.library.getAnnotationDatabase()

	print "GOT SIM..."
	
	if not sim:
		reporter.reportError("No simulation present yet")
		return

	if not browser.solver:
		reporter.reportError("No solver yet")
		return

	solvername = browser.solver.getName()
	reporter.reportNote("Active solver is '%s'" % solvername)

	notes = db.getNotes(self.getType(),ascpy.SymChar(solvername))

	print "GETTINGS SOLVER PARAMS..."

	params = sim.getSolverParameters()

	print "DONE SOLVER PARAMS"

	print params

	paramnames = [p.getName() for p in params]

	print "DONE PARAMS"

	for i in range(0,len(notes)):
		note = notes[i]

		if note.getId()==None:
			#browser.reporter.reportNote("Empty note ID...")
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
			elif param.isString():
				v = note.getText()
				param.setStrValue(v)
			elif param.isBool():
				v = bool( note.getText() )
				param.setBoolValue(v)
			else:
				raise Exception("unknown parameter type")
			reporter.reportNote("Set %s = %s" % (param.getName(),v))
		else:
			reporter.reportWarning("Parameter '%s' is not valid for solver '%s'" % (n,solvername))

extpy.registermethod(listnotes)
extpy.registermethod(setup_solver)
