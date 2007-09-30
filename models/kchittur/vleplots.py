import extpy
from pylab import *
from solverreporter import *
def txyplot(self):
	browser = extpy.getbrowser()
	ioff()
	figure()
	for P in [70000.0]:
		self.P.setRealValue(P)
		XX1 = []
		TT1 = []
		XX2 = []
		TT2 = []
		for x1 in [0.01,0.05,0.1,0.15,0.2,0.25,0.3,0.35,0.40,0.45,0.50,0.55,0.60,0.65,0.7,0.75,0.8,0.85,0.9,0.95,0.99]:
		    self.x1.setRealValue(x1)
		    try:
				browser.sim.solve(browser.solver,SimpleSolverReporter(browser,message="P = %f, x1 = %f" % (P,x1)))
				XX1.append(float(self.x1))
				TT1.append(float(self.TdegC))
				XX2.append(float(self.y1))
				TT2.append(float(self.TdegC))
		    except:
		        browser.reporter.reportError('Failed to solve for x1 = %f' % x1)
		        continue
		plot(XX1,TT1)
		plot(XX2,TT2)
		hold(1)
	ion()
	show()

extpy.registermethod(txyplot)

def pxyplot(self):
	browser = extpy.getbrowser()
	ioff()
	figure()
	for T in [340]:
		self.T.setRealValue(T)
		XX1 = []
		PP1 = []
		XX2 = []
		PP2 = []
		for x1 in [0.01,0.05,0.1,0.15,0.2,0.25,0.3,0.35,0.40,0.45,0.50,0.55,0.60,0.65,0.7,0.75,0.8,0.85,0.9,0.95,0.99]:
		    self.x1.setRealValue(x1)
		    try:
				browser.sim.solve(browser.solver,SimpleSolverReporter(browser,message="T = %f, x1 = %f" % (T,x1)))
				XX1.append(float(self.x1))
				PP1.append(float(self.P))
				XX2.append(float(self.y1))
				PP2.append(float(self.P))
		    except:
		        browser.reporter.reportError('Failed to solve for x1 = %f' % x1)
		        continue
		plot(XX1,PP1)
		plot(XX2,PP2)
		hold(1)
	ion()
	show()

extpy.registermethod(pxyplot)

