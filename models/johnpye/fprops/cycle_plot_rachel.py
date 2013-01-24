# -*- coding: utf8 -*-
import extpy, sys
from solverreporter import *
from __builtin__ import *

import sys, os, os.path
sys.path.append(os.path.expanduser("~/ascend/models/johnpye/fprops/python"))
import fprops

try:
	from pylab import *
except:
	pass

#--- for (T,s) plots ---

def sat_curve(D):
	Tt = D.T_t
	Tc = D.T_c
	TT = []
	pp = []
	ssf = []
	ssg = []
	for T in linspace(Tt,Tc,100):
		# TODO this is inefficient because of repeated saturation solutions.
		SF = D.set_Tx(T,0)
		SG = D.set_Tx(T,1)
		TT.append(SF.T - 273.15)
		pp.append(SF.p)
		ssf.append(SF.s/1.e3)
		ssg.append(SG.s/1.e3)
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
	D = fprops.fluid(str(S1.cd.component.getSymbolValue()),str(S1.cd.type.getSymbolValue()))	
	out = []
	hh = linspace(float(S1.h), float(S2.h), n)
	for h in hh:
		S = D.set_ph(float(S1.p), h)
		out += [TSPoint(S.T,S.s)]
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
	def __init__(self,T,h,mdot = 1.):
		self.T = T
		self.h = h
		self.mdot = mdot

def pconsth(S1,S2,n):
	"""Return a set of (T,H) points between two states, with pressure constant"""
	D = fprops.fluid(str(S1.cd.component.getSymbolValue()),str(S1.cd.type.getSymbolValue()))	
	out = []
	hh = linspace(float(S1.h), float(S2.h), n)
	mdot = float(S1.mdot)
	for h in hh:
		# TODO add try/except
		S = D.set_ph(float(S1.p),h)
		out += [THPoint(S.T,h * mdot)]
	return out

def plot_TH(SS,style='b-',Href = 0):
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append(((float(S.h)*float(S.mdot)) - Href)/1.e6)
	plot(xx,yy,style)

#--- various Rankine cycle configurations ---

def cycle_plot_brayton(self):
	"""Plot T-s diagram for a simple Brayton cycle"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
	sat_curve(D)
	hold(1)

	boiler_curve = pconst(self.BO.inlet, self.BO.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,100)
	SS = [self.PU.outlet, self.BO.inlet] + boiler_curve + [self.TU.inlet, self.TU.outlet] + condenser_curve + [self.CO.outlet, self.PU.outlet]
	plot_Ts(SS)

	title(unicode(r"Brayton cycle with %s" % D.name))
	ylabel(unicode(r"T / [°C]"))
	aa = axis(); axis([aa[0],aa[1],-100,600])
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/brayton.eps"))
	
def cycle_plot_rankine_regen2(self):
	"""Plot T-s diagram for a regenerative Rankine cycle (bleed steam regen)"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
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
	aa = axis(); axis([aa[0],aa[1],-100,600])
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
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
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
	aa = axis(); axis([aa[0],aa[1],-100,600])
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
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
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

#--- simple gas turbine models ---


def cycle_plot_brayton_regen(self):
	"""Plot T-s diagran for regenerative gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
	sat_curve(D)	

	# plot gas turbine cycle
	SS = [self.PU.inlet, self.PU.outlet, self.RE.inlet, self.RE.outlet, self.BO.inlet,self.BO.outlet, self.TU.inlet, self.TU.outlet,self.RE.inlet_hot, self.RE.outlet_hot, self.CO.inlet, self.CO.outlet,self.PU.inlet]
	plot_Ts(SS,'g-')
	plot_Ts(SS,'go')
	hold(1)

	title(unicode(r"Regenerative Brayton cycle"))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/brayton_regen.eps"))


def cycle_plot_brayton_split(self):
	"""Plot T-s diagran for split-regeneration gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()		
	hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
	sat_curve(D)
	
	# add some dots for the points in the cycle
#	seq = "CO2.inlet HEL.inlet HEL.outlet HEH.inlet BO.inlet TU.inlet HEH.inlet_hot HEL.inlet_hot CO1.inlet CO1.outlet".split(" ")
	seq = "CO2.inlet HEL.inlet HEH.inlet BO.inlet TU.inlet HEH.inlet_hot HEH.outlet_hot CO1.inlet".split(" ")
	lalign = "CO1.inlet HEH.outlet_hot ".split(" ")
	SS1 = []; SS1a = []
	for s in seq:
		print "looking at '%s'"%s
		p = reduce(getattr,s.split("."),self)
		SS1.append(p)
		SS1a.append((p,s))
	plot_Ts(SS1,'go')

	print "ANNOTATIONS"
	for s in SS1a:
		align = "right"
		if s[1] in lalign:
			align = "left"
		annotate(s[1]+"  ", xy =(float(s[0].s)/1.e3,float(s[0].T) - 273.15)
			,horizontalalignment=align
		)

	SS2 = pconst(self.DI.inlet, self.DI.outlet, 50) + [self.CO2.inlet,self.CO2.outlet] + pconst(self.HEL.inlet,self.HEH.outlet,50) + pconst(self.BO.inlet,self.BO.outlet,50) + [self.TU.inlet, self.TU.outlet] + pconst(self.HEH.inlet_hot,self.HEL.outlet_hot,50) + [self.CO1.inlet,self.CO1.outlet]
	plot_Ts(SS2,'g-')

	SS3 = [self.HEL.inlet, self.HEL.outlet_hot]
	plot_Ts(SS3,'g--')
	SS4 = [self.HEL.outlet, self.HEL.inlet_hot]
	plot_Ts(SS4,'g--')

	SS5 = [self.HEH.inlet, self.HEH.outlet_hot]
	plot_Ts(SS5,'g--')
	SS6 = [self.HEH.outlet, self.HEH.inlet_hot]
	plot_Ts(SS6,'g--')


	title(unicode(r"Split Regenerative Brayton cycle"))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	savefig(os.path.expanduser("~/Desktop/brayton__split_regen.eps"))


def cycle_plot_brayton_reheat(self):
	"""Plot T-s diagram for reheat gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()		
	hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
	sat_curve(D)
	
	# plot gas turbine cycle
	boiler1_curve = pconst(self.BO1.inlet, self.BO1.outlet,100)
	boiler2_curve = pconst(self.BO2.inlet, self.BO2.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,1000)
	SS = [self.PU.inlet, self.PU.outlet] + boiler1_curve + [self.TU1.inlet, self.TU1.outlet] + boiler2_curve + [self.TU2.inlet, self.TU2.outlet, self.CO.inlet] + condenser_curve + [self.CO.outlet,self.PU.inlet]
	plot_Ts(SS)
	hold(1)

	title("Reheat Brayton cycle")
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()


def cycle_plot_brayton_reheat_regen(self):
	"""Plot T-s diagram for reheat-regenerative gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()		
	"""hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()))
	sat_curve(D)"""
	
	# plot gas turbine cycle
	boiler1_curve = pconst(self.BO1.inlet, self.BO1.outlet,100)
	boiler2_curve = pconst(self.BO2.inlet, self.BO2.outlet,100)
	condenser_curve = pconst(self.CO.inlet,self.CO.outlet,1000)
	SS = [self.PU.inlet, self.PU.outlet] + boiler1_curve + [self.TU1.inlet, self.TU1.outlet] + boiler2_curve + [self.TU2.inlet, self.TU2.outlet, self.CO.inlet, self.CO.outlet, self.PU.inlet]
	plot_Ts(SS)
	hold(1)

	title(unicode(r"Reheat Regenerative Brayton cycle"))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	#savefig(os.path.expanduser("~/Desktop/brayton__split_regen.eps"))	



def cycle_plot_brayton_reheat_regen_intercool(self):
	"""Plot T-s diagram for reheat-regenerative gas turbine"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()		
	hold(1)
	D = fprops.fluid(str(self.cd.component.getSymbolValue()),str(self.cd.type.getSymbolValue()))
	sat_curve(D)
	
	# add some dots for the points in the cycle
	seq = "CO1.inlet DI2.inlet CO2.inlet RE.inlet BU1.inlet TU1.inlet BU2.inlet TU2.inlet RE.inlet_hot DI1.inlet".split(" ")
	lalign = "TU2.inlet RE.inlet_hot BU2.inlet DI1.inlet DI2.inlet CO1.inlet".split(" ")
	SS1 = []; SS1a = []
	for s in seq:
		print "looking at '%s'"%s
		p = reduce(getattr,s.split("."),self)
		SS1.append(p)
		SS1a.append((p,s))
	plot_Ts(SS1,'bo')

	print "ANNOTATIONS"
	for s in SS1a:
		align = "right"
		if s[1] in lalign:
			align = "left"
		annotate(s[1]+"  ", xy =(float(s[0].s)/1.e3,float(s[0].T) - 273.15)
			,horizontalalignment=align
		)

	# plot the cycle with smooth curves
	BU1_curve = pconst(self.BU1.inlet, self.BU1.outlet,30)
	BU2_curve = pconst(self.BU2.inlet, self.BU2.outlet,20)
	DI1_curve = pconst(self.DI1.inlet,self.DI1.outlet,20)
	DI2_curve = pconst(self.DI2.inlet,self.DI2.outlet,20)
	REH_curve = pconst(self.RE.inlet_hot,self.RE.outlet_hot,50)
	REL_curve = pconst(self.RE.inlet,self.RE.outlet,50)

	SS2 = [self.CO1.inlet, self.CO1.outlet] + DI2_curve + [self.CO2.inlet, self.CO2.outlet] + REL_curve + BU1_curve + [self.TU1.inlet, self.TU1.outlet] + BU2_curve + [self.TU2.inlet, self.TU2.outlet] + REH_curve + DI1_curve + [self.CO1.inlet]
	plot_Ts(SS2)

	title(unicode(r"Reheat Regenerative Brayton cycle with Intercooling"))
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

		
def air_stream_heat_exchanger_plot(self):
	"""Plot T-H diagram of heat transfer in a heater_closed model"""
	import loading
	loading.load_matplotlib(throw=True)
	ioff()
	figure()
	hold(1)
	D = fprops.fluid(str(self.cd_cold.component.getSymbolValue()),str(self.cd_cold.type.getSymbolValue()))

	n = self.n.getIntValue()
	extpy.getbrowser().reporter.reportNote("Fluid is %s" % D.name)	

	# hot side is the air, calculated in the model
	plot_TH( [self.H[i] for i in range(1+int(n))],'r-',\
		Href = (float(self.outlet.h)*float(self.outlet.mdot))\
	)

	plot_TH(pconsth(self.inlet_cold, self.outlet_cold, 50),'b-',
		Href = (float(self.inlet_cold.h)*float(self.inlet_cold.mdot))\
	)

	title(unicode(r"Combined-cycle air-%s heat exchanger" % D.name))
	ylabel(unicode(r"T / [°C]"))
	xlabel("H / [MW]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()
	#savefig(os.path.expanduser("~/Desktop/air_stream_heatex.eps"))

extpy.registermethod(cycle_plot_brayton)
extpy.registermethod(cycle_plot_rankine_regen1)
extpy.registermethod(cycle_plot_rankine_regen2)
extpy.registermethod(cycle_plot_brayton_regen)
extpy.registermethod(cycle_plot_ccgt)
extpy.registermethod(cycle_plot_brayton_split)
extpy.registermethod(heater_closed_plot)
extpy.registermethod(air_stream_heat_exchanger_plot)
extpy.registermethod(cycle_plot_brayton_reheat_regen)
extpy.registermethod(cycle_plot_brayton_reheat)
extpy.registermethod(cycle_plot_brayton_reheat_regen_intercool)

#the above method can be called using "EXTERNAL fourbarplot(SELF)" in ASCEND.
