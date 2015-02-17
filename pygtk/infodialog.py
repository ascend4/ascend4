# General-purpose popup window for reporting texty stuff

from gi.repository import Gtk
from gi.repository import Pango
#import ascpy
#from varentry import *

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
			self.set_transient_for(self.parent)

		self.textview = self.browser.builder.get_object("textview")
		self.closebutton = self.browser.builder.get_object("closebutton")

		if tabs:
			self.setTabs(*tabs)

		self.textbuff = Gtk.TextBuffer();
		self.textview.set_buffer(self.textbuff)

		self.fill_values(text)
        #self.browser.builder.connect_signals(self)

	def setTabs(self,*args):
		n = len(args)
		t = Pango.TabArray(n,True)
		i = 0
		for v in args:
			t.set_tab(i,Pango.TabAlign.LEFT,v)
			i+=1;
		self.textview.set_tabs(t)

	def fill_values(self,text):
		self.textbuff.set_text(text);

	def on_infodialog_close(self,*args):
		self.window.response(Gtk.ResponseType.CLOSE);

	def run(self):
		self.window.run()
		self.window.hide()


glade_file = "Specify the glade file here"

class T:
    def __init__(self):
        self.glade_file = glade_file
        self.icon = None
        builder = Gtk.Builder()
        builder.add_objects_from_file(glade_file,["integ_icon","browserwin","list_of_td"])
        self.builder=builder
        self.window=self.builder.get_object ("browserwin")


def main():
    t = T()
    _d = InfoDialog(t,t.window,"text","title")
    _d.run()
    
    
if __name__ == "__main__":
    main()

