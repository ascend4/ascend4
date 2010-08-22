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

#--- for (T,s) plots ---

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
	D = fprops.fprops_fluid(str(S1.cd.component.getSymbolValue()))	
	out = []
	hh = linspace(float(S1.h), float(S2.h), n)
	for h in hh:
		res, T, rho = fprops.fprops_solve_ph(float(S1.p), h, 0, D)
		if not res:
			out += [TSPoint(T,fprops.helmholtz_s(T,rho,D))]
	return out


def plot_Ts(SS,style='b-'):
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append(float(S.s)/1.e3)
	plot(xx,yy,style)

#--- for (T,H) plots ---

class THPoint:
	def __init__(self,T,H):
		self.T = T
		self.H = H

def pconsth(S1,S2,n):
	"""Return a set of (T,H) points between two states, with pressure constant"""
	D = fprops.fprops_fluid(str(S1.cd.component.getSymbolValue()))	
	out = []
	hh = linspace(float(S1.h), float(S2.h), n)
	for h in hh:
		res, T, rho = fprops.fprops_solve_ph(float(S1.p), h, 0, D)
		if not res:
			out += [THPoint(T,h * float(S1.mdot))]
	return out

def plot_TH(SS,style='b-',Href = 0):
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append((float(S.H) - Href)/1.e6)
	plot(xx,yy,style)

#--- various Rankine cycle configurations ---

def cycle_plot_rankine(self):
	"""Plot T-s diagram for a simple Rankine cycle"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fprops_fluid(str(self.cd.component.getSymbolValue()))
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


def cycle_plot_rankine_regen2(self):
	"""Plot T-s diagram for a regenerative Rankine cycle (bleed steam regen)"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fprops_fluid(str(self.cd.component.getSymbolValue()))
	sat_curve(D)

	boiler_curve = pconst(self.BO.inlet, self.BO.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,100)

	SS = [self.PU1.inlet, self.PU1.outlet] + \
			pconst(self.HE.inlet, self.HE.outlet, 100) + \
			[self.PU2.inlet, self.PU2.outlet] + \
			boiler_curve + \
			[self.TU1.inlet, self.TU1.outlet, self.TU2.outlet] + \
			condenser_curve + [self.PU1.inlet]

	plot_Ts(SS)
	plot_Ts(
		[self.PU1.inlet, self.PU1.outlet, self.HE.inlet, self.HE.outlet, 
			self.PU2.inlet, self.PU2.outlet, self.TU1.inlet, self.TU1.outlet, 
			self.TU2.outlet, self.PU1.inlet]
		,'bo'
	)

	# line for the heat exchanger
	plot_Ts(pconst(self.HE.inlet_heat, self.HE.outlet,100),'b-')

	title(unicode(r"Regenerative Rankine cycle with %s" % D.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/regen2.eps"))



def cycle_plot_rankine_regen1(self):
	"""Plot T-s diagram for a regenerative Rankine cycle"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fprops_fluid(str(self.cd.component.getSymbolValue()))
	sat_curve(D)

	boiler_curve = pconst(self.BO.inlet, self.BO.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,100)
	he_hot = pconst(self.HE.inlet_heat, self.HE.outlet_heat,100)
	he_cold = pconst(self.HE.inlet, self.HE.outlet,100)

	SS = [self.PU.outlet] + he_cold + [self.BO.inlet] + boiler_curve + [self.TU.inlet, self.TU.outlet] + he_hot + condenser_curve + [self.PU.inlet, self.PU.outlet]

	plot_Ts(SS)
	plot_Ts(
		[self.PU.outlet,self.BO.inlet,self.TU.inlet, self.TU.outlet
		 	,self.HE.outlet_heat, self.PU.inlet, self.PU.outlet]
		,'bo'
	)

	# dotted lines for the heat exchanger
	plot_Ts([self.HE.inlet_heat, self.HE.outlet],'b:')
	plot_Ts([self.HE.outlet_heat, self.HE.inlet],'b:')

	title(unicode(r"Regenerative Rankine cycle with %s" % D.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/regen1.eps"))


#--- heat exchange (T,H) plot ---

def heater_closed_plot(self):
	"""Plot T-H diagram of heat transfer in a heater_closed model"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fprops_fluid(str(self.cd.component.getSymbolValue()))
	HE = self.HE

	extpy.getbrowser().reporter.reportNote("Fluid is %s" % D.name)	

	plot_TH(pconsth(HE.inlet_heat, HE.outlet_heat, 50),'r-',
		Href = (float(HE.outlet_heat.h)*float(HE.outlet_heat.mdot))\
	)

	plot_TH(pconsth(HE.inlet, HE.outlet, 50),'b-',
		Href = (float(HE.inlet.h)*float(HE.inlet.mdot))\
	)

	title(unicode(r"Closed feedwater heater with %s" % D.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("H / [MW]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/heater_closed.eps"))

#--- the big one: a combined-cycle GT ---

def cycle_plot(self):
	"""Plot T-s diagram for combined-cycle gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()

	D = fprops.fprops_fluid(str(self.cd_rankine.component.getSymbolValue()))

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


extpy.registermethod(cycle_plot_rankine)
extpy.registermethod(cycle_plot_rankine_regen1)
extpy.registermethod(cycle_plot_rankine_regen2)
extpy.registermethod(heater_closed_plot)

extpy.registermethod(cycle_plot)
#the above method can be called using "EXTERNAL fourbarplot(SELF)" in ASCEND.
