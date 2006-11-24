import unittest
import ascpy
import math

class AscendTest(unittest.TestCase):

	def setUp(self):
		import ascpy
		self.L = ascpy.Library()
	
	def tearDown(self):
		self.L.clear()
		del self.L

	def testlog10(self):
		self.L.load('johnpye/testlog10.a4c')
		T = self.L.findType('testlog10')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())		
		M.run(T.getMethod('self_test'))		

	def testLSODE(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		print M.sim.getChildren()
		assert float(M.sim.x) == 10.0
		assert float(M.sim.v) == 0.0
		t_end = 3 * math.pi

		I = ascpy.Integrator(M)
		I.setReporter(ascpy.IntegratorReporterNull(I))
		I.setEngine('LSODE');
		I.setLinearTimesteps(ascpy.Units("s"), 0.0, t_end, 100);
		I.setMinSubStep(0.005);
		I.setMaxSubStep(0.5);
		I.setInitialSubStep(0.01);
		I.setMaxSubSteps(100);
		I.analyse();
		I.solve();
		assert abs(float(M.sim.x) + 10) < 1e-2
		assert abs(float(M.sim.v)) < 1e-2
		assert I.getNumObservedVars() == 3

class NotToBeTested:
	def nothing(self):
		pass

	def testloading(self):
		pass

	def testsystema4l(self):
		self.L.load('simpleflowsheet01.a4c')

	def testatomsa4l(self):
		self.L.load('atoms.a4l')
		
if __name__=='__main__':
	unittest.main()
