#!/usr/bin/env python
''' This is the main application for the Canvas modeller, it handles the ASCEND solver and the GUI'''

from __future__ import with_statement
import os
import sys
import platform

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk


os.chdir(os.path.abspath(os.path.dirname(sys.argv[0])))

DEFAULT_LIBRARY = 'basic_electronics_model.a4c'

# Remove this sometime
DEFAULT_CANVAS_MODEL_LIBRARY = os.path.join('..', '..', 'models', 'test', 'canvas')

'''Set the required paths'''
try:
	os.environ['ASCENDLIBRARY']
	os.environ['LD_LIBRARY_PATH']
except KeyError:
	os.environ['ASCENDLIBRARY'] = os.path.join('..', '..', 'models')
	os.environ['LD_LIBRARY_PATH'] = os.path.join('..', '..')
os.environ['ASCENDSOLVERS'] = os.path.join('..', '..', 'solvers', 'qrslv')

sys.path.append("..")
sys.path.append("../../ascxx")

if platform.system() == "Windows":
	import _winreg as wreg

	k = wreg.OpenKey(wreg.HKEY_LOCAL_MACHINE, "SOFTWARE\ASCEND")
	INSTALL_LIB, t = wreg.QueryValueEx(k, "INSTALL_LIB")
	INSTALL_SOLVERS, t = wreg.QueryValueEx(k, "INSTALL_SOLVERS")
	INSTALL_MODELS, t = wreg.QueryValueEx(k, "INSTALL_MODELS")
	os.environ['PATH'] = os.environ['PATH'] + INSTALL_LIB
	os.environ['ASCENDLIBRARY'] = INSTALL_MODELS
	os.environ['ASCENDSOLVERS'] = INSTALL_SOLVERS
	DEFAULT_CANVAS_MODEL_LIBRARY = os.path.join(INSTALL_MODELS, 'test', 'canvas')


class Application():
	
	def __init__(self, options):
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
		Gtk.main()

if __name__ == '__main__':
	from optparse import OptionParser
	parser = OptionParser()
	parser.add_option('-f', '--file', dest='file')
	parser.add_option('-l', '--library', dest='library')
	(options, args) = parser.parse_args()
	_Application = Application(options)
	_Application.run()
