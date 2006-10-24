import ascpy
import extpy
browser = extpy.getbrowser()

def listnotes(self):
	""" make a list of NOTES for the present model """
	self = ascpy.Registry().getInstance('context')

	db = browser.library.getAnnotationDatabase()
	notes = db.getNotes(self.getType())

	for i in range(1,len(notes)):
		mm = notes[i].getMethod()
		ii = notes[i].getId()
		tt = notes[i].getText()
		s = "type = %s, method = %s, id = %s, text = %s" % (notes[i].getType(), mm, ii, tt)
		print "NOTES:",s
		browser.reporter.reportNote(s)

extpy.registermethod(listnotes)
