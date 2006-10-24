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
	""" use the NOTES DB to configure solver parameters for the current model """
	self = ascpy.Registry().getInstance('context')
	sim = browser.sim
	reporter = browser.reporter

	# at present this code is pretty clunky because of the bare-bone code in the wrapper API.
	# this could be improved a lot with some python wizardry for iterators, __getitem__ etc.

	db = browser.library.getAnnotationDatabase()

	solvernotes = db.getNotes(self.getType(),ascpy.SymChar("solver"),ascpy.SymChar("name"))
	if len(solvernotes) > 1:
		reporter.reportNote("Multiple solvers specified in NOTES for model '%s'", sim.getType())
	elif len(solvernotes) == 1:
		solver = ascpy.Solver(solvernotes[0].getText())
		reporter.reportNote("Setting solver to '%s'" % solver.getName())
		sim.setSolver(solver)
	else:
		reporter.reportNote("No solver specified in NOTES , using current")

	solvername = sim.getSolver().getName()
	#reporter.reportNote("Parameters for solver '%s'" % solvername)

	notes = db.getNotes(self.getType(),ascpy.SymChar(solvername))

	params = sim.getSolverParameters()
	paramnames = [p.getName() for p in params]

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
