import unittest

class AscendTest(unittest.TestCase):

	def testIDA(self):
		import ascpy
		try:
			L = ascpy.Library()
			L.load('johnpye/shm.a4c')
			M = L.findType('shm').getSimulation('sim')
			M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		except Exception,e:
			self.fail(str(e))

	def testloading(self):
		import ascpy

	def testsystema4l(self):
		import ascpy
		L = ascpy.Library()
		L.load('simpleflowsheet01.a4c')

	def testatomsa4l(self):
		import ascpy
		L = ascpy.Library()
		#L.clear();
		L.load('atoms.a4l')

	def testlog10(self):
		import ascpy
		L = ascpy.Library()
		L.load('johnpye/testlog10.a4c')
		T = L.findType('testlog10')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())		
		M.run(T.getMethod('self_test'))		

class NotToBeTested:
	def nothing(self):
		pass

		
if __name__=='__main__':
	unittest.main()
