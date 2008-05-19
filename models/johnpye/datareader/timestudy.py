# -*- coding: utf8 -*-
import extpy;

from pylab import *
from solverreporter import *

def timestudy(self):

	# following is an unfortunate necessity in the current system architecture:
	browser = extpy.getbrowser()

	# just check that all is ok
	browser.do_solve()

	# set up figure
	ioff()
	figure()

	# calculate time points in advance
	tvalue = float(self.t);
	tincr = 1800 # seconds
	tt = []
	for i in range(1*24*3600/tincr):
		tt.append(tvalue);
		tvalue += tincr

	series = [unicode('G [W/m²]'), unicode('Gbn [W/m²]'), unicode('Gd [W/m²]'),'T [K]', 'v_wind [m/s]']
	xdata = []
	ydata = [[], [], [], [], []]

	for t in tt:
		 self.t.setRealValueWithUnits(t,"s")
		 try:
		     browser.sim.solve(browser.solver,SimpleSolverReporter(browser,"t = %f"%(t)))
		 except:
		     browser.reporter.reportError('Failed to solve for t = %f' % t)
		     continue

		 xdata.append(t / 3600.)
		 ydata[0].append(self.G.as("W/m^2"))
		 ydata[1].append(self.Gbn.as("W/m^2"))
		 ydata[2].append(self.Gd.as("W/m^2"))
		 ydata[3].append(self.T.as("K") - 273.15)
		 ydata[4].append(self.v_wind.as("m/s"))


	subplot(311)
	plot(xdata,ydata[0])
	title("Weather data vs Time")
	ylabel(unicode("Radiation / [W/m²]"))
	hold(1)
	plot(xdata,ydata[1])
	plot(xdata,ydata[2])
	legend(series[0:3])
	grid(1)

	subplot(312)
	plot(xdata,ydata[3])
	ylabel('Temperature [K]')
	grid(1)
	
	subplot(313)
	plot(xdata,ydata[4])
	ylabel('Wind speed [m/s]')
	grid(1)
	xlabel("Time / [h]")

	ion()
	show()

extpy.registermethod(timestudy)
#the above method can be called using "EXTERNAL timestudy(SELF)" in ASCEND.

