import unittest

class AscendTest(unittest.TestCase):

	def setUp(self):
		import ascpy
		self.L = ascpy.Library()
	
	def tearDown(self):
		self.L.clear()
		del self.L

	def testIDA(self):
		try:
			self.L.load('johnpye/shm.a4c')
			M = self.L.findType('shm').getSimulation('sim')
			M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		except Exception,e:
			self.fail(str(e))

	def testloading(self):
		pass

	def testsystema4l(self):
		self.L.load('simpleflowsheet01.a4c')

	def testatomsa4l(self):
		self.L.load('atoms.a4l')

	def testlog10(self):
		self.L.load('johnpye/testlog10.a4c')
		T = self.L.findType('testlog10')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())		
		M.run(T.getMethod('self_test'))		

class NotToBeTested:
	def nothing(self):
		pass

		
if __name__=='__main__':
	unittest.main()
