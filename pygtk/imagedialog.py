# General-purpose popup window for reporting texty stuff

import gtk, pango
import ascpy
from varentry import *

class ImageDialog:
	def __init__(self,browser,parent,imagefilename,title):
		self.browser = browser;
		self.imagefilename = imagefilename

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["imagedialog"])
		self.window = self.browser.builder.get_object("imagedialog")
		self.vbox = self.browser.builder.get_object("vbox")
		self.closebutton = self.browser.builder.get_object("closebutton")
		self.window.set_title(title)

		if self.browser.icon:
			self.window.set_icon(self.browser.icon)

		self.parent = None
		if parent:
			self.parent = parent
			self.window.set_transient_for(self.parent)
		
		s = gtk.ScrolledWindow()
		self.imageview = gtk.Image()
		self.imageview.set_from_file(imagefilename)
		s.add_with_viewport(self.imageview)
		self.imageview.show()
		s.show()
		self.vbox.add(s)

		self.browser.builder.connect_signals(self)
		#self.browser.builder.connect_signals("vbox")
		#self.browser.builder.connect_signals("closebutton")

	def on_savebutton_clicked(self,*args):
		self.browser.reporter.reportNote("SAVE %s" % self.imagefilename)
		self.window.response(gtk.RESPONSE_NONE)

	def on_zoom_change_value(self,*args):
		self.browser.reporter.reportNote("ZOOM");

	def on_infodialog_close(self,*args):
		self.window.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.window.run()
		self.window.hide()
