# General-purpose popup window for reporting texty stuff

import gtk, gtk.glade, pango
import ascpy
from varentry import *

class ImageDialog:
	def __init__(self,browser,parent,imagefilename,title):
		self.browser = browser;
		self.imagefilename = imagefilename

		# GUI config
		_xml = gtk.glade.XML(browser.glade_file,"imagedialog")
		self.window = _xml.get_widget("imagedialog")
		self.vbox = _xml.get_widget("vbox")
		self.closebutton = _xml.get_widget("closebutton")
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

		_xml.signal_autoconnect(self)

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
