import extpy;

from pylab import *
from solverreporter import *

def fourbarplot(self):
	"""Plot the geometry of the four-bar linkage"""
	# following is an unfortunate necessity in the current system architecture:
	self = ascpy.Registry().getInstance('context')

	browser = extpy.getbrowser()
	browser.do_solve()

	ioff()
	figure()
	hold(True)

	for alpha in range(10,74,4):
		self.alpha.setRealValueWithUnits(alpha,"deg")

		browser.sim.solve(browser.solver,SimpleSolverReporter(browser))

		x = [float(x) for x in [self.x_A, self.x_B, self.x_C, self.x_D]]
		y = [float(y) for y in [self.y_A, self.y_B, self.y_C, self.y_D]]

		plot(x,y,"y-")
		plot(x[0:2],y[0:2],"ro")
		plot(x[2:4],y[2:4],"bo")

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(fourbarplot)
#the above method can be called using "EXTERNAL mypythonmethod(SELF)" in ASCEND.
