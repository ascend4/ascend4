import unittest
import ascpy
import math
import os, subprocess, sys
import atexit

class Ascend(unittest.TestCase):

	def setUp(self):
		import ascpy
		self.L = ascpy.Library()
	
	def tearDown(self):
		self.L.clear()
		del self.L

class TestCompiler(Ascend):

	def testloading(self):
		pass

	def testsystema4l(self):
		self.L.load('system.a4l')

	def testatomsa4l(self):
		self.L.load('atoms.a4l')

class TestSolver(Ascend):
	
	def _run(self,modelname,solvername="QRSlv",filename=None):
		if filename==None:
			filename = 'johnpye/%s.a4c' % modelname
		self.L.load(filename)
		T = self.L.findType(modelname)
		M = T.getSimulation('sim')
		M.build()
		M.solve(ascpy.Solver(solvername),ascpy.SolverReporter())	
		M.run(T.getMethod('self_test'))

	def testlog10(self):
		self._run('testlog10')

	def testconopt(self):
		self._run('testconopt',"CONOPT")				

	def testcmslv2(self):
		self._run('testcmslv2',"CMSlv")	

	def testsunpos1(self):
		self._run('example_1_6_1',"QRSlv","johnpye/sunpos.a4c")

	def testsunpos2(self):
		self._run('example_1_6_2',"QRSlv","johnpye/sunpos.a4c")

	def testsunpos3(self):
		self._run('example_1_7_1',"QRSlv","johnpye/sunpos.a4c")

	def testsunpos4(self):
		self._run('example_1_7_2',"QRSlv","johnpye/sunpos.a4c")

	def testsunpos5(self):
		self._run('example_1_7_3',"QRSlv","johnpye/sunpos.a4c")

	def testsunpos6(self):
		self._run('example_1_8_1',"QRSlv","johnpye/sunpos.a4c")

class TestIntegrator(Ascend):

	def testListIntegrators(self):
		I = ascpy.Integrator.getEngines()
		s1 = sorted([str(i) for i in I.values()])
		s2 = sorted(['IDA','LSODE','AWW'])
		assert s1==s2

	# this routine is reused by both testIDA and testLSODE
	def _testIntegrator(self,integratorname):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
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
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		try:
			I.setEngine('___NONEXISTENT____')
		except RuntimeError:
			return
		self.fail("setEngine did not raise error!")

	def testLSODE(self):
		self._testIntegrator('LSODE')

	def testIDA(self):
		self._testIntegrator('IDA')

class TestLSODE(Ascend):

	def testzill(self):
		self.L.load('johnpye/zill.a4c')
		T = self.L.findType('zill')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setMinSubStep(1e-7)
		I.setMaxSubStep(0.001)
		I.setMaxSubSteps(10000)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units(), 1.0, 1.5, 5);
		I.analyse()
		I.solve()
		M.run(T.getMethod('self_test'))

	def testnewton(self):
		sys.stderr.write("STARTING TESTNEWTON\n")
		self.L.load('johnpye/newton.a4c')
		T = self.L.findType('newton')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setParameter('rtolvect',False)
		I.setParameter('rtol',1e-7)
		I.setParameter('atolvect',False)
		I.setParameter('atol',1e-7)
		I.setMinSubStep(1e-7)
		I.setMaxSubStep(0.001)
		I.setMaxSubSteps(10000)
		
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 2*float(M.sim.v)/float(M.sim.g), 2);
		I.analyse()
		I.solve()
		print "At end of simulation,"
		print "x = %f" % M.sim.x
		print "v = %f" % M.sim.v
		M.run(T.getMethod('self_test'))

	def testlotka(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 5);
		I.analyse()
		I.solve()
		assert I.getNumObservedVars() == 3;
		assert abs(float(M.sim.R) - 832) < 1.0
		assert abs(float(M.sim.F) - 21.36) < 0.1
		
class TestIDA(Ascend):

	def testparameters(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		P = I.getParameters()
		for p in P:
			print p.getName(),"=",p.getValue()
		assert len(P)==7
		assert P[0].isStr()
		assert P[0].getName()=="linsolver"
		assert P[0].getValue()=='SPGMR'
		assert P[1].getName()=="autodiff"
		assert P[1].getValue()==True
		assert P[5].getName()=="atolvect"
		assert P[5].getBoolValue() == True
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

	def testzill(self):
		self.L.load('johnpye/zill.a4c')
		T = self.L.findType('zill')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('safeeval',True)
		I.setMinSubStep(1e-7)
		I.setMaxSubStep(0.001)
		I.setMaxSubSteps(10000)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units(), 1.0, 1.5, 5);
		I.analyse()
		I.solve()
		M.run(T.getMethod('self_test'))
		
	def testdenx(self):
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('calcic',False)
		I.setParameter('linsolver','DENSE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11);
		I.setMaxSubStep(0);
		I.setInitialSubStep(0);
		I.setMaxSubSteps(0);
		I.setParameter('autodiff',True)
		I.analyse()
		I.solve()
		assert abs(float(M.sim.y1) - 5.1091e-08) < 1e-10;
		assert abs(float(M.sim.y2) - 2.0437e-13) < 1e-15;
		assert abs(float(M.sim.y3) - 1.0) < 1e-5;

	def testdenxSPGMR(self):
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11);
		I.setMaxSubStep(0);
		I.setInitialSubStep(0);
		I.setMaxSubSteps(0);
		I.setParameter('autodiff',True)
		I.setParameter('linsolver','SPGMR')
		I.setParameter('gsmodified',False)
		I.analyse()
		I.solve()
		assert abs(float(M.sim.y1) - 5.1091e-08) < 1e-10;
		assert abs(float(M.sim.y2) - 2.0437e-13) < 1e-15;
		assert abs(float(M.sim.y3) - 1.0) < 1e-5;

	def testkryx(self):
		self.L.load('johnpye/idakryx.a4c')
		M = self.L.findType('idakryx').getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setParameter('linsolver','SPGMR')
		I.setParameter('maxl',8)
		I.setParameter('gsmodified',False)
		I.setParameter('autodiff',True)
		I.setParameter('rtol',0)
		I.setParameter('atol',1e-3);
		I.setParameter('atolvect',False)
		I.analyse()
		I.setLogTimesteps(ascpy.Units("s"), 0.01, 10.24, 10);
		print M.sim.udot[1][3];
		I.solve()
		assert 0
	
class CUnit(unittest.TestCase):
	def setUp(self):
		self.cunitexe = "../base/generic/test/test"
	
	def testcunittests(self):
		res = os.system(self.cunitexe)
		if res:
			raise RuntimeError("CUnit tests failed (returned %d -- run %s for details)" % (res,self.cunitexe))
		else:
			print "CUnit returned %s" % res

# move code above down here if you want to temporarily avoid testing it
class NotToBeTested:
	def nothing(self):
		pass
		
if __name__=='__main__':
	atexit.register(ascpy.shutdown)
	unittest.main()
