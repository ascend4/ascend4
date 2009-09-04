import gtk, gtk.glade, pango
import blockcanvas

class MethodEntry:
	def __init__(self):
		self.methodd = gtk.Dialog(
			title="Custom METHOD entry - ASCEND Canvas",
			flags = (gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT),
			buttons = None)
		self.methodd.set_default_size(600,500)
		self.text = None
		self.flag = 0
		self.tb = gtk.TextBuffer()
		self.tb.set_text(" (* Enter your custom METHOD below *)\n")
		self.textview = gtk.TextView(self.tb)
		self.textview.set_editable(True)
		sb = gtk.ScrolledWindow()
		sb.add_with_viewport(self.textview)
		sb.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		sb.show_all()	
		self.methodd.vbox.pack_start(sb)
		cancel = self.methodd.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		ok = self.methodd.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		ok.connect("clicked", self.storemethod)
		self.methodd.show_all()
		
		
	def on_close(self,*args):
		self.methodd.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.methodd.run()
		self.methodd.hide()
		
	def storemethod(self, widget):
		startiter = self.tb.get_start_iter()
		enditer = self.tb.get_end_iter()
		if self.tb.get_text(startiter, enditer) == " (* Enter your custom METHOD below *)\n" :
			pass
		else:
			self.text = "METHOD user_code;\n"
			self.text += self.tb.get_text(startiter, enditer)
			self.text += "\nEND user_code;"
			blockcanvas.ucflag = 1
			#global usercode 
			blockcanvas.usercode = self.text
			print blockcanvas.usercode
		
if __name__=='__main__':
	MethodEntry()
	gtk.main()	