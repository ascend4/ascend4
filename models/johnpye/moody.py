import ascpy
import extpy
browser = extpy.getbrowser()

from pylab import *
from solverreporter import *

def setsomevalue(self):
	""" a silly listing testing routine """
	self = ascpy.Registry().getInstance('context')
	f = 0.005
	self.f.setRealValue(f)
	s = "Set f to %f (confirmed as: %f)" % (f, self.f.getRealValue())
	print s
	browser.reporter.reportNote(s)

def moodyplot(self):
	""" repeatedly solve the colebrook equation to plot a Moody diagram """
	self = ascpy.Registry().getInstance('context')
	
	self.eps.setFixed(False)
	self.eps_on_D.setFixed(True)

	ioff()
	figure()
	hold(True)
	leg = []
	Re_vals = array([1000,1500,2000,2100,2300,2400,2500,2600,2700,2900,3000,4000,5000,10000,20000,50000,100000,200000,500000,1e6])

	#browser.reporter.reportNote(str(len(Re_vals)))
	for eps_on_D in [1e-5,2e-5,5e-5,1e-4,2e-4,5e-4,1e-3,5e-3,0.01,0.02]:
		self.eps_on_D.setRealValue(eps_on_D)
		f_vals = zeros(size(Re_vals),'f')
		for i in range(0,len(Re_vals)):
			self.Re.setRealValue(Re_vals[i])
			browser.sim.solve(ascpy.Solver("QRSlv"),SimpleSolverReporter(browser))
			f_vals[i] = self.f.getRealValue()

		loglog(Re_vals,f_vals)
		leg += ["e/D = %f" % eps_on_D]

	legend(leg)
	ion()
	show()

def checksomevalue(self):
	""" a silly listing testing routine """
	self = ascpy.Registry().getInstance('context')
	s = "Value of f = %f" % self.f.getRealValue()
	print s
	browser.reporter.reportNote(s)
			
extpy.registermethod(moodyplot)
extpy.registermethod(checksomevalue)
extpy.registermethod(setsomevalue)
