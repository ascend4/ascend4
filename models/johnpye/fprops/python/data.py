# -*- coding: utf8 -*-
"""
Display main data about fluids in the database: T_c, p_c, rho_c, T_t, p_t.
"""
from fprops import *

print "    %30s\t%10s %10s %10s %10s %10s %10s" % ('Name','Tc/[°C]', 'pc/bar', 'rhoc', 'Tt/[°C]', 'pt/[bar]','omega')
for i in range(num_fluids()):
	D = get_fluid(i)
	Tc = D.T_c
	pc = D.p_c
	rhoc = D.rho_c
	Tt = D.T_t
	omega = D.omega
	pt, rhoft, rhogt = 0,0,0
	try:
		pt, rhoft, rhogt = D.triple_point()
	except Exception,e:
		print "ERROR: %s" % str(e)
	print "%3d:%30s\t%10.2f %10.2f %10.2f %10.2f %10.2f %10.2f" % (i,D.name, Tc-273.15, pc/1e5, rhoc, Tt-273.15, pt/1e5, omega)
