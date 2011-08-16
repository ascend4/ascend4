# General-purpose popup window for reporting texty stuff

import gtk, pango
import ascpy
from varentry import *

class InfoDialog:
	def __init__(self,browser,parent,text,title,tabs=None):
		self.browser = browser;

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file,["infodialog"])
		self.window = self.browser.builder.get_object("infodialog")
		#self.window.set_visible(True)
		self.window.set_title(title)

		if self.browser.icon:
			self.window.set_icon(self.browser.icon)

		self.parent = None
		if parent:
			self.parent = parent
			self.window.set_transient_for(self.parent)

		self.textview = self.browser.builder.get_object("textview")
		self.closebutton = self.browser.builder.get_object("closebutton")

		if tabs:
			self.setTabs(*tabs)

		self.textbuff = gtk.TextBuffer();
		self.textview.set_buffer(self.textbuff)

		self.fill_values(text)
		self.browser.builder.connect_signals(self)

	def setTabs(self,*args):
		n = len(args)
		t = pango.TabArray(n,True)
		i = 0
		for v in args:
			t.set_tab(i,pango.TAB_LEFT,v)
			i+=1;
		self.textview.set_tabs(t)

	def fill_values(self,text):
		self.textbuff.set_text(text);

	def on_infodialog_close(self,*args):
		self.window.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.window.run()
		self.window.hide()
