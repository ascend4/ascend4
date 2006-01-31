import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango

class ObserverTab:
	def __init__(self,name,browser):
		self.name = name
		self.browser = browser
	
	def on_add_click(self,*args):
		print "ADD"
		pass

	def on_clear_click(self,*args):
		print "CLEAR"
		pass

