#!/usr/bin/env python
# Test of convergence of the saturation routines across a range of substances

from fprops import *
from pylab import *

DD = [
	helmholtz_data_water
	, helmholtz_data_carbondioxide
## following still need T_triple data added
#	, helmholtz_data_ammonia
#	, helmholtz_data_hydrogen
#	, helmholtz_data_nitrogen
#	, helmholtz_data_methane
]

for D in DD:
	TT = linspace(D.T_t, D.T_c, 300);
	for T in TT:
		res, p, rhof, rhog = fprops_sat_T(T,D)
		if res:
			print "%s: Error %d at T = %f" % (D.name,res,T)

	
