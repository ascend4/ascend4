import sys, os
import pathlib

global have_gtk
have_gtk = False

if not sys.executable.endswith("pythonw.exe"):
	pass #print("PYTHON PATH =",sys.path)

try:
	import gi 
	gi.require_version('Gtk', '3.0') 
	from gi.repository import Gtk
	have_gtk = True
except Exception as e:
	if sys.platform=="win32":
		try:
			from ctypes import c_int, WINFUNCTYPE, windll
			from ctypes.wintypes import HWND, LPCSTR, UINT
			prototype = WINFUNCTYPE(c_int, HWND, LPCSTR, LPCSTR, UINT)
			paramflags = (1, "hwnd", 0), (1, "text", "Hi"), (1, "caption", None), (1, "flags", 0)
			MessageBox = prototype(("MessageBoxA", windll.user32), paramflags)
			MessageBox(text="""ASCEND could not load PyGI. Probably this is because
either PyGI, PyCairo, PyGObject or GTK3+ are not installed on your
system. Please try re-installing ASCEND to rectify the problem.""")
		except:	
			pass
	else:
		print("PyGI COULD NOT BE LOADED (is it installed? do you have X-Windows running?) (%s)" % str(e))
		
	sys.exit("FATAL ERROR: PyGI not available, unable to start ASCEND.")

global _messages
_messages = []

def get_messages():
	return _messages

def load_matplotlib(throw=False,alert=False):
	print_status("Loading python matplotlib")
	try:
		import matplotlib
        # Added a new backend backend_gtk3. File bundle exists in PYTHONPATH
		matplotlib.use('GTK3Cairo')
		try:
			print_status("Trying python numpy")
			import numpy
			print_status("")
		except ImportError:
			print_status("","FAILED to load Python module 'numpy'")
		import pylab


	except ImportError as e:
		print_status("","FAILED TO LOAD MATPLOTLIB")
		if alert or throw:
			_d = Gtk.MessageDialog(None,Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT
				,Gtk.MessageType.ERROR,Gtk.ButtonsType.CLOSE,"Plotting functions are not available unless you have 'matplotlib' installed.\n\nSee http://matplotlib.sf.net/\n\nFailed to load matplotlib (%s)" % str(e)
			)
			_d.run()
			_d.destroy()
			while Gtk.events_pending():
				Gtk.main_iteration_do(False)
		if throw:
			raise RuntimeError("Failed to load plotting library 'matplotlib'. (%s)" % str(e))

class LoadingWindow(Gtk.Window):
	def __init__(self):
		self.is_loading = False
		Gtk.Window.__init__(self, title="Splash Screen")
		self.set_decorated(False)
		self.set_default_size(300, 200)
		self.set_position(Gtk.WindowPosition.CENTER)
		_a = Gtk.Alignment.new(0.5,0.5,0,0)
		_a.set_padding(4,4,4,4)
		self.add(_a)
		_v = Gtk.VBox()
		_a.add(_v)
		# FIXME check if this path calculation always works or not...
		splash = pathlib.Path(__file__).parent.parent/"pygtk"/"glade"/"ascend-loading.png"
		image = Gtk.Image.new_from_file(str(splash))
		_v.add(image)
		self.label = Gtk.Label(label="Loading ASCEND...")
		self.label.set_justify(Gtk.Justification.CENTER)
		_v.add(self.label)
		self.show_all()
		self.is_loading = True
	
	def print_status(self,status,msg=None):
		if self.is_loading:
			self.label.set_text(status)
			while Gtk.events_pending():
				Gtk.main_iteration()
		try:
			sys.stderr.write(f"\rCLR:                                                 \r")
			if msg:
				sys.stderr.write(f"MSG: {msg}\n")
				_messages.append(msg)
			sys.stderr.write(f"\rSTA: {status}\r")
			sys.stderr.flush()
		except IOError:
			pass

	def complete(self):
		if self.is_loading:
			self.destroy()
		self.is_loading = False

def print_status(status,msg=None):
	w.print_status(status,msg)

def complete():
	w.destroy()

global w
w = LoadingWindow()

