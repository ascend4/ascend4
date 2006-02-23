import ascend
import time

class PythonSolverReporter(ascend.SolverReporter):
	def __init__(self):
		print "SOLVER REPORTER ---- PYTHON"
		self.solvedvars = 0;
		self.updateinterval = 0.1;
		ascend.SolverReporter.__init__(self)
		_time = time.clock();
		self.startime = _time;
		self.lasttime = _time;
		print "Start time = ",self.lasttime

	def report(self,status):
		_solvedvars = status.getNumConverged();
		_time = time.clock();
		_sincelast = _time - self.lasttime
		if _sincelast > self.updateinterval:
			if _solvedvars > self.solvedvars:
				print "Solved %d vars" % _solvedvars
				self.solvedvars = _solvedvars

			print "Iteration ",status.getIterationNum()
			self.lasttime = _time;
		return 0
