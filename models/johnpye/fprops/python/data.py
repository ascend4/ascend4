# -*- coding: utf8 -*-
"""
Display main data about fluids in the database: T_c, p_c, rho_c, T_t, p_t.
"""
from fprops import *

print "%30s\t%10s %10s %10s %10s %10s" % ('Name','Tc/[°C]', 'pc/bar', 'rhoc', 'Tt/[°C]', 'pt/[bar]')
for i in range(fprops_num_fluids()):
	D = fprops_get_fluid(i)
	Tc = D.T_c
	pc = fprops_pc(D)
	rhoc = D.rho_c
	Tt = D.T_t
	res, pt, rhoft, rhogt = fprops_triple_point(D)
	print "%30s\t%10.2f %10.2f %10.2f %10.2f %10.2f" % (D.name, Tc-273.15, pc/1e5, rhoc, Tt-273.15, pt/1e5)


