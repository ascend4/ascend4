import sys
import config
import os.path

global have_gtk
have_gtk = False
try:
	import pygtk 
	pygtk.require('2.0') 
	import gtk
	have_gtk = True
except:
	print "PyGTK COULD NOT BE LOADED (is it installed? do you have X-Windows running?)"
	sys.exit(1)

global _messages
_messages = []

def get_messages():
	return _messages

def load_matplotlib(throw=False,alert=False):
	print_status("Loading python matplotlib")
	try:
		import matplotlib
		matplotlib.use('GTKAgg')

		try:
			print_status("Trying python numpy")
			import numpy
			matplotlib.rcParams['numerix'] = 'numpy'  
			print_status("","Using python module numpy")
		except ImportError:
			try:
				print_status("Trying python numarray")
				import numarray
				matplotlib.rcParams['numerix'] = 'numarray'  
				print_status("","Using python module numarray")
			except ImportError:
				try:
					print_status("Trying python Numeric")
					import Numeric
					matplotlib.rcParams['numerix'] = 'Numeric'  
					print_status("","Using python module Numeric")
				except ImportError:
					print_status("","FAILED TO LOAD A NUMERIC MODULE FOR PYTHON")

	except ImportError,e:
		print_status("","FAILED TO LOAD MATPLOTLIB")
		if alert or throw:
			_d = gtk.MessageDialog(None,gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT
				,gtk.MESSAGE_ERROR,gtk.BUTTONS_CLOSE,"Plotting functions are not available unless you have 'matplotlib' installed.\n\nSee http://matplotlib.sf.net/\n\nFailed to load matplotlib (%s)" % str(e)
			)
			_d.run()
			_d.destroy()
			while gtk.events_pending():
				gtk.main_iteration(False)		
		if throw:
			raise RuntimeError("Failed to load plotting library 'matplotlib'. (%s)" % str(e))


class LoadingWindow:
	def __init__(self):
		self.is_loading = False
		self.set_assets_dir(config.PYGTK_ASSETS)

	def set_assets_dir(self, d):
		self.assetsdir = d
		self.splashfile = os.path.join(self.assetsdir,'ascend-loading.svg')

	def create_window(self):
		if have_gtk:
			if os.path.exists(self.splashfile):
				_w = gtk.Window(gtk.WINDOW_TOPLEVEL)
				_w.set_decorated(False)
				_w.set_position(gtk.WIN_POS_CENTER)
				_a = gtk.Alignment()
				_a.set_padding(4,4,4,4)
				_w.add(_a)
				_a.show()
				_v = gtk.VBox()
				_a.add(_v)
				_v.show()
				_i = gtk.Image()
				self.image = _i
				_i.set_pixel_size(3)
				_i.set_from_file(self.splashfile)
				_v.add(_i)
				_i.show()
				_l = gtk.Label("Loading ASCEND...")
				_l.set_justify(gtk.JUSTIFY_CENTER)
				_v.add(_l)
				_l.show()
				_w.show()
				self.window = _w
				self.label = _l
				self.is_loading = True
				while gtk.events_pending():
					gtk.main_iteration(False)
			else:
				pass
				#do nothing, don't know where splash file is yet
		else:
			print "DON'T HAVE GTK!"
			sys.exit(1)
	
	def print_status(self,status,msg=None):
		if self.is_loading:
			print status
			self.label.set_text(status)
			if msg is not None:
				sys.stderr.write(msg+"\n")
				_messages.append(msg)
			while gtk.events_pending():
				gtk.main_iteration(False)
		else:
			sys.stderr.write("\r                                                 \r")
			if msg!=None:
				sys.stderr.write(msg+"\n")
				_messages.append(msg)
			sys.stderr.write(status+"...\r")
			sys.stderr.flush()

	def complete(self):
		if self.is_loading:
			self.window.destroy()
		self.is_loading = False

global w

def print_status(status,msg=None):
	w.print_status(status,msg)

def complete():
	w.complete()

def create_window(assetsdir=config.PYGTK_ASSETS):
	w.set_assets_dir(assetsdir)
	w.create_window()

w = LoadingWindow()
create_window()
