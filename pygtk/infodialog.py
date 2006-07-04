# General-purpose popup window for reporting texty stuff

import gtk, gtk.glade
import ascpy
from varentry import *

class InfoDialog:
	def __init__(self,browser,parent,text,title):
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(browser.glade_file,"infodialog")
		self.window = _xml.get_widget("infodialog")
		self.window.set_title(title)

		if self.browser.icon:
			self.window.set_icon(self.browser.icon)

		self.parent = None
		if parent:
			self.parent = parent
			self.window.set_transient_for(self.parent)

		self.textview = _xml.get_widget("textview")
		self.closebutton = _xml.get_widget("closebutton")

		self.textbuff = gtk.TextBuffer();
		self.textview.set_buffer(self.textbuff)

		self.fill_values(text)
		_xml.signal_autoconnect(self)

	def fill_values(self,text):
		self.textbuff.set_text(text);

	def on_infodialog_close(self,*args):
		self.window.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.window.run()
		self.window.hide()
