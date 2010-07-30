#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *
import sys

DD = [
	helmholtz_data_water
	, helmholtz_data_carbondioxide
## unexplained errors for these fluids:
#	, helmholtz_data_methane
	, helmholtz_data_ammonia
#	, helmholtz_data_nitrogen
## following still need T_triple data added
#	, helmholtz_data_hydrogen
]

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
	
