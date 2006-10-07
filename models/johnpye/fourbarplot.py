import extpy;
browser = extpy.getbrowser()

from pylab import *

def fourbarplot(self):
	"""Plot the geometry of the four-bar linkage"""
	# following is an unfortunate necessity in the current system architecture:
	self = ascpy.Registry().getInstance('context')

	extpy.getbrowser().do_solve()

	ioff()
	figure()
	hold(True)

	for alpha in [10,20,30]:
		self.alpha.setRealValueWithUnits(alpha,"deg")

		extpy.getbrowser().do_solve()

		x = [float(x) for x in [self.x_A, self.x_B, self.x_C, self.x_D]]
		y = [float(y) for y in [self.y_A, self.y_B, self.y_C, self.y_D]]

		plot(x,y,"r-")
		plot(x,y,"bo")

	browser.reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(fourbarplot)
#the above method can be called using "EXTERNAL mypythonmethod(SELF)" in ASCEND.
