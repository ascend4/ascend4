# -*- coding: utf8 -*-
import extpy, sys

try:
	from pylab import *
except:
	pass

def leastsq_plot(self):
	"""Plot a least-squares fit, no added intermediate points though."""
	import loading
	loading.load_matplotlib(throw=True)
	
	ioff()
	figure()

	n = self.n.getIntValue()

	x = []
	y = []
	ye = []
	for i in range(n):
		x.append(float(self.x[i+1]))
		y.append(float(self.y[i+1]))
		ye.append(float(self.f[i+1].y))
	plot(x,y,'bo')
	plot(x,ye,'b-')

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(leastsq_plot)

