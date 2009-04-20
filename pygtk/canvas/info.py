# General-purpose popup window for reporting texty stuff

import gtk, gtk.glade, pango
import ascpy

class Info:
	def __init__(self,parent,text,title,tabs=None,icon=None):
		print "parent =",parent
		self.dialog = gtk.Dialog(
			title=title,
			#parent=parent, # for some reason, the parent window isn't working
			flags = (gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)
			,buttons = (gtk.STOCK_OK, gtk.RESPONSE_ACCEPT)
		)
		self.dialog.set_default_size(600,500)

		tb = gtk.TextBuffer()
		tb.set_text(text)	
		print "text =",text
		self.textview = gtk.TextView(tb)
		self.textview.set_editable(False)
		sb = gtk.ScrolledWindow()
		sb.add_with_viewport(self.textview)
		sb.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		sb.show_all()	
		self.dialog.vbox.pack_start(sb)
	
		self.dialog.connect("close",self.on_close)

		if tabs:
			self.setTabs(*tabs)

	def setTabs(self,*args):
		n = len(args)
		t = pango.TabArray(n,True)
		i = 0
		for v in args:
			t.set_tab(i,pango.TAB_LEFT,v)
			i+=1;
		self.textview.set_tabs(t)

	def on_close(self,*args):
		self.dialog.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.dialog.run()
		print "FINISHED DIALOG"
		self.dialog.hide()

