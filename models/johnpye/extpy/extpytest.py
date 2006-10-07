import extpy;
browser = extpy.getbrowser()

import ascpy;

def mypythonmethod(self):
	# we hope to be able to get rid of this first line, but so far it's proved
	# difficult from the architectural point of view:
	self = ascpy.Registry().getInstance('context')

	print "CHILDREN OF INSTANCE '%s':" % self.getName()
	for i in self.getChildren():
		print i.getName()," = ",i.getValue()

	print "TEST OF ABILITY TO GET SPECIFIC CHILD VALUES:"
	print self.a,"=",self.a.getValue()

	"""I don't want to talk to you no more, you empty headed animal food trough wiper!"""
	browser.reporter.reportNote("No, now go away or I shall taunt you a second time!")

extpy.registermethod(mypythonmethod)
