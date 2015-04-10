from gi.repository import Gtk
import blockcanvas

class MethodEntry:
	def __init__(self):
		self.methodd = Gtk.Dialog(
			title="Custom METHOD entry - ASCEND Canvas",
			flags = (Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT))
		self.methodd.set_default_size(600,500)
		self.text = None
		self.flag = 0
		self.tb = Gtk.TextBuffer()
		self.tb.set_text(" (* Enter your custom METHOD below *)\n")
		self.textview = Gtk.TextView.new_with_buffer(self.tb)
		self.textview.set_editable(True)
		sb = Gtk.ScrolledWindow()
		sb.add_with_viewport(self.textview)
		sb.set_policy(Gtk.PolicyType.AUTOMATIC,Gtk.PolicyType.AUTOMATIC)
		sb.show_all()	
		self.methodd.vbox.pack_start(sb, True, True, 0)
		cancel = self.methodd.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
		ok = self.methodd.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
		ok.connect("clicked", self.storemethod)
		self.methodd.show_all()
		
		
	def on_close(self,*args):
		self.methodd.response(Gtk.ResponseType.CLOSE);

	def run(self):
		self.methodd.run()
		self.methodd.hide()
		
	def storemethod(self, widget):
		startiter = self.tb.get_start_iter()
		enditer = self.tb.get_end_iter()
		if self.tb.get_text(startiter, enditer, True) == " (* Enter your custom METHOD below *)\n" :
			pass
		else:
			self.text = "METHOD user_code;\n"
			self.text += self.tb.get_text(startiter, enditer, True)
			self.text += "\nEND user_code;"
			blockcanvas.ucflag = 1
			#global usercode 
			blockcanvas.usercode = self.text
			print blockcanvas.usercode
		
if __name__=='__main__':
	MethodEntry()
	Gtk.main()	