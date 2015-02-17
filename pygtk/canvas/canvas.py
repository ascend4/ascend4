#!/usr/bin/env python
''' This is the main application for the Canvas modeller, it handles the ASCEND solver and the GUI'''

from __future__ import with_statement
import os, sys
import gtk

os.chdir(os.path.abspath(os.path.dirname(sys.argv[0])))

DEFAULT_LIBRARY = 'rankine_canvas.a4c'

#Remove this sometime
DEFAULT_CANVAS_MODEL_LIBRARY = os.path.join('..','..','models','test','canvas')

'''Set the required paths'''
try:
	os.environ['ASCENDLIBRARY']
	os.environ['LD_LIBRARY_PATH']
except KeyError:
	os.environ['ASCENDLIBRARY'] = os.path.join('..','..','models')
	os.environ['LD_LIBRARY_PATH'] = os.path.join('..','..')
os.environ['ASCENDSOLVERS'] = os.path.join('..','..','solvers','qrslv')

sys.path.append("..")
sys.path.append("../../ascxx")

if sys.platform.startswith("win"):
    # Fetchs gtk2 path from registry
	import _winreg
	import msvcrt
	try:
		k = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "Software\\GTK\\2.0")
	except EnvironmentError:
		# use TkInter to report the error :-)
		from TkInter import *
		root = Tk()
		w = Label(root,"You must install the Gtk+ 2.2 Runtime Environment to run this program")
		w.pack()
		root.mainloop()
		sys.exit(1)
	else:
		gtkdir = _winreg.QueryValueEx(k, "Path")
        import os
        # we must make sure the gtk2 path is the first thing in the path
        # otherwise, we can get errors if the system finds other libs with
        # the same name in the path...
        os.environ['PATH'] = "%s/lib;%s/bin;" % (gtkdir[0], gtkdir[0]) + os.environ['PATH']
	
class Application(object):
	
	def __init__(self,options):
		from asclibrary import ascPy
		from blocklist import mainWindow
		self.ascwrap = ascPy()
		self.window = mainWindow(self.ascwrap)
		if options.library:
			print options.library
			self.window.loadlib(lib_name=options.library)
		else:
			self.window.loadlib(lib_name=DEFAULT_LIBRARY)
		
		if options.file:
			self.window.load_canvas_file(options.file)
			
	def run(self):
		gtk.main()

if __name__ == '__main__':
	from optparse import OptionParser
	parser = OptionParser()
	parser.add_option('-f','--file',dest='file')
	parser.add_option('-l','--library',dest='library')
	(options,args) = parser.parse_args()
	_Application = Application(options)
	_Application.run()
