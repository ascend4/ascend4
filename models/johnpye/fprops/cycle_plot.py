# -*- coding: utf8 -*-
import extpy
from solverreporter import *

import sys, os, os.path
sys.path.append(os.path.expanduser("~/ascend/models/johnpye/fprops/python"))
import fprops

try:
	from pylab import *
except:
	pass

def sat_curve(fluid):
	d = eval("fprops.helmholtz_data_%s" % fluid.getSymbolValue())
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
			ssf.append(fprops.helmholtz_s(T,rf,d)/1.e3)
			ssg.append(fprops.helmholtz_s(T,rg,d)/1.e3)
	plot(ssf,TT,"b--")
	plot(ssg,TT,"r--")

def cycle_plot(self):
	"""Plot the geometry of the four-bar linkage"""
	# following is an unfortunate necessity in the current system architecture:
	import loading
	loading.load_matplotlib(throw=True)

	ioff()
	figure()

	# plot gas turbine cycle
	SS = [self.GC.inlet, self.GC.outlet, self.GT.inlet, self.GT.outlet, self.GC.inlet]
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append(float(S.s)/1.e3)
	plot(xx,yy);
	hold(1)
	
	sat_curve(self.cd_rankine.component)

	SS2 = [self.PU.outlet, self.HE.inlet_cold, self.HE.outlet_cold, self.TU.inlet, self.TU.outlet, self.CO.inlet, self.CO.outlet, self.PU.inlet, self.PU.outlet]
	xx = []
	yy = []
	for S in SS2:
		yy.append(float(S.T) - 273.15)
		xx.append(float(S.s)/1.e3)

	plot(xx,yy);
	ylabel(unicode(r"T / [°C]"))
	xlabel("s / [kJ/kg/K]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(cycle_plot)
#the above method can be called using "EXTERNAL fourbarplot(SELF)" in ASCEND.
