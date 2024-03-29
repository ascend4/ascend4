#!/usr/bin/env python3
#	ASCEND modelling environment
#	Copyright (C) 2006, 2007 Carnegie Mellon University
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
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This script gives a test suite for the high-level interface of ASCEND via
# Python. It is also planned to be a wrapper for the CUnit test suite, although
# this is still experimental.

import unittest
import os, sys
from pathlib import Path
import math
import atexit

import platform
if platform.system() != "Windows":
	try:
		import os
		_dlflags = os.RTLD_GLOBAL|os.RTLD_NOW
	except:
		# On platforms that unilaterally refuse to provide the 'dl' module
		# we'll just set the value and see if it works.
		print("Note: python 'dl' module not available on this system, guessing value of RTLD_* flags")
		_dlflags = 258

	sys.setdlopenflags(_dlflags)

class Ascend(unittest.TestCase):

	def setUp(self):
		import ascpy
		self.L = ascpy.Library()
	
	def tearDown(self):
		self.L.clear()
		del self.L

class AscendSelfTester(Ascend):

	def _run(self,modelname,solvername="QRSlv",filename=None,parameters={}):
		if filename==None:
			filename = 'johnpye/%s.a4c' % modelname
		self.L.load(filename)
		T = self.L.findType(modelname)
		M = T.getSimulation('sim',True)
		M.setSolver(ascpy.Solver(solvername))
		for k,v in list(parameters.items()):
			M.setParameter(k,v)
		M.solve(ascpy.Solver(solvername),ascpy.SolverReporter())	
		M.run(T.getMethod('self_test'))
		return M

class TestCompiler(Ascend):

	def _run(self,filen,modeln=""):
		self.L.load('test/compiler/%s.a4c' % filen)
		T = self.L.findType('%s%s' % (filen,modeln))
		M = T.getSimulation('sim',1)
		M.build()

	def _runfail(self,filen,n,msg="failed"):
		try:
			self._run(filen,'fail%d' % n)
		except Exception as e:
			print("(EXPECTED) ERROR: %s" % e)
			return
		self.fail(msg)


	def testloading(self):
		"""library startup"""
		pass

	def testsystema4l(self):
		"""loading system.a4l?"""
		self.L.load('system.a4l')

	def testatomsa4l(self):
		"""loading atoms.a4l?"""
		self.L.load('atoms.a4l')

	def testmissingreq(self):
		"""flagging a missing REQUIRE"""
		self._runfail('missingreq',1)

	def testmissingsubreq(self):
		"""flagging a subsidiary missing REQUIRE"""
		self._runfail('missingreq',1)

	def defaultmethodstest(self,modelname):
		self.L.load("test/defaultmethods.a4c")
		T = self.L.findType(modelname)
		M = T.getSimulation('sim',True)
		M.run(T.getMethod('on_load'))
		M.run(T.getMethod('self_test'))
		return M

	def testdefault1(self):
		self.defaultmethodstest('testdefault1')

	def testdefault2(self):
		self.defaultmethodstest('testdefault2')

	def testdefault3(self):
		self.defaultmethodstest('testdefault3')

	def testdefault4(self):
		self.defaultmethodstest('testdefault4')

	def testdefault5(self):
		self.defaultmethodstest('testdefault5')

	def testdefault6(self):
		self.defaultmethodstest('testdefault6')

	def testdefault7(self):
		self.defaultmethodstest('testdefault7')

	def testdefault8(self):
		self.defaultmethodstest('testdefault8')

	def testdefault9(self):
		self.defaultmethodstest('testdefault9')

	def testdefault10(self):
		self.defaultmethodstest('testdefault10')

	def testdefault11(self):
		self.defaultmethodstest('testdefault11')

	def testdefault12(self):
		self.defaultmethodstest('testdefault12')

	def testdefault13(self):
		self.defaultmethodstest('testdefault13')

	def testdefault14(self):
		self.defaultmethodstest('testdefault14')

	def testdefault15(self):
		self.defaultmethodstest('testdefault15')

	def testdefault16(self):
		self.defaultmethodstest('testdefault16')

	def testdefault17(self):
		self.defaultmethodstest('testdefault17')
	def testdefault18(self):
		self.defaultmethodstest('testdefault18')

	def testdefault19(self):
		self.defaultmethodstest('testdefault19')

	#def testdefault19fail(self):
	#	self.defaultmethodstest('testdefault19fail')

	def testdefault20(self):
		self.defaultmethodstest('testdefault20')

	#def testdefault20fail(self):
	#	self.defaultmethodstest('testdefault20fail')

class TestSystem(AscendSelfTester):

	def testwritegraph(self):
		M = self._run('testlog10')
		M.run(self.L.findType('testlog10').getMethod('on_load'))
		if platform.system!="Windows":
			M.write('temp.png',"dot")
			assert Path('temp.png').exists()
		else:
			self.fail("not implemented on windows")

class TestSolver(AscendSelfTester):
	
	def testlog10(self):
		M = self._run('testlog10')

	def testrootsofpoly(self):
		self._run('roots_of_poly',filename="roots_of_poly.a4c")

	def testcollapsingcan(self):
		self._run('collapsingcan',filename="collapsingcan.a4c")

	def testdistancecalc(self):
		self._run('distance_calc',filename="distance_calc.a4c")

	def testconopt(self):
		self._run('conopttest',"CONOPT",filename="test/conopt/conopttest.a4c")				

	def testcmslv2(self):
		self._run('testcmslv2',"CMSlv")	

	def testsunpos1(self):
		self._run('example_1_6_1',"QRSlv","johnpye/sunpos_db.a4c")

	def testsunpos2(self):
		self._run('example_1_6_2a',"QRSlv","johnpye/sunpos_db.a4c")
	def testsunpos2(self):
		self._run('example_1_6_2b',"QRSlv","johnpye/sunpos_db.a4c")

	def testsunpos2(self):
		self._run('example_1_6_3',"QRSlv","johnpye/sunpos_db.a4c")

	def testsunpos3(self):
		self._run('example_1_8_1',"QRSlv","johnpye/sunpos_db.a4c")

	def testsunpos4(self):
		self._run('example_1_8_2',"QRSlv","johnpye/sunpos_db.a4c")

	def testsunpos5(self):
		self._run('example_1_8_3',"QRSlv","johnpye/sunpos_db.a4c")

	def testinstanceas(self):
		M = self._run('example_1_6_1',"QRSlv","johnpye/sunpos_db.a4c")
		self.assertAlmostEqual( float(M.t_solar), M.t_solar.to("s"))
		self.assertAlmostEqual( float(M.t_solar)/3600, M.t_solar.to("h"))

	def testrelinclude(self):
		self.L.load('test/relinclude.a4c')
		T = self.L.findType('relinclude')
		M = T.getSimulation('sim',True)
		M.eq1.setIncluded(True)
		M.eq2.setIncluded(False)
		M.eq3.setIncluded(False)
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		self.assertAlmostEqual( float(M.z), 2.0)
		M.eq1.setIncluded(False)
		M.eq2.setIncluded(True)
		M.eq3.setIncluded(False)
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		self.assertAlmostEqual( float(M.z), 4.0)
		M.eq1.setIncluded(False)
		M.eq2.setIncluded(False)
		M.eq3.setIncluded(True)
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		self.assertAlmostEqual( float(M.z), 4.61043629206)


class TestBinTokens(AscendSelfTester):

	def test1(self):
		ascpy.getCompiler().setBinaryCompilation(True)
		self.L.load('johnpye/testlog10.a4c')
		T = self.L.findType('testlog10')
		M = T.getSimulation('sim',True)
		M.build()
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())

class TestLRSlv(AscendSelfTester):
	def testonerel(self):
		self._run('onerel',"LRSlv","test/lrslv/onerel.a4c")

# need to migrate to 'FIX boolvar', currently not supported...
#	def testonerel(self):
#		self._run('onerel',"LRSlv","test/lrslv/onerel.a4c")

	def testsequencecrash(self):
		try:
			self._run('onerel',"LRSlv","test/lrslv/sequencecrash.a4c")
		except:
			pass
			# it just has to not crash, that's all

	def testsequence(self):
		self._run('onerel',"LRSlv","test/lrslv/sequence.a4c")


class TestCMSlv(AscendSelfTester):
	def testsonic(self):
		M = self._run('sonic',"CMSlv","sonic.a4c")
		assert(M.sonic_flow.getBoolValue())

		# other side of boundary...
		M.D.setRealValueWithUnits(4.,"cm")	
		T = self.L.findType('sonic')
		M.solve(ascpy.Solver('CMSlv'),ascpy.SolverReporter())
		M.run(T.getMethod('self_test'))
		assert(not M.sonic_flow.getBoolValue())

	def testheatex(self):
		self._run('heatex',"CMSlv","heatex.a4c")
	def testphaseeq(self):
		self._run('phaseq',"CMSlv","phaseq.a4c")
	def testpipeline(self):
		self._run('pipeline',"CMSlv","pipeline.a4c"
			,{'infinity':3.2e9}
		)
	def testrachford(self):
		self._run('rachford',"CMSlv","rachford.a4c")
	def testlinmassbal(self):
		self._run('linmassbal',"CMSlv","linmassbal.a4c")


class TestMatrix(AscendSelfTester):
	def testlog10(self):
		M = self._run('testlog10')
		print("FETCHING MATRIX.................")
		X = M.getMatrix()
# this stuff crashes Windows because the FILE* structure used by Python is not the same
# as used by MinGW...
		#print "GOT MATRIX"
		#sys.stderr.flush()
		#sys.stdout.flush()
		#F = os.tmpfile()
		#X.write(F.fileno,"mmio")
		#F.seek(0)
		#print F.read()
		
class TestIntegrator(Ascend):

	def testListIntegrators(self):
		I = ascpy.Integrator.getEngines()
		s1 = sorted([str(i) for i in I])
		assert 'IDA' in s1
		assert 'LSODE' in s1

	# this routine is reused by both testIDA and testLSODE
	def _testIntegrator(self,integratorname):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim',True)
		M.setSolver(ascpy.Solver('QRSlv'))
		P = M.getParameters()
		M.setParameter('feastol',1e-12)
		print(M.getChildren())
		assert float(M.x) == 10.0
		assert float(M.v) == 0.0
		t_end = math.pi

		I = ascpy.Integrator(M)
		I.setReporter(ascpy.IntegratorReporterNull(I))
		I.setEngine(integratorname);
		I.setLinearTimesteps(ascpy.Units("s"), 0.0, t_end, 100);
		I.setMinSubStep(0.0001); # these limits are required by IDA at present (numeric diff)
		I.setMaxSubStep(0.1);
		I.setInitialSubStep(0.001);
		I.setMaxSubSteps(200);
		if(integratorname=='IDA'):
			I.setParameter('autodiff',False)
		for p in M.getParameters():
			print(p.getName(),"=",p.getValue())
		I.analyse();
		I.solve();
		print("At end of simulation,")
		print("x = %f" % M.x)
		print("v = %f" % M.v)
		assert abs(float(M.x) + 10) < 1e-2
		assert abs(float(M.v)) < 1e-2
		assert I.getNumObservedVars() == 3

	def testInvalidIntegrator(self):
		self.L.load('johnpye/shm.a4c') 
		M = self.L.findType('shm').getSimulation('sim',True)
		M.setSolver(ascpy.Solver('QRSlv'))
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

	def testparameters(self):
		self.L.load('johnpye/shm.a4c')
		M = self.L.findType('shm').getSimulation('sim',True)
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		P = I.getParameters()
		for p in P:
			print(p.getName(),"=",p.getValue())
		assert len(P)==12
		assert P[0].isStr()
		assert P[0].getName()=="linsolver"
		assert P[0].getValue()=='DENSE'
		assert P[2].getName()=="maxord"
		assert P[3].getName()=="autodiff"
		assert P[3].getValue()==True
		assert P[8].getName()=="atolvect"
		assert P[8].getBoolValue() == True
		P[3].setBoolValue(False)
		assert P[3].getBoolValue()==False
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
		M = T.getSimulation('sim',True)
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
		M = T.getSimulation('sim',True)
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
		print("At end of simulation,")
		print("x = %f" % M.x)
		print("v = %f" % M.v)
		M.run(T.getMethod('self_test'))

	def testlotka(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim',True)
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 5)
		I.analyse()
		print("Number of vars = %d" % I.getNumVars())
		assert I.getNumVars()==2
		I.solve()
		assert I.getNumObservedVars() == 3;
		assert abs(M.R - 832) < 1.0
		assert abs(M.F - 21.36) < 0.1

	def testwritegraph(self):
		self.L.load('johnpye/lotka.a4c')
		M = self.L.findType('lotka').getSimulation('sim',1)
		M.build()
		M.write('lotka.png',"dot")


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
			M = self.L.findType('fail1').getSimulation('sim',True)
			self.fail("expected exception was not raised")
		except RuntimeError as e:
			print("Caught exception '%s', assumed ok" % e)

	def testfail2(self):
		"""Incorrect data arg check -- tests bbox, not ascend"""
		self.L.load('test/blackbox/fail2.a4c')
		try:
			M = self.L.findType('fail2').getSimulation('sim',True)
			self.fail("expected exception was not raised")
		except RuntimeError as e:
			print("Caught exception '%s', assumed ok (should mention errors during instantiation)" % e)

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
# Testing of Sensitivity module

class TestSensitivity(AscendSelfTester):
	def test1(self):
		self.L.load('sensitivity_test.a4c')
		T = self.L.findType('sensitivity_test')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		M.run(T.getMethod('analyse'))
		M.run(T.getMethod('self_test'))

#	def testall(self):
#		self.L.load('sensitivity_test.a4c')
#		T = self.L.findType('sensitivity_test_all')
#		M = T.getSimulation('sim',False)
#		M.run(T.getMethod('on_load'))
#		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
#		M.run(T.getMethod('analyse'))
#		M.run(T.getMethod('self_test'))
# CAUSES CRASH
				
#-------------------------------------------------------------------------------
# Testing of a ExtPy - external python methods

class TestExtPy(AscendSelfTester):
	def test1(self):
		self.L.load('johnpye/extpy/extpytest.a4c')
		T = self.L.findType('extpytest')
		M = T.getSimulation('sim',True)
		M.run(T.getMethod('self_test'))
		
	def test2(self):
		self.L.load('johnpye/extpy/extpytest.a4c')
		T = self.L.findType('extpytest')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('pythonthing'))
		M.run(T.getMethod('pythonthing'))
		M.run(T.getMethod('pythonthing'))
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

	# test the stream model basically works
	def testsatsteamstream(self):
		M = self._run('satsteamstream',filename='steam/satsteamstream.a4c')

	# test that we can solve in terms of various (rho,u)
	def testsatuv(self):
		self.L.load('steam/iapwssat.a4c')
		T = self.L.findType('testiapwssatuv')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		print("p = %f bar" % M.p.to('bar'));
		print("T = %f C" % (M.T.to('K') - 273.15));
		print("x = %f" % M.x);
		M.run(T.getMethod('self_test'))
		M.run(T.getMethod('values2'))
#		M.v.setRealValueWithUnits(1.0/450,"m^3/kg");
#		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		print("p = %f bar" % M.p.to('bar'));
		print("T = %f C" % (M.T.to('K') - 273.15));
		print("x = %f" % M.x);
		M.run(T.getMethod('self_test2'))
		

## @TODO fix error capture from bounds checking during initialisation
#	def testiapwssat1(self):
#		M = self._run('testiapwssat1',filename='steam/iapwssat.a4c')

	def testdsgsat(self):
		self.L.load('steam/dsgsat3.a4c')
		T = self.L.findType('dsgsat3')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		self.assertAlmostEqual(M.dTw_dt[2],0.0);
		M.run(T.getMethod('configure_dynamic'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		return M

	def testdsgsatrepeat(self):
		self.L.load('steam/dsgsat3.a4c')
		T = self.L.findType('dsgsat3')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())

	def testvary(self):
		self.L.load('steam/dsgsat3.a4c')
		T = self.L.findType('dsgsat3')
		M = T.getSimulation('sim',False)
		M.run(T.getMethod('on_load'))
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		print("----- setting qdot_s -----")
		M.qdot_s.setRealValueWithUnits(1000,"W/m")
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		print("----- setting qdot_s -----")
		M.qdot_s.setRealValueWithUnits(2000,"W/m")
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())

	def teststeadylsode(self):
		"test that steady conditions are stable with LSODE"
		M = self.testdsgsat()
		#M.qdot_s.setRealValueWithUnits(1000,"W/m")
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		#M.setParameter('
		I = ascpy.Integrator(M)
		I.setEngine('LSODE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 3600, 10)
		I.analyse()	
		I.solve()

#	def testpeturblsode(self):
#		"test that steady conditions are stable with LSODE"
#		M = self.testdsgsat()
#		# here is the peturbation...
#		M.qdot_s.setRealValueWithUnits(1000,"W/m")
#		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
#	 	I = ascpy.Integrator(M)
#		I.setEngine('LSODE')
#		I.setReporter(ascpy.IntegratorReporterConsole(I))
#		I.setLinearTimesteps(ascpy.Units("s"), 0, 5, 1)
#		I.analyse()	
#		I.solve()

	def teststeadyida(self):	
		M = self.testdsgsat()
		self.assertAlmostEqual(M.dTw_dt[2],0.0)
		Tw1 = float(M.T_w[2])
		T = self.L.findType('dsgsat3')
		M.run(T.getMethod('free_states'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setParameter('safeeval',True)
		I.setParameter('rtol',1e-4)
		I.setParameter('atolvect',False)
		I.setParameter('atol',1e-4)
		I.setParameter('maxord',3)		
		I.setInitialSubStep(0.001)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 3600, 10)
		I.analyse()
		I.solve()
		self.assertAlmostEqual(float(M.T_w[2]),Tw1)
		M.qdot_s.setRealValueWithUnits(1000,"W/m")
		self.assertAlmostEqual(M.qdot_s.to("W/m"),1000)
		M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
		print("dTw/dt = %f" % M.dTw_dt[2])
		self.assertNotAlmostEqual(M.dTw_dt[2],0.0)
		F=file('dsgsat.dot','w')
		M.write(F,'dot')

	def testpeturbida(self):	
		M = self.testdsgsat()
		self.assertAlmostEqual(M.dTw_dt[2],0.0)
		T = self.L.findType('dsgsat3')
		M.run(T.getMethod('free_states'))
		# here is the peturbation...
		qdot_s = float(M.qdot_s)
		print("OLD QDOT_S = %f" % qdot_s)
		M.qdot_s.setRealValueWithUnits(6000,"W/m")
		# IDA has its own initial conditions solver, so no need to call QRSlv here
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		#I.setLinearTimesteps(ascpy.Units("s"), 0,300,300)
		I.setLogTimesteps(ascpy.Units("s"), 0.009, 1200, 150)
		I.analyse()
		F = file('ga.mm','w')
		I.writeMatrix(F,'dg/dz')
		F = file('gd.mm','w')
		I.writeMatrix(F,'dg/dx')
		F = file('fa.mm','w')
		I.writeMatrix(F,'df/dz')
		F = file('fd.mm','w')
		I.writeMatrix(F,'df/dx')
		F = file('fdp.mm','w')
		I.writeMatrix(F,"df/dx'")
		I.solve()
		
#-------------------------------------------------------------------------------
# Testing of freesteam external steam properties functions

with_freesteam = True
try:
	# we assume that if the freesteam python module is installed, the ASCEND
	# external library will also be.
	import freesteam
	have_freesteam = True
except ImportError as e:
	have_freesteam = False

if with_freesteam and have_freesteam:
	class TestFreesteam(AscendSelfTester):
#		def testfreesteamtest(self):
#			"""run the self-test cases bundled with freesteam"""
#			self._run('testfreesteam',filename='testfreesteam.a4c')

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
			print("Number of vars = %d" % I.getNumVars())
			assert I.getNumVars()==2
			I.solve()
			assert I.getNumObservedVars() == 3;
			print("S[1].T = %f K" % M.S[1].T)
			print("S[2].T = %f K" % M.S[2].T)
			print("Q = %f W" % M.Q)		
			self.assertAlmostEqual(float(M.S[1].T),506.77225109,4);
			self.assertAlmostEqual(float(M.S[2].T),511.605173967,5);
			self.assertAlmostEqual(float(M.Q),-48.32922877329,3);
			self.assertAlmostEqual(float(M.t),3000);
			print("Note that the above values have not been verified analytically")

		def testcollapsingcan2(self):
			""" solve the collapsing can model using IAPWS-IF97 steam props """
			M = self._run("collapsingcan2",filename="collapsingcan2.a4c");

#-------------------------------------------------------------------------------
# Testing of the brent-solver EXTERNAL method

class TestBrent(AscendSelfTester):
	def testbrent(self):
		M = self._run('brent1',filename='test/brent.a4c')

#-------------------------------------------------------------------------------
# Testing of IDA's analysis module

class TestIDA(Ascend):
	def _run(self,filen,modeln=""):
		self.L.load('test/ida/%s.a4c' % filen)
		T = self.L.findType('%s%s' % (filen,modeln))
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.analyse()
		return M;

	def _runfail(self,filen,n,msg="failed"):
		try:
			self._run(filen,'fail%d' % n)
		except Exception as e:
			print("(EXPECTED) ERROR: %s" % e)
			return
		self.fail(msg)

	def testsinglederiv(self):
		self._run('singlederiv')

	def testsinglederivfail1(self):
		self._runfail('singlederiv',1
			,"t.ode_id=-1 did not trigger error")

	def testsinglederivfail2(self):
		self._runfail('singlederiv',2
			,"dy_dt.ode_id=2 did not trigger error")

	def testsinglederivfail3(self):
		self._runfail('singlederiv',3
			,"dy_dt.ode_type=3 did not trigger error")

	def testsinglederivfail4(self):
		self._runfail('singlederiv',4
			,"duplicate ode_type=1 did not trigger error")

	def testsinglederivfail5(self):
		self._runfail('singlederiv',5
			,"duplicate ode_type=1 did not trigger error")

	def testsinglederivfail6(self):
		self._runfail('singlederiv',6
			,"duplicate ode_type=1 did not trigger error")

	def testtwoderiv(self):
		self._run('twoderiv')

	def testtwoderivfail1(self):
		self._runfail('twoderiv',1)

	def testtwoderivfail2(self):
		self._runfail('twoderiv',2)

	def testtwoderivfail3(self):
		self._runfail('twoderiv',3)
	def testtwoderivfail4(self):
		self._runfail('twoderiv',4)
	def testtwoderivfail5(self):
		self._runfail('twoderiv',5)

	def testnoderivs(self):
		self._runfail('noderivs',1)

	def testnoindeps(self):
		self._runfail('indeps',1)

	def testtwoindeps(self):
		self._runfail('indeps',2)

	def testfixedvars(self):
		self._run('fixedvars')

	def testfixedvars1(self):
		self._run('fixedvars',1)

# fails the index check
#	def testfixedvars2(self):
#		self._run('fixedvars',2)

# fails the index check
#	def testfixedvars3(self):
#		self._run('fixedvars',3)

	def testincidence(self):
		self._run('incidence')

	def testincidence1(self):
		self._run('incidence',1)
	def testincidence2(self):
		self._run('incidence',2)
	def testincidence3(self):
		M = self._run('incidence',3)

	def testincidence4(self):
		self._run('incidence',4)
	def testincidencefail5(self):
		self._runfail('incidence',5)

	def testwritematrix(self):
		self.L.load('test/ida/writematrix.a4c')
		T = self.L.findType('writematrix')
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.analyse()
# this stuff fails on Windows because FILE structure is different python vs mingw
#		F = os.tmpfile()
#		I.writeMatrix(F,"dF/dy")
#		F.seek(0)
#		print F.read()
#		F1 = os.tmpfile()
#		I.writeMatrix(F1,"dF/dy'")
#		F1.seek(0)
#		print F1.read()
#		F1 = os.tmpfile()
#		I.writeMatrix(F1,"dg/dx")
#		F1.seek(0)
#		print F1.read()
#		# for the moment you'll have to check these results manually.

	def testwritematrix2(self):
		self.L.load('test/ida/writematrix.a4c')
		T = self.L.findType('writematrix2')
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.analyse()
# this stuff fails on Windows because FILE structure is different python vs mingw
#		F = os.tmpfile()
#		I.writeMatrix(F,"dF/dy")
#		F.seek(0)
#		print F.read()
#		F1 = os.tmpfile()
#		I.writeMatrix(F1,"dF/dy'")
#		F1.seek(0)
#		print F1.read()
#		F1 = os.tmpfile()
#		I.writeMatrix(F1,"dg/dx")
#		F1.seek(0)
#		print F1.read()
		#F1 = os.tmpfile()
		#I.writeMatrix(F1,"dydp/dyd")
		#F1.seek(0)
		#print F1.read()
		# for the moment you'll have to check these results manually.

	def testindexproblem(self):
		self.L.load('test/ida/indexproblem.a4c')
		T = self.L.findType('indexproblem')
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.analyse()
		pass

	def testindexproblem2(self):
		self.L.load('test/ida/indexproblem.a4c')
		T = self.L.findType('indexproblem2')
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		try:
			I.analyse()
		except Exception as e:
			return
		self.fail("Index problem not detected")

	def testboundaries(self):
		self.L.load('test/ida/boundaries.a4c')
		T = self.L.findType('boundaries')
		M = T.getSimulation('sim')
		M.build()
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.analyse()
		I.setLogTimesteps(ascpy.Units("s"), 0.1, 20, 20)
		I.setParameter('linsolver','DENSE')
		I.setParameter('calcic','Y')
		I.setParameter('linsolver','DENSE')
		I.setParameter('safeeval',False)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.solve()

# doesn't work yet:
#	def testincidence5(self):
#		self._run('incidence',5)


#-------------------------------------------------------------------------------
# Testing of IDA models using DENSE linear solver

class TestIDADENSE(Ascend):
	"""IDA DAE integrator, DENSE linear solver"""

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
		print("-----------------------------=====")
		self.L.load('johnpye/idadenx.a4c')
		M = self.L.findType('idadenx').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('calcic','YA_YDP')
		I.setParameter('linsolver','DENSE')
		I.setParameter('safeeval',True)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units("s"), 0.4, 4e10, 11)
		I.setMaxSubStep(0);
		I.setInitialSubStep(0)
		I.setMaxSubSteps(0)
		I.setParameter('autodiff',True)
		I.analyse()
		I.solve()
		assert abs(float(M.y1) - 5.1091e-08) < 2e-9
		assert abs(float(M.y2) - 2.0437e-13) < 2e-14
		assert abs(float(M.y3) - 1.0) < 1e-5

	def testhires(self):
		self.L.load('test/hires.a4c')
		T = self.L.findType('hires')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setParameter('rtol',1.1e-15)
		I.setParameter('atolvect',0)
		I.setParameter('atol',1.1e-15)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLogTimesteps(ascpy.Units(""), 1, 321.8122, 5)
		I.setInitialSubStep(1e-5)
		I.setMaxSubSteps(10000)
		I.analyse()
		I.solve()
		for i in range(8):
			print("y[%d] = %.20g" % (i+1, M.y[i+1]))
		M.run(T.getMethod('self_test'))

	def testchemakzo(self):
		self.L.load('test/chemakzo.a4c')
		T = self.L.findType('chemakzo')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setParameter('rtol',1e-15)
		I.setParameter('atolvect',0)
		I.setParameter('atol',1e-15)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 1, 180, 5)
		I.setInitialSubStep(1e-13)
		I.setMaxSubSteps(10000)
		I.analyse()
		I.solve()
		for i in range(6):
			print("y[%d] = %.20g" % (i+1, M.y[i+1]))
		M.run(T.getMethod('self_test'))

	def testtransamp(self):
		self.L.load('test/transamp.a4c')
		T = self.L.findType('transamp')
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver('QRSlv'))
		I = ascpy.Integrator(M)
		I.setEngine('IDA')
		I.setParameter('linsolver','DENSE')
		I.setParameter('rtol',1e-7)
		I.setParameter('atolvect',0)
		I.setParameter('atol',1e-7)
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0.05, 0.2, 20)
		I.setInitialSubStep(0.00001)
		I.setMaxSubSteps(10000)
		I.analyse()
		I.solve()
		for i in range(6):
			print("y[%d] = %.20g" % (i+1, M.y[i+1]))
		M.run(T.getMethod('self_test'))

# MODEL FAILS ANALYSIS: we need to add support for non-incident differential vars
#	def testpollution(self):
#		self.L.load('test/pollution.a4c')
#		T = self.L.findType('pollution')
#		M = T.getSimulation('sim')
#		M.setSolver(ascpy.Solver('QRSlv'))
#		I = ascpy.Integrator(M)
#		I.setEngine('IDA')
#		I.setParameter('linsolver','DENSE')
#		I.setParameter('rtol',1.1e-15)
#		I.setParameter('atolvect',0)
#		I.setParameter('atol',1.1e-15)
#		I.setReporter(ascpy.IntegratorReporterConsole(I))
#		I.setLogTimesteps(ascpy.Units("s"), 1, 60, 5)
#		I.setInitialSubStep(1e-5)
#		I.setMaxSubSteps(10000)
#		I.analyse()
#		I.solve()
#		for i in range(20):
#			print "y[%d] = %.20g" % (i+1, M.y[i+1])
#		M.run(T.getMethod('self_test'))

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
		print(M.udot[1][3])
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

class TestDOPRI5(Ascend):
	def testlotka(self):
		self.L.load('test/dopri5/dopri5test.a4c')
		M = self.L.findType('dopri5test').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())	
		I = ascpy.Integrator(M)
		I.setEngine('DOPRI5')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		I.setLinearTimesteps(ascpy.Units("s"), 0, 200, 20)
		I.setParameter('rtol',1e-8)
		I.analyse()
		assert I.getNumVars()==1
		I.solve()
	def testaren(self):
		self.L.load('test/dopri5/aren.a4c')
		M = self.L.findType('aren').getSimulation('sim')
		M.setSolver(ascpy.Solver("QRSlv"))
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		I = ascpy.Integrator(M)
		I.setEngine('DOPRI5')
		I.setReporter(ascpy.IntegratorReporterConsole(I))
		#xend = 17.0652165601579625588917206249
		I.setLinearTimesteps(ascpy.Units("s"), 0, 17.0652165601579625588917206249, 10)
		I.setParameter('rtol',1e-7)
		I.setParameter('atol',1e-7)
		I.setParameter('tolvect',False)
		I.setMinSubStep(0);
		I.setMaxSubStep(0);
		I.setInitialSubStep(0);
		I.analyse()
		I.solve()
		print("y[0] = %f" % float(M.y[0]))
		assert abs(float(M.y[0]) - 0.994) < 1e-5
		assert abs(float(M.y[1]) - 0.0) < 1e-5

class TestIPOPT(Ascend):

	def ipopt_tester(self,testname,hessian_approx='limited-memory',linear_solver='mumps'):
		self.L.load('test/ipopt/%s.a4c' % testname)
		T = self.L.findType(testname)
		M = T.getSimulation('sim')
		M.setSolver(ascpy.Solver("IPOPT"))
		M.setParameter('linear_solver',linear_solver)
		M.setParameter('hessian_approximation',hessian_approx)
		M.solve(ascpy.Solver("IPOPT"),ascpy.SolverReporter())
		M.run(T.getMethod('self_test'))

	def test2(self):
		self.ipopt_tester('test2')

	def test3(self):
		self.ipopt_tester('test3')

	def test4(self):
		self.ipopt_tester('test4')

	def test5(self):
		self.ipopt_tester('test5')

	def test6(self):
		self.ipopt_tester('test6')

	def test7(self):
		self.ipopt_tester('test7')

	def test7_hsl(self):
		self.ipopt_tester('test7',linear_solver='ma27')

	def test8(self):
		self.ipopt_tester('test8')

	def test9(self):
		self.ipopt_tester('test9')

	def test10(self):
		self.ipopt_tester('test10')

	def test11(self):
		self.ipopt_tester('test11')

	def test12(self):
		self.ipopt_tester('test12')

	def test13(self):
		self.ipopt_tester('test13')

	def test14(self):
		self.ipopt_tester('test14')

	# tests with exact hessian routines...

	def test2_exact(self):
		self.ipopt_tester('test2',hessian_approx='exact')

	def test3_exact(self):
		self.ipopt_tester('test3',hessian_approx='exact')

	def test4_exact(self):
		self.ipopt_tester('test4',hessian_approx='exact')

	def test5_exact(self):
		self.ipopt_tester('test5',hessian_approx='exact')

	def test6_exact(self):
		self.ipopt_tester('test6',hessian_approx='exact')

	def test7_exact(self):
		self.ipopt_tester('test7',hessian_approx='exact')

	def test8_exact(self):
		self.ipopt_tester('test8',hessian_approx='exact')

	def test9_exact(self):
		self.ipopt_tester('test9',hessian_approx='exact')

	def test10_exact(self):
		self.ipopt_tester('test10',hessian_approx='exact')

	def test11_exact(self):
		self.ipopt_tester('test11',hessian_approx='exact')

	def test12_exact(self):
		self.ipopt_tester('test12',hessian_approx='exact')

	def test13_exact(self):
		self.ipopt_tester('test13',hessian_approx='exact')

	def test14_exact(self):
		self.ipopt_tester('test14',hessian_approx='exact')

	def testformula(self):
		self.ipopt_tester('formula')

class TestCSV(Ascend):
	def test1(self):
		self.L.load('johnpye/datareader/testcsv.a4c')
		M = self.L.findType('testcsv').getSimulation('sim',True)
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())


class TestSlvReq(Ascend):
	def test1(self):
		self.L.load('test/slvreq/test1.a4c')
		H = ascpy.SolverHooks(ascpy.SolverReporter())
		ascpy.SolverHooksManager_Instance().setHooks(H)
		T = self.L.findType('test1')
		M = T.getSimulation('sim',False)
		print("\n\n\nRUNNING ON_LOAD EXPLICITLY NOW...")
		M.run(T.getMethod('on_load'))

	def test2(self):
		self.L.load('test/slvreq/test1.a4c')
		R = ascpy.SolverReporter()
		class SolverHooksPython(ascpy.SolverHooks):
			def __init__(self):
				print("PYTHON SOLVER HOOKS")
				ascpy.SolverHooks.__init__(self,None)
			def setSolver(self,solvername,sim):
				sim.setSolver(ascpy.Solver(solvername))
				print("PYTHON: SOLVER is now %s" % sim.getSolver().getName())	
				return 0
			def setOption(self,optionname,val,sim):
				try:
					PP = sim.getParameters()
				except Exception as e:
					print("PYTHON ERROR: ",str(e))
					return ascpy.SLVREQ_OPTIONS_UNAVAILABLE
				try:
					for P in PP:
						if P.getName()==optionname:
							try:
								P.setValueValue(val)
								sim.setParameters(PP)
								print("PYTHON: SET",optionname,"to",repr(val))
								return 0
							except Exception as e:
								print("PYTHON ERROR: ",str(e))
								return ascpy.SLVREQ_WRONG_OPTION_VALUE_TYPE
					return ascpy.SLVREQ_INVALID_OPTION_NAME
				except Exception as e:
					print("PYTHON ERROR: ",str(e))
					return ascpy.SLVREQ_INVALID_OPTION_NAME
			def doSolve(self,inst,sim):
				try:
					print("PYTHON: SOLVING",sim.getName(),"WITH",sim.getSolver().getName())
					sim.solve(sim.getSolver(),R)
				except Exception as e:
					print("PYTHON ERROR:",str(e))
					return 3
				return 0
		H = SolverHooksPython()
		ascpy.SolverHooksManager_Instance().setHooks(H)
		T = self.L.findType('test1')
		M = T.getSimulation('sim',True)

# test some stuff for beam calculations
class TestSection(Ascend):
	def test_compound3(self):
		self.L.load('johnpye/section.a4c')
		T = self.L.findType('compound_section_test3')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		M.run(T.getMethod('self_test'))
	def test_compound4(self):
		self.L.load('johnpye/section.a4c')
		T = self.L.findType('compound_section_test4')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		M.run(T.getMethod('self_test'))
	def test_compound2(self):
		self.L.load('johnpye/section.a4c')
		T = self.L.findType('compound_section_test2')
		M = T.getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
		M.run(T.getMethod('self_test'))


class TestErrorTree(AscendSelfTester):
	"""
	This test is looking at some a tricky bug arising from the use of error_reporter_tree through
	C++ (Simulation::run). Error should be caught when the 'on_load' method is run.
	"""
	def setUp(self):
		super(TestErrorTree,self).setUp();
		self.reporter = ascpy.getReporter()
		self.reporter.setPythonErrorCallback(self.error_callback)

		self.errors = []
	
	def tearDown(self):
		super(TestErrorTree,self).tearDown();
		self.reporter = ascpy.getReporter()
		print "CLEARING CALLBACK"
		self.reporter.clearPythonErrorCallback()

	def error_callback(self,sev,filename,line,msg):
		print "PYTHON ERROR CALLBACK: %s:%d: %s [sev=%d]" % (filename,line,msg,sev)
		self.errors.append((filename,line,msg,sev))
		return 0

	def test1(self):
		self.L.load('test/compiler/badassign.a4c')
		T = self.L.findType('badassign')
		try:
			M = T.getSimulation('sim',True)
		except RuntimeError,e:
			print self.errors

# move code above down here if you want to temporarily avoid testing it
class NotToBeTested:
	def nothing(self):
		pass

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
		print("At end of simulation,")
		print("x = %f" % M.x)
		print("v = %f" % M.v)
		M.run(T.getMethod('self_test'))

def patchpath(VAR,SEP,addvals):
	restart = 0
	envpath = [os.path.abspath(i) for i in os.environ[VAR].split(SEP)]
	for l in addvals:
		if l in envpath[len(addvals):]:
			envpath.remove(l)
			restart = 1
	for l in addvals:
		if l not in envpath:
			envpath.insert(0,l)
			restart = 1
	os.environ[VAR] = SEP.join(envpath)
	return restart	
	
if __name__=='__main__':
	SEP = os.pathsep
	if platform.system()=="Windows":
		LD_LIBRARY_PATH="PATH"
	else:
		LD_LIBRARY_PATH="LD_LIBRARY_PATH"

	for v in ['ASCENDLIBRARY',LD_LIBRARY_PATH,'ASCENDSOLVERS','PYTHONPATH']:
		if not os.environ.get(v):
			raise RuntimeError("Missing expected environment variable '%s'"%(v,))

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

# ex: set ts=4 :

