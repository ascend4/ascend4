import ascend

class PythonSolverReporter(ascend.SolverReporter):
	def __init__(self):
		print "SOLVER REPORTER ---- PYTHON"
		ascend.SolverReporter.__init__(self)
	def report(self,status):
		print "Reporting (python)",status.getIterationNum()
