# Extpy script for displaying the transfer matrix eigenvalues for a DAE
# system from within ASCEND.
#
# This script must be run via the PyGTK GUI or it will throw an
# exception at extpy.getbrowser().
#
# This script in turn calls roots_subproc.py, in order to work around a
# bug in scipy (used for the computation of matrix eigenvalues)

import extpy;
from solverreporter import *
import os
import os.path
import subprocess
import sys

derivs = ['dg/dz','dg/dx','df/dz','df/dx',"df/dx'"]

def createfiles():
	fff = {}
	for d in derivs:
		fff[d] = os.tempnam()
	return fff

def deletefiles(fff):
	for f in fff.values():
		os.unlink(f)

def roots(self):
	"""Plot the complex eigenvalues of a DAE system"""

	# the following is an unfortunate necessity in the current system architecture:
	browser = extpy.getbrowser()
	M = browser.sim
	M.setSolver(ascpy.Solver('QRSlv'))

	# get IDA to analyse the DAE structure
	I = ascpy.Integrator(M)
	I.setEngine('IDA')
	I.setReporter(ascpy.IntegratorReporterConsole(I))
	I.analyse()

	# write the results of analysis to some tempfiles

	fff = createfiles()

	for k,v in fff.iteritems():
		F = file(v,'w')
		I.writeMatrix(F,k)
	
	print "WROTE MATRICES TO FILE. NOW PROCESSING..."

	# we can't import scipy here due to a crash. so we must use a subprocess...

	script = os.path.expanduser('~/ascend/models/johnpye/roots_subproc.py')
	if os.path.exists(script):
		P = subprocess.Popen(['python',script]+[fff[d] for d in derivs],stdout=subprocess.PIPE,close_fds=True)
		ret = P.wait()
		if ret:
			print "GOT ERROR CODE FROM roots_subproc.py"
			browser.reporter.reportError(P.stdout.read())
			deletefiles(fff)
			return 1

		print "OK"
	else:
		browser.reporter.reportError("Couldn't find script '%s'" % script)
		deletefiles(fff)
		return 1

	deletefiles(fff)
	return 0

extpy.registermethod(roots)
#the above method can be called using "EXTERNAL roots(SELF)" in ASCEND.
