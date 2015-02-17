#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *
import sys

testfluids = ['carbon_dioxide']

print "\nfprops_sat_T_cubic test...\n"

toterrors = 0
totchecks = 0
for f in testfluids:
	D = fluid(f, 'pengrob')
	print D.name, D.T_t, D.T_c
	TT = linspace(D.T_t,D.T_c)
	firsterror = True
	errcount = 0
	for T in TT:
		sys.stderr.write("%s: %f\r" % (D.name,T))
		try: 
			p, rhof, rhog = D.sat_T_cubic(T)
		except:
			if firsterror:
				print "%s: Error at T = %f" % (D.name,T)
				firsterror = False
			errcount += 1	
		print T,p/1e3,rhof,rhog			
	print "%s: %d errors from %d checks" % (D.name, errcount, len(TT))
	totchecks += len(TT)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))



