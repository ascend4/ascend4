import extpy;
browser = extpy.getbrowser()

import ascpy;
import matplotlib;

def mypythonmethod(self):
	"""I don't want to talk to you no more, you empty headed animal food trough wiper!"""
	# the above python docstring is visible as the method 'help' in ASCEND.

	# we hope to be able to get rid of this first line, but so far it's proved
	# difficult from the architectural point of view:
	self = ascpy.Registry().getInstance('context')

	# iterating through the children of an instance
	print "CHILDREN OF INSTANCE '%s':" % self.getName()
	for i in self.getChildren():
		print i.getName()," = ",i.getValue()

	# finding a specific child instance
	print "TEST OF ABILITY TO GET SPECIFIC CHILD VALUES:"
	print self.a,"=",self.a.getValue()

	self.x_1.setFixed(not self.x_1.isFixed())
	self.x_2.setFixed(not self.x_2.isFixed())

	self.y_1.setFixed(not self.x_1.isFixed())
	self.y_2.setFixed(not self.x_2.isFixed())

	print "SETTING VALUE OF X_2"
	self.x_2.setRealValueWithUnits(2.0,"m")

	print "X_2 = %f" % self.x_2

	if browser:
		browser.reporter.reportNote("No, now go away or I shall taunt you a second time!")
	else:
		print "No, now go away or I shall taunt you a second time!"

extpy.registermethod(mypythonmethod)
#the above method can be called using "EXTERNAL mypythonmethod(SELF)" in ASCEND.
