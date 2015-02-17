#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *
import sys

testfluids = ['water','toluene','carbon_dioxide','xenon','ammonia','methane','hydrogen']

print "\nfprops_sat_T_cubic test...\n"

toterrors = 0
totchecks = 0
for f in testfluids:
	D = fluid(f, 'rpp','pengrob')
	print D.name, D.T_t, D.T_c
	TT = linspace(D.T_t, D.T_c, 1000);
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
	print "%s: %d errors from %d checks" % (D.name, errcount, len(TT))
	totchecks += len(TT)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))




print "\nfprops_sat_p test...\n"

toterrors = 0
totchecks = 0
testfluids = ['water','toluene','carbondioxide','xenon','ammonia','methane','hydrogen']
for f in testfluids:
	D = fluid(f, 'iapws','helmholtz')
	pt, rhof, rhog = D.triple_point()
	pc = D.p(D.T_c, D.rho_c)
	print D.name, pt, pc
	pp = linspace(pt, pc, 1000);
	firsterror = True
	errcount = 0
	for p in pp:
		sys.stderr.write("%s: %f\r" % (D.name,p))
		try:
			T, rhof, rhog = D.sat_p(p)
		except:
			if firsterror:
				print "%s: Error %d at p = %0.12e" % (D.name,res,p)
				firsterror = False
			errcount += 1				
	print "%s: %d errors from %d checks" % (D.name, errcount, len(pp))
	totchecks += len(pp)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))


