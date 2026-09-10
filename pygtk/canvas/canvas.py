#!/usr/bin/env python
''' This is the main application for the Canvas modeller, it handles the ASCEND solver and the GUI'''


import os, sys
from gtkcompat import gtk

CANVAS_DIR = os.path.dirname(os.path.abspath(__file__))
PYGTK_DIR = os.path.dirname(CANVAS_DIR)
SOURCE_ROOT = os.path.dirname(PYGTK_DIR)

DEFAULT_LIBRARY = 'brayton_fprops_rachel.a4c'
DEFAULT_CANVAS_MODEL_LIBRARY = os.path.join(SOURCE_ROOT,'models','test','canvas')


def configure_standalone_environment():
	"""Supply standalone defaults without replacing launcher configuration."""
	os.chdir(CANVAS_DIR)
	os.environ.setdefault('ASCENDLIBRARY', os.path.join(SOURCE_ROOT,'models'))
	os.environ.setdefault('LD_LIBRARY_PATH', SOURCE_ROOT)
	os.environ.setdefault('ASCENDSOLVERS', os.path.join(SOURCE_ROOT,'solvers','qrslv'))
	for path in (PYGTK_DIR, os.path.join(SOURCE_ROOT,'ascxx')):
		if path not in sys.path:
			sys.path.append(path)

	if sys.platform.startswith("win"):
		# Retained only for old Gtk2 Windows setups. Gtk3 should normally
		# already be available on PATH.
		import winreg
		try:
			k = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "Software\\GTK\\2.0")
		except EnvironmentError:
			from tkinter import Label, Tk
			root = Tk()
			w = Label(root,"You must install the Gtk+ 2.2 Runtime Environment to run this program")
			w.pack()
			root.mainloop()
			sys.exit(1)
		else:
			gtkdir = winreg.QueryValueEx(k, "Path")
			os.environ['PATH'] = "%s/lib;%s/bin;" % (gtkdir[0], gtkdir[0]) + os.environ['PATH']
	
class Application(object):
	
	def __init__(self,options):
		from asclibrary import ascPy
		from blocklist import mainWindow
		self.ascwrap = ascPy()
		self.window = mainWindow(self.ascwrap)
		if options.library:
			print(options.library)
			self.window.loadlib(lib_name=options.library)
		else:
			self.window.loadlib(lib_name=DEFAULT_LIBRARY)
		
		if options.file:
			self.window.load_canvas_file(options.file)
			
	def run(self):
		gtk.main()

if __name__ == '__main__':
	configure_standalone_environment()
	from optparse import OptionParser
	parser = OptionParser()
	parser.add_option('-f','--file',dest='file')
	parser.add_option('-l','--library',dest='library')
	(options,args) = parser.parse_args()
	_Application = Application(options)
	_Application.run()
