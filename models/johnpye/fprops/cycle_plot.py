# -*- coding: utf8 -*-
import extpy, sys
from solverreporter import *

import sys, os, os.path
sys.path.append(os.path.expanduser("~/ascend/models/johnpye/fprops/python"))
import fprops

try:
	from pylab import *
except:
	pass

def sat_curve(d):
	Tt = d.T_t
	Tc = d.T_c
	TT = []
	pp = []
	ssf = []
	ssg = []
	for T in linspace(Tt,Tc,100):
		res,p,rf,rg = fprops.fprops_sat_T(T,d)
		if not res:
			TT.append(T - 273.15)
			pp.append(p)
			ssf.append(fprops.helmholtz_s_raw(T,rf,d)/1.e3)
			ssg.append(fprops.helmholtz_s_raw(T,rg,d)/1.e3)
	plot(ssf,TT,"b--")
	plot(ssg,TT,"r--")

class TSPoint:
	def __init__(self,T,s):
		self.T = T
		self.s = s

def write(msg):
	extpy.getbrowser().reporter.reportNote(msg)

def pconst(S1,S2,n):
	"""Return a set of (T,s) points between two states, with pressure held constant."""
	D = eval("fprops.helmholtz_data_%s" % S1.cd.component.getSymbolValue())	
	out = []
	hh = linspace(float(S1.h), float(S2.h), n)
	for h in hh:
		res, T, rho = fprops.fprops_solve_ph(float(S1.p), h, 0, D)
		if not res:
			out += [TSPoint(T,fprops.helmholtz_s(T,rho,D))]
	return out

def plot_Ts(SS):
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append(float(S.s)/1.e3)
	plot(xx,yy)

def cycle_plot(self):
	"""Plot T-s diagram for combined-cycle gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()

	d = eval("fprops.helmholtz_data_%s" % self.cd_rankine.component.getSymbolValue())

	# plot gas turbine cycle
	SS = [self.GC.inlet, self.GC.outlet, self.GT.inlet, self.GT.outlet, self.HE.inlet, self.HE.outlet, self.GC.inlet]
	plot_Ts(SS)
	hold(1)
	
	sat_curve(d)

	boiler_curve = pconst(self.HE.inlet_cold,self.HE.outlet_cold,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,100)
	SS2 = [self.PU.outlet, self.HE.inlet_cold] + boiler_curve + [self.HE.outlet_cold, self.TU.inlet, self.TU.outlet, self.CO.inlet] + condenser_curve + [self.CO.outlet, self.PU.inlet, self.PU.outlet]
	plot_Ts(SS2)

	title(unicode(r"With %s bottoming cycle" % d.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

def cycle_plot_rankine(self):
	"""Plot T-s diagram for a simple Rankine cycle"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = eval("fprops.helmholtz_data_%s" % self.cd.component.getSymbolValue())
	sat_curve(D)

	boiler_curve = pconst(self.BO.inlet, self.BO.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,100)
	SS = [self.PU.outlet, self.BO.inlet] + boiler_curve + [self.TU.inlet, self.TU.outlet] + condenser_curve + [self.CO.outlet, self.PU.outlet]
	plot_Ts(SS)
	title(unicode(r"Rankine cycle with %s" % D.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()


extpy.registermethod(cycle_plot_rankine)

extpy.registermethod(cycle_plot)
#the above method can be called using "EXTERNAL fourbarplot(SELF)" in ASCEND.
