import extpy;
browser = extpy.getbrowser()

def mypythonmethod(inst):
	"""I don't want to talk to you no more, you empty headed animal food trough wiper!"""
	browser.reporter.reportNote("No, now go away or I shall taunt you a second time!")

extpy.registermethod(mypythonmethod)
