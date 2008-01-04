
import gtk, gtk.glade, pango

class UnitsDialog:

	def __init__(self,browser,parent=None):
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(browser.glade_file,"unitsdialog")
		self.window = _xml.get_widget("unitsdialog")

		self.parent = parent
		if parent:
			self.window.set_transient_for(self.parent)

	def run(self):
		self.window.run()
		self.window.hide()

