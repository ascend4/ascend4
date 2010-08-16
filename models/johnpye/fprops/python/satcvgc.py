#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *
import sys

DD = [
	helmholtz_data_water
	, helmholtz_data_toluene
	, helmholtz_data_carbondioxide
	, helmholtz_data_xenon
	, helmholtz_data_ammonia
	, helmholtz_data_methane
	, helmholtz_data_nitrogen
	, helmholtz_data_hydrogen
## unexplained errors for these fluids:
## following still need T_triple data added
]

print "\nfprops_sat_T test...\n"

toterrors = 0
totchecks = 0
for D in DD:
	print D.name, D.T_t, D.T_c
	TT = linspace(D.T_t, D.T_c, 1000);
	firsterror = True
	errcount = 0
	for T in TT:
		sys.stderr.write("%s: %f\r" % (D.name,T))
		res, p, rhof, rhog = fprops_sat_T(T,D)
		if res:
			if firsterror:
				print "%s: Error %d at T = %f" % (D.name,res,T)
				firsterror = False
			errcount += 1				
	print "%s: %d errors from %d checks" % (D.name, errcount, len(TT))
	totchecks += len(TT)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))




print "\nfprops_sat_p test...\n"

toterrors = 0
totchecks = 0
for D in DD:
	res, pt, rhof, rhog = fprops_triple_point(D)
	pc = helmholtz_p(D.T_c, D.rho_c, D)
	print D.name, pt, pc
	pp = linspace(pt, pc, 1000);
	firsterror = True
	errcount = 0
	for p in pp:
		sys.stderr.write("%s: %f\r" % (D.name,p))
		res, T, rhof, rhog = fprops_sat_p(p,D)
		if res:
			if firsterror:
				print "%s: Error %d at p = %0.12e" % (D.name,res,p)
				firsterror = False
			errcount += 1				
	print "%s: %d errors from %d checks" % (D.name, errcount, len(pp))
	totchecks += len(pp)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))

