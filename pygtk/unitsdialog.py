import gtk, gtk.glade, pango, gobject, re

class UnitsDialog:

	def __init__(self,browser,parent=None):
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(browser.glade_file,"unitsdialog")
		self.window = _xml.get_widget("unitsdialog")

		self.typecombo = _xml.get_widget("typecombo")
		self.dimensionlabel = _xml.get_widget("dimensionlabel")

		self.parent = parent
		if parent:
			self.window.set_transient_for(self.parent)

		_xml.signal_autoconnect(self)

		self.unitsdict = self.browser.library.getUnits()
		self.realtypes = self.browser.library.getRealAtomTypes()
		self.update_typecombo()

	def update_typecombo(self,text = None):
		m = gtk.ListStore(gobject.TYPE_STRING)
		for t in self.realtypes:
			if not text or re.compile("^%s"%re.escape(text)).match(str(t.getName())):
				m.append([t.getName()])
		self.typecombo.set_model(m)
		if text and m.iter_n_children(None):
			self.typecombo.popup()
			self.typecombo.child.grab_focus()
		self.typecombo.set_text_column(0)

	def on_typecombo_changed(self,widget,*args):
		s = widget.get_active_text()
		self.update_typecombo(s)
		self.browser.reporter.reportNote("value changed to '%s'" % s)
		T = self.browser.library.findType(widget.get_active_text())
		self.dimensionlabel.set_text(str(T.getDimensions()))
		
	def run(self):
		self.window.run()
		self.window.hide()

