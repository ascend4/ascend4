#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *
import sys

DD = [
	helmholtz_data_water
	, helmholtz_data_carbondioxide
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
		sys.stderr.write("%f\r" % T)
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
	print D.name, D.p_t, D.p_c
	pp = linspace(D.p_t, D.p_c, 100);
	firsterror = True
	errcount = 0
	for p in pp:
		sys.stderr.write("%f\r" % p)
		res, T, rhof, rhog = fprops_sat_p(p,D)
		if res:
			if firsterror:
				print "%s: Error %d at p = %f" % (D.name,res,p)
				firsterror = False
			errcount += 1				
	print "%s: %d errors from %d checks" % (D.name, errcount, len(pp))
	totchecks += len(pp)
	toterrors += errcount

print "Total: %d errors across all tested substances (%0.1f%% correct)" % (toterrors,100.*(1 - float(toterrors)/totchecks))

