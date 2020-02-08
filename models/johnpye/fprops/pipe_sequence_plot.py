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
	pl.subplot(211)
	pl.hold(1)

	extpy.getbrowser().reporter.reportNote("Gathering values...")

	D = fprops.fluid(str(self.cd.component.getSymbolValue()))
	
	n = self.n.getIntValue()
	p = np.zeros((n+1))
	T = np.zeros((n+1))
	L = np.zeros((n))
	for i in range(n):
		p[i] = float(self.P[i+1].inlet.p) / 1e5
		T[i] = float(self.P[i+1].inlet.T)
		L[i] = float(self.P[i+1].L)
	p[n] = float(self.P[n].outlet.p) / 1e5;
	T[n] = float(self.P[i+1].outlet.T)

	extpy.getbrowser().reporter.reportNote("Plotting...")

	x = np.zeros((n+1))
	x[0] = 0;
	for i in range(n):
		x[i+1] = x[i] + L[i]
	
	plot(x,p,'bo-');
	ylabel(unicode(r"p / [bar]"))
	subplot(212)
	plot(x,T,'ro-');
	ylabel(unicode(r"T / [K]"))

	title(unicode(r"Pipe heat loss and pressure drop (fluid %s)" % D.name))
	xlabel("x / [m]")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(pipe_sequence_plot)

