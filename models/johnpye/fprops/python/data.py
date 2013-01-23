# -*- coding: utf8 -*-
"""
Display main data about fluids in the database: T_c, p_c, rho_c, T_t, p_t.
"""
from fprops import *

print "    %30s\t%10s %10s %10s %10s %10s %10s" % ('Name','Tc/[°C]', 'pc/bar', 'rhoc', 'omega', 'Tt/[°C]', 'pt/[Pa]')
for i in range(num_fluids()):
	D = get_fluid(i)
	Tc = D.T_c
	pc = D.p_c
	rhoc = D.rho_c
	Tt = D.T_t
	omega = D.omega
	pt, rhoft, rhogt = 0,0,0
	if D.T_t:
		try:
			pt, rhoft, rhogt = D.triple_point()
		except Exception,e:
			print "ERROR (%s): %s" % (D.name, str(e))
		print "%3d:%30s\t%10.2f %10.2f %10.2f %10.3f %10.2f %10.2f" % (i,D.name, Tc-273.15, pc/1e5, rhoc, omega, Tt-273.15, pt)
	else:
		print "%3d:%30s\t%10.2f %10.2f %10.2f %10.3f %10s %10s" % (i,D.name, Tc-273.15, pc/1e5, rhoc, omega, "--","--")
