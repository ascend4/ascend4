from gi.repository import Gtk
from gi.repository import Pango
import re
from celsiusunits import CelsiusUnits
from preferences import Preferences


class UnitsDialog:

	def __init__(self,browser,T=None):
		"""create units browser for a atom type T, or None"""
		self.browser = browser;

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["unitsdialog"])
		self.window = self.browser.builder.get_object("unitsdialog")
		self.typecombo = self.browser.builder.get_object("typecombo")
		self.dimensionlabel = self.browser.builder.get_object("dimensionlabel")
		self.unitsview = self.browser.builder.get_object("unitsview")
		self.applybutton = self.browser.builder.get_object("units_applybutton")

		self.applybutton.set_sensitive(False)

		self.window.set_transient_for(self.browser.window)

		self.browser.builder.connect_signals(self)

		self.units = self.browser.library.getUnits()
		self.realtypes = self.browser.library.getRealAtomTypes()

		if not len(self.realtypes):
			self.browser.reporter.reportError("No dimensioned atom types available yet (have you loaded a model yet?)")
			return
			#raise RuntimeError("no units available")

		# set up columns in the units view:
		_renderer0 = Gtk.CellRendererToggle()
		_renderer0.set_radio(True)
		_renderer0.connect("toggled",self.unitsview_row_toggled)
		_col0 = Gtk.TreeViewColumn("",_renderer0,active=0)
		self.unitsview.append_column(_col0)
	
		_renderer1 = Gtk.CellRendererText()	
		_col1 = Gtk.TreeViewColumn("Units", _renderer1, text=1, weight=3)
		self.unitsview.append_column(_col1)

		# value column: 'editable' set by column 3 of the model data.
		_renderer2 = Gtk.CellRendererText()	
		_col2 = Gtk.TreeViewColumn("Conversion", _renderer2, text=2)
		self.unitsview.append_column(_col2)

		self.changed = {}
		self.T = T
		model = Gtk.ListStore(str)
		self.typecombo.set_model(model)
		if T is not None:
			if T.isRefinedReal():
				self.typecombo.append_text(str(T.getName()))
		self.update_typecombo(str(T.getName()))
		self.typecombo.set_active(0)

	def unitsview_row_toggled(self,widget,path,*args):
		i = self.unitsview.get_model().get_iter_from_string(path)
		n = self.unitsview.get_model().get_value(i,1)
		j = self.unitsview.get_model().get_iter_first()
		while j is not None:
			self.unitsview.get_model().set_value(j,0,False)
			j = self.unitsview.get_model().iter_next(j)
		self.unitsview.get_model().set_value(i,0,True)
		self.changed[self.typecombo.get_active_text()]=n
		#self.browser.reporter.reportNote("Units for '%s' set to '%s'" % (self.typecombo.get_active_text(),n))
		self.update_applybutton()	

	def update_applybutton(self):
		can_apply = False
		#print "changed = ",self.changed
		for k,v in self.changed.items():
			T = self.browser.library.findType(k)
			u = T.getPreferredUnits()
			if u is None or str(T.getPreferredUnits().getName()) != v:
				#print "CAN APPLY: for type '%s', pref units currently '%s', now selected '%s'" % (T.getName(),T.getPreferredUnits().getName(), v)
				can_apply = True
				break
		##### CELSIUS TEMPERATURE WORKAROUND
		if str(T.getDimensions()) == 'TMP':
			units = Preferences().getPreferredUnitsOrigin(str(T.getName()))
			if units == CelsiusUnits.get_celsius_sign():
				if units != list(self.changed.items())[0][1]:
					can_apply = True
				else:
					can_apply = False
		##### CELSIUS TEMPERATURE WORKAROUND
		self.applybutton.set_sensitive(can_apply)

	def update_typecombo(self,text = None):
		m = self.typecombo.get_model()
		for t in self.realtypes:
			if not text and re.compile("^%s" % re.escape(text)).match(str(t.getName())):
				m.append([str(t.getName())])
		if text and m.iter_n_children(None):
			self.typecombo.get_child().grab_focus()
			self.typecombo.set_entry_text_column(0)

	def update_unitsview(self,T):
		m = Gtk.ListStore(bool,str,str,int)
		if T is not None:
			d = T.getDimensions()
			up = T.getPreferredUnits()
			if up is None:
				print("no preferred units")
			else:
				print("preferred units =",up.getName())
		else:
			up = None
		##### CELSIUS TEMPERATURE WORKAROUND
		if str(d) == 'TMP':
			units = Preferences().getPreferredUnitsOrigin(str(T.getName()))
			if units == CelsiusUnits.get_celsius_sign():
				up = None
			m.append(CelsiusUnits.get_units_row(up is None and units is not None))
		##### CELSIUS TEMPERATURE WORKAROUND
		for u in self.units:
			if T is None or u.getDimensions()==d:
				if up is None:
					selected = False
				else:
					selected = (u == up)
				weight = Pango.Weight.NORMAL
				if selected:
					weight = Pango.Weight.BOLD
				du = u.getDimensions().getDefaultUnits().getName()
				if str(du) == "1":
					du = ""
				m.append([selected,str(u.getName()),"%g %s" %(u.getConversion(),du),weight])

		self.unitsview.set_model(m)

	def on_typecombo_changed(self,widget,*args):
		s = widget.get_active_text()
		#self.browser.reporter.reportNote("value changed to '%s'" % s)
		try:
			T = self.browser.library.findType(s)
			dt = str(T.getDimensions())
			self.dimensionlabel.set_text(dt)
		except:
			T = None
			self.dimensionlabel.set_text("")
		self.update_unitsview(T)
		
	def run(self):
		_res = Gtk.ResponseType.APPLY
		while _res == Gtk.ResponseType.APPLY:
			_res = self.window.run()
			if _res == Gtk.ResponseType.APPLY or _res == Gtk.ResponseType.CLOSE:
				if _res == Gtk.ResponseType.CLOSE and not len(self.realtypes):
					break
				for k,v in self.changed.items():
					self.browser.prefs.setPreferredUnits(k,v)
				self.changed = {}
				typename = self.typecombo.get_active_text()
				if typename:
					self.update_unitsview(self.browser.library.findType(typename))
				else:
					self.update_unitsview(None)
				self.browser.modelview.refreshtree()
				for _obs in self.browser.observers:
					if _obs.alive:
						_obs.units_refresh(self.T)
		self.window.hide()
