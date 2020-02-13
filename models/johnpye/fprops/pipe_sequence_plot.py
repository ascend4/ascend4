# -*- coding: utf8 -*-
import extpy, sys
from solverreporter import *
from __builtin__ import *
import sys, os, os.path
if 'ASCENDLIBRARY' in os.environ:
	p1 = os.path.join(os.environ['ASCENDLIBRARY'],'johnpye/fprops/python')
	print "ADDING PATH",p1
	sys.path.append(p1)
import fprops

try:
	import matplotlib.pyplot as pl
	import numpy as np
except:
	pass

#--- heat exchange (T,H) plot ---

def pipe_sequence_plot(self):
	"""Plot T-H diagram of heat transfer in a heater_closed model"""
	import loading
	loading.load_matplotlib(throw=True)
	import matplotlib.pyplot as pl
	import numpy as np

	pl.ioff()
	pl.subplot(311)
	pl.hold(1)

	extpy.getbrowser().reporter.reportNote("Gathering values...")

	D = fprops.fluid(str(self.cd.component.getSymbolValue()))
	
	n = self.n.getIntValue()
	p = np.zeros((n+1))
	T = np.zeros((n+1))
	L = np.zeros((n))
	Q = np.zeros((n))
	T_ext = np.zeros((n))
	for i in range(n):
		p[i] = float(self.P[i+1].inlet.p) / 1e5
		T[i] = float(self.P[i+1].inlet.T)
		L[i] = float(self.P[i+1].L)
		Q[i] = float(self.P[i+1].Q)
		T_ext[i] = float(self.P[i+1].T_ext)
	p[n] = float(self.P[n].outlet.p) / 1e5;
	T[n] = float(self.P[i+1].outlet.T)

	extpy.getbrowser().reporter.reportNote("Plotting...")

	x = np.zeros((n+1))
	x[0] = 0;
	for i in range(n):
		x[i+1] = x[i] + L[i]
	
	pl.plot(x,p,'bo-');
	pl.ylabel(unicode(r"p / [bar]"))
	pl.subplot(312)
	pl.plot(x,T,'ro-');
	pl.plot(x[1:],T_ext,'go-')
	pl.ylabel(unicode(r"T / [K]"))

	pl.subplot(313)
	pl.plot(x[1:],Q,'ro-')
	pl.ylabel(unicode(r"Q / [W]"))

	pl.title(unicode(r"Pipe heat loss and pressure drop (fluid %s)" % D.name))
	pl.xlabel("x / [m]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	pl.ion()
	pl.show()

def table_to_text(t):
	#import tabulate
	return "\n".join(["\t".join([str(c) for c in r]) for r in t]) + "\n"
def float_C(n):
	return float(n)-273.15

def disharray_temperature_list(self):
	n_DI = self.n_DI.getIntValue()
	n_PH = self.n_PH.getIntValue()
	t = []
	t.append(["Location","T / [°C]","p / [bar]"])
	import ascpy
	for i in range(n_DI):
		t.append(["DI[%d].inlet"%i, float_C(self.DI[i].inlet.T), float(self.DI[i].inlet.p)/1e5])
		t.append(["DI[%d].outlet"%i, float_C(self.DI[i].outlet.T), float(self.DI[i].outlet.p)/1e5])
		t.append(["TH[%d].outlet"%i, float_C(self.TH[i].outlet.T), float(self.DI[i].outlet.p)/1e5])
	t.append(["inlet", float_C(self.inlet.T), float(self.inlet.p)/1e5])
	t.append(["outlet", float_C(self.outlet.T), float(self.outlet.p)/1e5])
	s = table_to_text(t) + "\n"
	
	t = []
	t.append(["Pipe","T_in / [°C]","T_out / [°C]","L / [m]", "D_i / [m]","t_pipe / [m]","Vel_out / [m/s]"])
	for i in range(n_PH):
		d = {'PC[%d]'%i : self.PC[i], 'PH[%d]'%i : self.PH[i]}
		for k,n in d.iteritems():
			t.append([k] + [float(j) for j in [n.T_in_C, n.T_out_C, n.L, n.D, n.t_pipe, n.Vel_out]])
	s += table_to_text(t) + "\n"
	extpy.getbrowser().show_info(s,"csparray temperatures")

extpy.registermethod(pipe_sequence_plot)
extpy.registermethod(disharray_temperature_list)


