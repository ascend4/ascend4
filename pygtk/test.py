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

	def testloading(self):
		pass

	def testsystema4l(self):
		self.L.load('system.a4l')

	def testatomsa4l(self):
		self.L.load('atoms.a4l')

	def testlog10(self):
		self.L.load('johnpye/testlog10.a4c')
		T = self.L.findType('testlog10')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		M.run(T.getMethod('self_test'))		

	def testListIntegrators(self):
		I = ascpy.Integrator.getEngines()
		s1 = sorted([str(i) for i in I.values()])
		s2 = sorted(['IDA','LSODE'])
		assert s1==s2

	# this routine is reused by both testIDA and testLSODE
	def _testIntegrator(self,integratorname):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		print M.sim.getChildren()
		assert float(M.sim.x) == 10.0
		assert float(M.sim.v) == 0.0
		t_end = math.pi

		I = ascpy.Integrator(M)
		I.setReporter(ascpy.IntegratorReporterNull(I))
		I.setEngine(integratorname);
		I.setLinearTimesteps(ascpy.Units("s"), 0.0, t_end, 100);
		I.setMinSubStep(0.0005); # these limits are required by IDA at present (numeric diff)
		I.setMaxSubStep(0.02);
		I.setInitialSubStep(0.001);
		I.setMaxSubSteps(200);
		if(integratorname=='IDA'):
			I.setParameter('autodiff',False)
		I.analyse();
		I.solve();
		print "At end of simulation,"
		print "x = %f" % M.sim.x
		print "v = %f" % M.sim.v
		assert abs(float(M.sim.x) + 10) < 1e-2
		assert abs(float(M.sim.v)) < 1e-2
		assert I.getNumObservedVars() == 3

	def testInvalidIntegrator(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		I = ascpy.Integrator(M)
		try:
			I.setEngine('___NONEXISTENT____')
		except IndexError:
			return
		self.fail("setEngine did not raise error!")

	def testLSODE(self):
		self._testIntegrator('LSODE')

	def testIDA(self):
		self._testIntegrator('IDA')

	def testIDAparameters(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		P = I.getParameters()
		for p in P:
			print p.getName(),"=",p.getValue()
		assert len(P)==5
		assert P[0].isStr()
		assert P[0].getName()=="linsolver"
		assert P[0].getValue()=='DENSE'
		assert P[1].getName()=="autodiff"
		assert P[1].getValue()==True
		assert P[4].getName()=="atolvect"
		assert P[4].getBoolValue() == True
		P[1].setBoolValue(False)
		assert P[1].getBoolValue()==False
		I.setParameters(P)
		for p in I.getParameters():
			print p.getName(),"=",p.getValue()
		assert I.getParameterValue('autodiff')==False
		I.setParameter('autodiff',True)
		try:
			v = I.getParameterValue('nonexist')
		except KeyError:
			pass
		else:
			self.fail('Failed to trip invalid Integrator parameter')

	def testIDAwithDAE(self):
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11);
		I.setMinSubStep(0.0005); # these limits are required by IDA at present (numeric diff)
		I.setMaxSubStep(0.02);
		I.setInitialSubStep(0.001);
		I.setMaxSubSteps(500);
		I.setParameter('autodiff',True)
		I.setParameter('linsolver','DENSE')
		I.analyse()
		I.solve()
		self.fail("WHAT IS THE ANSWER?")

# move code above down here if you want to temporarily avoid testing it
class NotToBeTested:
	def nothing(self):
		pass
		
if __name__=='__main__':
	unittest.main()
