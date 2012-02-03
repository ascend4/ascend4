import ascpy

class TestReporter(ascpy.IntegratorReporterCxx) :
	def __init__(self, integ):
		self.data = None
		ascpy.IntegratorReporterCxx.__init__(self, integ)
	
	def initOutput(self):
		self.t = []
		self.data = []
		return 1

	def closeOutput(self):
		print "Caught close"
		return 1

	def updateStatus(self):
		i = self.getIntegrator();
		print i.getCurrentObservations()
		return 1

	def recordObservedValues(self):
		i = self.getIntegrator()
		self.t.append(i.getCurrentTime())
		self.data.append(i.getCurrentObservations())
		return 1
		

