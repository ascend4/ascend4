#!/usr/bin/env python

# This script runs LCOV to generate coverage testing results from the main CUnit test suites
# implemented for ASCEND currently.

LCOV="/usr/local/bin/lcov"
GENHTML='/usr/local/bin/genhtml'
SCONS_CALL=["scons","-j4","MALLOC_DEBUG=1","GCOV=1","CC=gcc","CXX=g++","test","ascend","models","solvers"]

import os
PREFIX=os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),".."))
#print "PREFIX=",PREFIX
ORIG=os.getcwd()
os.chdir(PREFIX)

DEVNULL = open(os.devnull,'w')

tests = {
	'general':[
		'general_color','general_dstring','general_hashpjw'
		,'general_list','general_listio','general_mem','general_pool'
		,'general_pairlist'
		,'general_pretty','general_stack','general_table','general_tm_time'
		,'general_ospath','general_env','general_ltmatrix','general_ascMalloc'
	],'utilities':[
		'utilities_ascEnvVar','utilities_ascPrint'
		,'utilities_ascSignal','utilities_readln','utilities_set'
		,'utilities_error'
	],'linear':['linear_qrrank','linear_mtx']
	,'compiler':[
		'compiler_basics','compiler_expr','compiler_fixfree','compiler_fixassign'
	],'packages':['packages_defaultall']
	,'solver':[
		'solver_slv_common','solver_slvreq','solver_qrslv'
		,'solver_fprops','solver_lrslv' #'solver_ipopt',
	],'integrator':['integrator_lsode']
}
	
import subprocess
LCOV_STEM=[LCOV,'-d',PREFIX,'--no-external','--exclude','\<stdout\>']
LCOV_CD=LCOV_STEM + ['-c']
# clean
print "CLEANING UP"
subprocess.check_call(SCONS_CALL + ['-c'],stdout=DEVNULL)

#build
print "BUILD"
subprocess.check_call(SCONS_CALL,stdout=DEVNULL,stderr=DEVNULL)

if 0:
	#baseline
	print "BASELINE"
	F_BASELINE = 'mycov-0.info'
	mycall = LCOV_CD + ['-i','-o',F_BASELINE]
	#print "CALL:",mycall 
	subprocess.check_call(mycall,stdout=DEVNULL)
else:
	print "ZERO"
	mycall = LCOV_STEM + ['-z']
	subprocess.check_call(mycall,stdout=DEVNULL)	
	
myenv = os.environ.copy()
myenv['ASCENDLIBRARY']='models:solvers/qrslv:solvers/ipopt:solvers/lrslv'
myenv['LD_LIBRARY_PATH']='.'

for t in tests:
	print "TEST '%s'" % (t,)
	print " ".join(tests[t])
	subprocess.check_call(['test/test']+tests[t],env=myenv,stdout=DEVNULL,stderr=DEVNULL)
	F = 'mycov-%s.info' % (t,)
	subprocess.check_call(LCOV_CD + ['-o',F,'-t',t],stdout=DEVNULL)
	F1 = 'mycov-%s-1.info' % (t,)
	subprocess.check_call([LCOV,'-r',F,'*stdout*','-o',F1],stdout=DEVNULL)

subprocess.check_call([GENHTML
		,'-p',PREFIX
		,'-o','lcov-html'
		,'-t','ASCEND - CUnit test coverage'
	] + ['mycov-%s-1.info'%(t,) for t in tests]
)

os.chdir(ORIG)

