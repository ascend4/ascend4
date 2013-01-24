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

def plot_TH(SS,style='b-',Href = 0):
	xx = []
	yy = []
	for S in SS:
		yy.append(float(S.T) - 273.15)
		xx.append(((float(S.h)*float(S.mdot)) - Href)/1.e6)
	plot(xx,yy,style)

def regenerator_plot_fprops(self):
	"""Plot T-H diagram of regenerator"""
	import loading; loading.load_matplotlib(throw=True)
	ioff();	figure(); hold(1)
	FH = fprops.fluid(str(self.hxd.component_hot.getSymbolValue()),str(self.hxd.type_hot.getSymbolValue()))
	FC = fprops.fluid(str(self.hxd.component.getSymbolValue()),str(self.hxd.type.getSymbolValue()))

	extpy.getbrowser().reporter.reportNote("Hot fluid is %s, cold fluid is %s" % (FH.name, FC.name))	

	plot_TH(pconsth(self.inlet_hot, self.outlet_hot, 50),'r-',
		Href = (float(self.outlet_hot.h)*float(self.outlet_hot.mdot))\
	)

	plot_TH(pconsth(self.inlet, self.outlet, 50),'b-',
		Href = (float(self.inlet.h)*float(self.inlet.mdot))\
	)

	title(unicode(r"%s-%s heat exchanger" % (FH.name, FC.name)))
	ylabel(unicode(r"T / [°C]"))
	xlabel("H / [MW]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(regenerator_plot_fprops)

