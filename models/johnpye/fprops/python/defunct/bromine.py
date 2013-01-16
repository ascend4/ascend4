#!/usr/bin/env python
import matplotlib
matplotlib.use('gtk')
from pylab import *
from fprops import *
import numpy as np
import sys

print "FPROPS from",sys.modules['fprops'].__file__

TYPE_HELM = 5

FP = fluid('water','pengrob')
FH  = fluid('water','helmholtz')

r = REF_TRHS(FH.T_c,FH.rho_c, 2000e3, 5e3)
FP.set_ref(r)
FH.set_ref(r)

print "T_c =",FP.T_c,FH.T_c
print "omega =",FP.omega,FH.omega
print "rho_c =",FP.rho_c,FH.rho_c
print "p_c =",FP.p_c,FH.p_c

print "p(298,100) ",FP.p(298,100), FH.p(298,100)

rmin = 10 * FP.rhof_T_rackett(273)
rmax = 0.01 * FP.rhog_T_chouaieb(273)

print "rho_min =",rmin
print "rho_max =",rmax

rr = logspace(np.log10(rmin),np.log10(rmax),200)

for F in [FP,FH]:
	figure()
	hold(1)
	cc = ['r-','g-','b-']
	#TT = [0.95 * F.T_c, F.T_c, 1.05 * F.T_c] #
	TT = linspace(273,2 * F.T_c,10)
	for i in range(len(TT)):
		T = TT[i]
		#c = cc[i]
		#T = 298
		pp = [F.p(T,r) for r in rr]
		semilogx(rr,pp)

	p_c = F.p(F.T_c, F.rho_c)
	semilogx([F.rho_c], [p_c],'rx')
	ylabel("pressure / [Pa]")
	xlabel("density / [kg/m3]")
	axis([rmin,rmax,0,p_c*2])
	title('Correlation type %d'%F.type)
	
	

	figure()
	hold(1)
	for i in range(len(TT)):
		T = TT[i]
		#c = cc[i]
		#T = 298
		hh = [F.h(T,r) for r in rr]
		pp = [F.p(T,r) for r in rr]
		semilogy(hh,pp)
	h_c = F.h(F.T_c, F.rho_c)
	p_c = F.p(F.T_c, F.rho_c)
	semilogy(h_c,p_c,'rx')
	ylabel("pressure / [Pa]")
	xlabel("enthalpy / [J/kg]")
	axis([0,5000e3,0,10*p_c])
	title('Correlation type %d'%F.type)

	figure()
	hold(1)
	for i in range(len(TT)):
		T = TT[i]
		ss = [F.s(T,r) for r in rr]
		pp = [F.p(T,r) for r in rr]
		semilogy(ss,pp)
	s_c = F.s(F.T_c, F.rho_c)
	p_c = F.p(F.T_c, F.rho_c)
	semilogy(s_c,p_c,'rx')
	ylabel("pressure / [Pa]")
	xlabel("entropy / [J/kgK]")
	axis([0,10e3,0,10*p_c])
	title('Correlation type %d'%F.type)

	if 1:
		figure()
		hold(1)
		for i in range(len(TT)):
			T = TT[i]
			gg = np.array([F.g(T,r) for r in rr])
			pp = np.ma.masked_less_equal(np.array([F.p(T,r) for r in rr]),0)
			semilogy(gg,pp)
		g_c = F.g(F.T_c, F.rho_c)
		p_c = F.p(F.T_c, F.rho_c)
		semilogy(g_c,p_c,'rx')
		if F.type == TYPE_HELM:
			TTs = linspace(F.T_t,F.T_c,20)
			xxf = []
			xxg = []
			pps = []
			for T in TTs:
				ps,rf,rg = F.sat_T(T)
				pps.append(ps)
				xxf.append(F.g(T,rf))
				xxg.append(F.g(T,rg))
			semilogy(xxg,pps,'r-')
			semilogy(xxf,pps,'go')
		ylabel("pressure / [Pa]")
		xlabel("gibbs energy / [J/kg]")
		axis([0,7e7,0,10*p_c])
		title('Correlation type %d'%F.type)

show()
