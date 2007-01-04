#!/usr/bin/env python
import unittest

import platform, sys
if platform.system() != "Windows":
	import dl
	sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

import ascpy
import math
import os, subprocess
import atexit
import cunit

class Ascend(unittest.TestCase):

	def setUp(self):
		import ascpy
		self.L = ascpy.Library()
	
	def tearDown(self):
		self.L.clear()
		del self.L

class AscendSelfTester(Ascend):

	def _run(self,modelname,solvername="QRSlv",filename=None):
		if filename==None:
			filename = 'johnpye/%s.a4c' % modelname
		self.L.load(filename)
		T = self.L.findType(modelname)
		M = T.getSimulation('sim')
		M.build()
		M.solve(ascpy.Solver(solvername),ascpy.SolverReporter())	
		M.run(T.getMethod('self_test'))
		return M

class TestCompiler(Ascend):

	def testloading(self):
		pass

	def testsystema4l(self):
		self.L.load('system.a4l')

	def testatomsa4l(self):
		self.L.load('atoms.a4l')

class TestSolver(AscendSelfTester):
	
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
		print M.getChildren()
		assert float(M.x) == 10.0
		assert float(M.v) == 0.0
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
		print "x = %f" % M.x
		print "v = %f" % M.v
		assert abs(float(M.x) + 10) < 1e-2
		assert abs(float(M.v)) < 1e-2
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

	def testparameters(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		P = I.getParameters()
		for p in P:
			print p.getName(),"=",p.getValue()
		assert len(P)==11
		assert P[0].isStr()
		assert P[0].getName()=="linsolver"
		assert P[0].getValue()=='SPGMR'
		assert P[2].getName()=="autodiff"
		assert P[2].getValue()==True
		assert P[7].getName()=="atolvect"
		assert P[7].getBoolValue() == True
		P[2].setBoolValue(False)
		assert P[2].getBoolValue()==False
		I.setParameters(P)
		assert I.getParameterValue('autodiff')==False
		I.setParameter('autodiff',True)
		try:
			v = I.getParameterValue('nonexist')
		except KeyError:
			pass
		else:
			self.fail('Failed to trip invalid Integrator parameter')

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
		I.setLinearTimesteps(ascpy.Units(), 1.0, 1.5, 5)
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
		I.setLinearTimesteps(ascpy.Units("s"), 0, 2*float(M.v)/float(M.g), 2)
		I.analyse()
		I.solve()
		print "At end of simulation,"
		print "x = %f" % M.x
		print "v = %f" % M.v
		M.run(T.getMethod('self_test'))

	def testlotka(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 5)
		I.analyse()
		print "Number of vars = %d" % I.getNumVars()
		assert I.getNumVars()==2
		I.solve()
		assert I.getNumObservedVars() == 3;
		assert abs(M.R - 832) < 1.0
		assert abs(M.F - 21.36) < 0.1

#-------------------------------------------------------------------------------
# Testing of a external blackbox functions

class TestBlackBox(AscendSelfTester):
	def testparsefail0(self):
		try:
			self.L.load('test/blackbox/parsefail0.a4c')
			self.fail("parsefail0 should not have loaded without errors")
		except:
			pass

	def testparsefail1(self):
		try:
			self.L.load('test/blackbox/parsefail1.a4c')
			self.fail("parsefail1 should not have loaded without errors")
		except:
			pass

	def testparsefail2(self):
		try:
			self.L.load('test/blackbox/parsefail2.a4c')
			self.fail("parsefail2 should not have loaded without errors")
		except:
			pass

	def testparsefail3(self):
		try:
			self.L.load('test/blackbox/parsefail3.a4c')
			self.fail("parsefail3 should not have loaded without errors")
		except:
			pass

	def testparsefail4(self):
		try:
			self.L.load('test/blackbox/parsefail4.a4c')
			self.fail("parsefail4 should not have loaded")
		except:
			pass

	def testfail1(self):
		"""Mismatched arg counts check-- tests bbox, not ascend."""
		self.L.load('test/blackbox/fail1.a4c')
		M = self.L.findType('fail1').getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())

	def testfail2(self):
		"""Incorrect data arg check -- tests bbox, not ascend"""
		self.L.load('test/blackbox/fail2.a4c')
		M = self.L.findType('fail2').getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())

	def testpass1(self):
		"""simple single bbox forward solve"""
		M = self._run('pass1',filename='test/blackbox/pass.a4c')

	def testpass2(self):
		"""simple single bbox reverse solve"""
		M = self._run('pass2',filename='test/blackbox/pass.a4c')

	def testpass3(self):
		"""simple double bbox solve"""
		M = self._run('pass3',filename='test/blackbox/pass3.a4c')

	def testpass4(self):
		"""simple double bbox reverse solve"""
		M = self._run('pass4',filename='test/blackbox/pass3.a4c')

	def testpass5(self):
		M = self._run('pass5',filename='test/blackbox/pass5.a4c')

	def testpass6(self):
		M = self._run('pass6',filename='test/blackbox/pass5.a4c')

	def testpass7(self):
		M = self._run('pass7',filename='test/blackbox/passmerge.a4c')

	def testpass8(self):
		M = self._run('pass8',filename='test/blackbox/passmerge.a4c')

	def testpass9(self):
		M = self._run('pass9',filename='test/blackbox/passmerge.a4c')

	def testpass10(self):
		M = self._run('pass10',filename='test/blackbox/passmerge.a4c')

	def testpass11(self):
		M = self._run('pass11',filename='test/blackbox/passmerge.a4c')

	def testpass12(self):
		M = self._run('pass12',filename='test/blackbox/passmerge.a4c')

	def testpass13(self):
		"""cross-merged input/output solve"""
		M = self._run('pass13',filename='test/blackbox/passmerge.a4c')

	def testpass14(self):
		"""cross-merged input/output reverse solve"""
		M = self._run('pass14',filename='test/blackbox/passmerge.a4c')

	def testpass20(self):
		M = self._run('pass20',filename='test/blackbox/passarray.a4c')

	def testparsefail21(self):
		"""dense array of black boxes wrong syntax"""
		try:
			self.L.load('test/blackbox/parsefail21.a4c')
			self.fail("parsefail21 should not have loaded without errors")
		except:
			pass

	def testpass22(self):
		M = self._run('pass22',filename='test/blackbox/passarray.a4c')

	def testpass23(self):
		M = self._run('pass23',filename='test/blackbox/passarray.a4c')

	def testpass61(self):
		M = self._run('pass61',filename='test/blackbox/reinstantiate.a4c')

	def testpass62(self):
		M = self._run('pass62',filename='test/blackbox/reinstantiate.a4c')

	def testpass64(self):
		M = self._run('pass64',filename='test/blackbox/reinstantiate.a4c')

	def testpass65(self):
		M = self._run('pass65',filename='test/blackbox/reinstantiate.a4c')

	def testpass66(self):
		M = self._run('pass66',filename='test/blackbox/reinstantiate.a4c')

	def testpass67(self):
		M = self._run('pass67',filename='test/blackbox/reinstantiate.a4c')

class TestExtFn(AscendSelfTester):
	def testextfntest(self):
		M = self._run('extfntest',filename='johnpye/extfn/extfntest.a4c')
		self.assertAlmostEqual(M.y, 2);
		self.assertAlmostEqual(M.x, 1);
		self.assertAlmostEqual(M.y, M.x + 1);

#   THIS TEST FAILS
#	def testextrelfor(self):
#		self.L.load('johnpye/extfn/extrelfor.a4c')
#		T = self.L.findType('extrelfor')
#		M = T.getSimulation('sim')
#		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
#		print "x[1] = %f" % M.x[1]
#		print "x[2] = %f" % M.x[2]
#		print "x[3] = %f" % M.x[3]
#		M.run(T.getMethod('self_test'))

	def testextrelrepeat(self):
		M = self._run('extrelrepeat',filename='johnpye/extfn/extrelrepeat.a4c')

#-------------------------------------------------------------------------------
# Testing of a ExtPy - external python methods

class TestExtPy(AscendSelfTester):
	def testextpytest(self):
		print "-------------------=--=-=-=-"
		M = self._run('extpytest',filename='johnpye/extpy/extpytest.a4c')

#-------------------------------------------------------------------------------
# Testing of freesteam external steam properties functions

with_freesteam = True
try:
	import freesteam
	have_freesteam = True
except ImportError,e:
	have_freesteam = False

if with_freesteam and have_freesteam:
	class TestFreesteam(Ascend):
		def testload(self):
			self.L.load('johnpye/thermalequilibrium2.a4c')

		def testinstantiate(self):
			self.testload()
			M = self.L.findType('thermalequilibrium2').getSimulation('sim')

		def testsolve(self):
			self.testinstantiate()
			M.setSolver(ascpy.Solver("QRSlv"))
			#I = ascpy.Integrator(M)
			#I.setEngine('LSODE')
			#I.setReporter(ascpy.IntegratorReporterConsole(I))
			#I.setLinearTimesteps(ascpy.Units("s"), 0, 3000, 30)
			#I.setMinSubStep(0.01)
			#I.setInitialSubStep(0.1)
			#I.analyse()
			#print "Number of vars = %d" % I.getNumVars()
			#assert I.getNumVars()==2
			#I.solve()
			#assert I.getNumObservedVars() == 3;
			#assert abs(M.R - 832) < 1.0
			#assert abs(M.F - 21.36) < 0.1


#-------------------------------------------------------------------------------
# Testing of IDA models using DENSE linear solver

class TestIDADENSE(Ascend):
	"""IDA DAE integrator, DENSE linear solver"""

	def testnewton(self):
		sys.stderr.write("STARTING TESTNEWTON\n")
		self.L.load('johnpye/newton.a4c')
		T = self.L.findType('newton')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setParameter('safeeval',True)
		I.setParameter('rtol',1e-8)
		I.setMaxSubStep(0.001)
		I.setMaxSubSteps(10000)
		
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 2*float(M.v)/float(M.g), 2)
		I.analyse()
		I.solve()
		print "At end of simulation,"
		print "x = %f" % M.x
		print "v = %f" % M.v
		M.run(T.getMethod('self_test'))

	def testlotkaDENSE(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 5);
		I.setParameter('linsolver','DENSE')
		I.setParameter('rtol',1e-8);
		I.analyse()
		assert I.getNumVars()==2
		assert abs(M.R - 1000) < 1e-300
		I.solve()
		assert I.getNumObservedVars() == 3
		assert abs(M.R - 832) < 1.0
		assert abs(M.F - 21.36) < 0.1
		
	def testdenx(self):
		print "-----------------------------====="
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('calcic','YA_YPD')
		I.setParameter('linsolver','DENSE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11)
		I.setMaxSubStep(0);
		I.setInitialSubStep(0)
		I.setMaxSubSteps(0);
		I.setParameter('autodiff',True)
		I.analyse()
		I.solve()
		assert abs(float(M.y1) - 5.1091e-08) < 2e-9
		assert abs(float(M.y2) - 2.0437e-13) < 2e-14
		assert abs(float(M.y3) - 1.0) < 1e-5

	def testkryxDENSE(self):
		self.L.load('johnpye/idakryx.a4c')
		M = self.L.findType('idakryx').getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setParameter('linsolver','DENSE')
		I.setParameter('maxl',8)
		I.setParameter('gsmodified',False)
		I.setParameter('autodiff',True)
		I.setParameter('rtol',0)
		I.setParameter('atol',1e-3);
		I.setParameter('atolvect',False)
		I.setParameter('calcic','YA_YDP')
		I.analyse()
		I.setLogTimesteps(ascpy.Units("s"), 0.01, 10.24, 11)
		I.solve()
		assert abs(M.u[2][2].getValue()) < 1e-5
	
#-------------------------------------------------------------------------------
# Testing of IDA models using SPGMR linear solver (Krylov)
	
# these tests are disabled until SPGMR preconditioning has been implemented
class TestIDASPGMR:#(Ascend):
	def testlotka(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 5)
		I.setParameter('rtol',1e-8)
		I.analyse()
		assert I.getNumVars()==2
		assert abs(M.R - 1000) < 1e-300
		I.solve()
		assert I.getNumObservedVars() == 3
		assert abs(M.R - 832) < 1.0
		assert abs(M.F - 21.36) < 0.1


	def testkryx(self):
		self.L.load('johnpye/idakryx.a4c')
		M = self.L.findType('idakryx').getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setParameter('linsolver','SPGMR')
		I.setParameter('prec','JACOBI')
		I.setParameter('maxl',8)
		I.setParameter('gsmodified',False)
		I.setParameter('autodiff',True)
		I.setParameter('gsmodified',True)
		I.setParameter('rtol',0)
		I.setParameter('atol',1e-3);
		I.setParameter('atolvect',False)
		I.setParameter('calcic','Y')
		I.analyse()
		I.setLogTimesteps(ascpy.Units("s"), 0.01, 10.24, 10);
		print M.udot[1][3]
		I.solve()
		assert 0

	def testzill(self):
		self.L.load('johnpye/zill.a4c')
		T = self.L.findType('zill')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('safeeval',False)
		I.setMinSubStep(1e-7)
		I.setMaxSubStep(0.001)
		I.setMaxSubSteps(10000)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units(), 1.0, 1.5, 5)
		I.analyse()
		I.solve()
		M.run(T.getMethod('self_test'))

	def testdenxSPGMR(self):
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11)
		I.setMaxSubStep(0);
		I.setInitialSubStep(0);
		I.setMaxSubSteps(0);
		I.setParameter('autodiff',True)
		I.setParameter('linsolver','SPGMR')
		I.setParameter('gsmodified',False)
		I.setParameter('maxncf',10)
		I.analyse()
		I.solve()
		assert abs(float(M.y1) - 5.1091e-08) < 1e-10
		assert abs(float(M.y2) - 2.0437e-13) < 1e-15
		assert abs(float(M.y3) - 1.0) < 1e-5

# move code above down here if you want to temporarily avoid testing it
class NotToBeTested:
	def nothing(self):
		pass

if __name__=='__main__':
	atexit.register(ascpy.shutdown)
	#suite = unittest.TestSuite()
	#suite = unittest.defaultTestLoader.loadTestsFromName('__main__')
	#unittest.TextTestRunner(verbosity=2).run(suite)
	unittest.main()	
