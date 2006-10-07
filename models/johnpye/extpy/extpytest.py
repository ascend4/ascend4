import extpy;
browser = extpy.getbrowser()

import ascpy;

def mypythonmethod(inst):
	inst = ascpy.Registry().getInstance('context')
	print "HELLO FROM PYTHON"
	for i in inst.getChildren():
		print i.getName()," = ",i.getValue()

	"""I don't want to talk to you no more, you empty headed animal food trough wiper!"""
	browser.reporter.reportNote("No, now go away or I shall taunt you a second time!")

extpy.registermethod(mypythonmethod)
