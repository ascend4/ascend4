#!/usr/bin/env python
#	ASCEND modelling environment
#	Copyright (C) 2006 Carnegie Mellon University
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2, or (at your option)
#	any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place - Suite 330,
#	Boston, MA 02111-1307, USA.

# This script gives a test suite for the high-level interface of ASCEND via
# Python. It is also planned to be a wrapper for the CUnit test suite, although
# this is still experimental.

import unittest
import os, sys
import math
import atexit

import platform
if platform.system() != "Windows":
	import dl
	sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

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

	def testinstanceas(self):
		M = self._run('example_1_6_1',"QRSlv","johnpye/sunpos.a4c")
		self.assertAlmostEqual( float(M.t_solar), M.t_solar.as("s"))
		self.assertAlmostEqual( float(M.t_solar)/3600, M.t_solar.as("h"))

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
		try:
			M = self.L.findType('fail1').getSimulation('sim')
			self.fail("expected exception was not raised")
		except RuntimeError,e:
			print "Caught exception '%s', assumed ok" % e

	def testfail2(self):
		"""Incorrect data arg check -- tests bbox, not ascend"""
		self.L.load('test/blackbox/fail2.a4c')
		try:
			M = self.L.findType('fail2').getSimulation('sim')
			self.fail("expected exception was not raised")
		except RuntimeError,e:
			print "Caught exception '%s', assumed ok (should mention errors during instantiation)" % e

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

# this test doesn't work: 'system is inconsistent' -- and structurally singular
#	def testpass13(self):
#		"""cross-merged input/output solve"""
#		M = self._run('pass13',filename='test/blackbox/passmerge.a4c')

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

	def testextrelfor(self):
		M = self._run('extrelfor',filename='johnpye/extfn/extrelfor.a4c')

## @TODO fix bug with badly-named bbox rel in a loop (Ben, maybe)
#	def testextrelforbadnaming(self):
#		self.L.load('johnpye/extfn/extrelforbadnaming.a4c')
#		T = self.L.findType('extrelfor')
#		M = T.getSimulation('sim')
#		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
#		print "x[1] = %f" % M.x[1]
#		print "x[2] = %f" % M.x[2]
#		print "x[3] = %f" % M.x[3]
#		print "x[4] = %f" % M.x[4]
#		print "x[5] = %f" % M.x[5]
#		M.run(T.getMethod('self_test'))

	def testextrelrepeat(self):
		M = self._run('extrelrepeat',filename='johnpye/extfn/extrelrepeat.a4c')

#-------------------------------------------------------------------------------
# Testing of a ExtPy - external python methods

class TestExtPy(AscendSelfTester):
	def test1(self):
		self.L.load('johnpye/extpy/extpytest.a4c')
		T = self.L.findType('extpytest')
		M = T.getSimulation('sim')
		M.run(T.getMethod('self_test'))
		
	def test2(self):
		self.L.load('johnpye/extpy/extpytest.a4c')
		T = self.L.findType('extpytest')
		M = T.getSimulation('sim')
		M.run(T.getMethod('pythonthing'))
		M.run(T.getMethod('pythonthing'))
		M.run(T.getMethod('pythonthing'))
		M.run(T.getMethod('pythonthing'))
		# causes crash!

#-------------------------------------------------------------------------------
# Testing of saturated steam properties library (iapwssatprops.a4c)

class TestSteam(AscendSelfTester):
	def testiapwssatprops1(self):
		M = self._run('testiapwssatprops1',filename='steam/iapwssatprops.a4c')
	def testiapwssatprops2(self):
		M = self._run('testiapwssatprops2',filename='steam/iapwssatprops.a4c')
	def testiapwssatprops3(self):
		M = self._run('testiapwssatprops3',filename='steam/iapwssatprops.a4c')
	def testsatsteamstream(self):
		M = self._run('satsteamstream',filename='steam/satsteamstream.a4c')

	def testsatsteamstream(self):
		M = self._run('satsteamstream',filename='steam/satsteamstream.a4c')

## @TODO fix error capture from bounds checking during initialisation
#	def testiapwssat1(self):
#		M = self._run('testiapwssat1',filename='steam/iapwssat.a4c')

# @TODO fix bug with unpivoted node[i].hg_expr eqns.
	def testdsgsat(self):
		self.L.load('steam/dsgsat2.a4c')
		T = self.L.findType('dsgsat2')
		M = T.getSimulation('sim',False)
		try:
			M.run(T.getMethod('on_load'))
		except:
			pass
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		M.run(T.getMethod('fixed_states'))
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 5, 100)
		I.setMinSubStep(0.01)
		I.setMaxSubStep(0.02)
		I.setInitialSubStep(0.1)
		I.analyse()
		I.solve()
		I.solve()
		#M.checkStructuralSingularity() causes crash!
		
#-------------------------------------------------------------------------------
# Testing of freesteam external steam properties functions

with_freesteam = True
try:
	# we assume that if the freesteam python module is installed, the ASCEND
	# external library will also be.
	import freesteam
	have_freesteam = True
except ImportError,e:
	have_freesteam = False

if with_freesteam and have_freesteam:
	class TestFreesteam(AscendSelfTester):
		def testfreesteamtest(self):
			"""run the self-test cases bundled with freesteam"""
			self._run('testfreesteam',filename='testfreesteam.a4c')

		def testload(self):
			"""check that we can load 'thermalequilibrium2' (IMPORT "freesteam", etc)"""
			self.L.load('johnpye/thermalequilibrium2.a4c')

		def testinstantiate(self):
			"""load an instantiate 'thermalequilibrium2'"""
			self.testload()
			M = self.L.findType('thermalequilibrium2').getSimulation('sim')
			return M

		def testintegrate(self):
			"""integrate transfer of heat from one mass of water/steam to another
			according to Newton's law of cooling"""
			M = self.testinstantiate()
			M.setSolver(ascpy.Solver("QRSlv"))
			I = ascpy.Integrator(M)
			I.setEngine('LSODE')
			I.setReporter(ascpy.IntegratorReporterConsole(I))
			I.setLinearTimesteps(ascpy.Units("s"), 0, 3000, 30)
			I.setMinSubStep(0.01)
			I.setInitialSubStep(1)
			I.analyse()
			print "Number of vars = %d" % I.getNumVars()
			assert I.getNumVars()==2
			I.solve()
			assert I.getNumObservedVars() == 3;
			print "S[1].T = %f K" % M.S[1].T
			print "S[2].T = %f K" % M.S[2].T
			print "Q = %f W" % M.Q		
			self.assertAlmostEqual(float(M.S[1].T),506.77225109,5);
			self.assertAlmostEqual(float(M.S[2].T),511.605173967,5);
			self.assertAlmostEqual(float(M.Q),-48.32922877329,3);
			self.assertAlmostEqual(float(M.t),3000);
			print "Note that the above values have not been verified analytically"

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

	def testlotka(self):
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

## @TODO fails during IDACalcIC (model too big?)
#	def testkryx(self):
#		self.L.load('johnpye/idakryx.a4c')
#		ascpy.getCompiler().setUseRelationSharing(False)
#		M = self.L.findType('idakryx').getSimulation('sim')
#		M.setSolver(ascpy.Solver('QRSlv'))
#		M.build()
#		I = ascpy.Integrator(M)
#		I.setEngine('IDA')
#		I.setReporter(ascpy.IntegratorReporterConsole(I))
#		I.setParameter('linsolver','DENSE')
#		I.setParameter('maxl',8)
#		I.setParameter('gsmodified',False)
#		I.setParameter('autodiff',True)
#		I.setParameter('rtol',0)
#		I.setParameter('atol',1e-3);
#		I.setParameter('atolvect',False)
#		I.setParameter('calcic','YA_YDP')
#		I.analyse()
#		I.setLogTimesteps(ascpy.Units("s"), 0.01, 10.24, 11)
#		I.solve()
#		assert abs(M.u[2][2].getValue()) < 1e-5
	
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
	# a whole bag of tricks to make sure we get the necessary dirs in our ascend, python and ld path vars
	restart = 0

	if platform.system()=="Windows":
		LD_LIBRARY_PATTH="PATH"
		SEP = ";"
	else:
		LD_LIBRARY_PATH="LD_LIBRARY_PATH"
		SEP = ":"

	modelsdir = os.path.normpath(os.path.join(sys.path[0],"models"))
	if not os.environ.get('ASCENDLIBRARY'):
		os.environ['ASCENDLIBRARY'] = modelsdir
		restart = 1
	else:
		envmodelsdir = [os.path.abspath(i) for i in os.environ['ASCENDLIBRARY'].split(SEP)]
		if modelsdir not in envmodelsdir:
			envmodelsdir.insert(0,modelsdir)
			os.environ['ASCENDLIBRARY']=SEP.join(envmodelsdir)
			restart = 1		

	libdirs = ["pygtk","."]
	libdirs = [os.path.normpath(os.path.join(sys.path[0],l)) for l in libdirs]
	if not os.environ.get(LD_LIBRARY_PATH):
		os.environ[LD_LIBRARY_PATH]=SEP.join(libdirs)
		restart = 1
	else:
		envlibdirs = [os.path.normpath(i) for i in os.environ[LD_LIBRARY_PATH].split(SEP)]
		for l in libdirs:
			if l in envlibdirs[len(libdirs):]:
				envlibdirs.remove(l)
				restart = 1
		for l in libdirs:
			if l not in envlibdirs:
				envlibdirs.insert(0,l)
				restart = 1		
		os.environ[LD_LIBRARY_PATH] = SEP.join(envlibdirs)

	pypath = os.path.normpath(os.path.join(sys.path[0],"pygtk"))
	if not os.environ.get('PYTHONPATH'):
		os.environ['PYTHONPATH']=pypath
	else:
		envpypath = os.environ['PYTHONPATH'].split(SEP)
		if pypath not in envpypath:
			envpypath.insert(0,pypath)
			os.environ['PYTHONPATH']=envpypath
			restart = 1

	if restart:
		script = os.path.join(sys.path[0],"test.py")
		print "Restarting"
		print "LD_LIBRARY_PATH = %s" % os.environ.get(LD_LIBRARY_PATH)
		print "PYTHONPATH = %s" % os.environ.get('PYTHONPATH')
		print "ASCENDLIBRARY = %s" % os.environ.get('ASCENDLIBRARY')
		os.execvp("python",[script] + sys.argv)

	import ascpy

	try:
		import cunit
	except:
		pass

	atexit.register(ascpy.shutdown)
	#suite = unittest.TestSuite()
	#suite = unittest.defaultTestLoader.loadTestsFromName('__main__')
	#unittest.TextTestRunner(verbosity=2).run(suite)
	unittest.main()	
