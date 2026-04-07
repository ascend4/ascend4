from gi.repository import Gtk
from gi.repository import Pango

import ascpy
import time
from celsiusunits import CelsiusUnits
from preferences import Preferences


class UnitsDialog:

	def __init__(self, browser, T=None, instance=None):
		"""Create units browser for a selected real type/instance."""
		self.browser = browser
		self.T = T
		self.instance = instance
		self.selected_units = None
		self.current_units_name = None

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
			self.browser.reporter.reportError(
				"No dimensioned atom types available yet (have you loaded a model yet?)"
			)
			return

		if len(self.unitsview.get_columns()) == 0:
			_renderer0 = Gtk.CellRendererToggle()
			_renderer0.set_radio(True)
			_renderer0.connect("toggled", self.unitsview_row_toggled)
			_col0 = Gtk.TreeViewColumn("", _renderer0, active=0)
			self.unitsview.append_column(_col0)

			_renderer1 = Gtk.CellRendererText()
			_col1 = Gtk.TreeViewColumn("Units", _renderer1, text=1, weight=3)
			self.unitsview.append_column(_col1)

			_renderer2 = Gtk.CellRendererText()
			_col2 = Gtk.TreeViewColumn("Conversion", _renderer2, text=2)
			self.unitsview.append_column(_col2)

		self._create_policy_widgets()

		model = Gtk.ListStore(str)
		self.typecombo.set_model(model)
		if T is not None and T.isRefinedReal():
			self.typecombo.append_text(str(T.getName()))
		if self.typecombo.get_active() < 0:
			self.typecombo.set_active(0)
		self.on_typecombo_changed(self.typecombo)

	def _create_policy_widgets(self):
		table = self.browser.builder.get_object("table8")
		table.resize(5, 2)
		for child in table.get_children():
			if isinstance(child, Gtk.CheckButton):
				table.remove(child)

		self.name_override_check = Gtk.CheckButton.new_with_mnemonic(
			"Apply as _variable override"
		)
		self.scope_model_check = Gtk.CheckButton.new_with_mnemonic(
			"Limit to current _model scope"
		)
		self.name_override_check.set_active(self.browser.get_units_edit_override_by_name())
		self.scope_model_check.set_active(self.browser.get_units_edit_scope_model())

		if self.instance is None:
			self.name_override_check.set_active(False)
			self.name_override_check.set_sensitive(False)

		self.name_override_check.connect("toggled", self.on_name_override_toggled)

		xfill = Gtk.AttachOptions.FILL
		yfill = Gtk.AttachOptions.FILL
		table.attach(self.name_override_check, 1, 2, 3, 4, xfill, yfill, 0, 0)
		table.attach(self.scope_model_check, 1, 2, 4, 5, xfill, yfill, 0, 0)

		self.name_override_check.show()
		self.scope_model_check.show()
		self._sync_policy_widgets()

	def _sync_policy_widgets(self):
		if self.name_override_check.get_active():
			self.scope_model_check.set_active(True)
			self.scope_model_check.set_sensitive(False)
		else:
			self.scope_model_check.set_sensitive(True)

	def on_name_override_toggled(self, widget, *args):
		self._sync_policy_widgets()
		self.update_applybutton()

	def unitsview_row_toggled(self, widget, path, *args):
		model = self.unitsview.get_model()
		i = model.get_iter_from_string(path)
		n = model.get_value(i, 1)
		j = model.get_iter_first()
		while j is not None:
			model.set_value(j, 0, False)
			j = model.iter_next(j)
		model.set_value(i, 0, True)
		self.selected_units = n
		self.update_applybutton()

	def update_applybutton(self):
		can_apply = self.selected_units is not None and self.selected_units != self.current_units_name
		self.applybutton.set_sensitive(can_apply)

	def update_unitsview(self, T):
		m = Gtk.ListStore(bool, str, str, int)
		if T is None:
			self.unitsview.set_model(m)
			self.current_units_name = None
			self.selected_units = None
			self.update_applybutton()
			return

		d = T.getDimensions()
		if self.instance is not None:
			up = self.browser.get_instance_display_units(self.instance, autoscale=False)
		else:
			up = T.getDeclaredUnits()
			if up is None:
				up = T.getDimensions().getDefaultUnits()
		self.current_units_name = str(up.getName())

		##### CELSIUS TEMPERATURE WORKAROUND
		if str(d) == "TMP":
			units = CelsiusUnits.normalize_units_name(Preferences().getPreferredUnitsOrigin(str(T.getName())))
			if units in CelsiusUnits.get_temperature_units():
				self.current_units_name = CelsiusUnits.get_display_symbol(units)
			for tunits in CelsiusUnits.get_temperature_units():
				label = CelsiusUnits.get_display_symbol(tunits)
				m.append(CelsiusUnits.get_units_row(self.current_units_name == label, tunits))
		##### CELSIUS TEMPERATURE WORKAROUND

		for u in self.units:
			if u.getDimensions() == d:
				uname = str(u.getName())
				selected = uname == self.current_units_name
				weight = Pango.Weight.BOLD if selected else Pango.Weight.NORMAL
				du = u.getDimensions().getDefaultUnits().getName()
				if str(du) == "1":
					du = ""
				m.append([selected, uname, "%g %s" % (u.getConversion(), du), weight])
				if selected:
					self.selected_units = uname

		self.unitsview.set_model(m)
		self.update_applybutton()

	def on_typecombo_changed(self, widget, *args):
		s = widget.get_active_text()
		try:
			T = self.browser.library.findType(s)
			self.dimensionlabel.set_text(str(T.getDimensions()))
		except Exception:
			T = None
			self.dimensionlabel.set_text("")
		self.update_unitsview(T)

	def _apply_selection(self):
		if self.instance is None or self.selected_units is None:
			return
		T = self.instance.getType()

		##### CELSIUS TEMPERATURE WORKAROUND
		if str(T.getDimensions()) == "TMP":
			selected_temp_units = CelsiusUnits.normalize_units_name(self.selected_units)
			if selected_temp_units in CelsiusUnits.get_temperature_units():
				self.browser.prefs.setPreferredUnits(str(T.getName()), selected_temp_units)
				self.current_units_name = CelsiusUnits.get_display_symbol(selected_temp_units)
				return
			self.browser.prefs.setPreferredUnits(str(T.getName()), "")
		##### CELSIUS TEMPERATURE WORKAROUND

		by_name = self.name_override_check.get_active()
		model_scope = self.scope_model_check.get_active()
		self.instance.setDisplayUnitsOverride(self.selected_units, by_name, model_scope)
		rc = ascpy.saveDisplayUnitsOverrides()
		if rc != 0:
			raise RuntimeError("Failed to save units-overrides preferences")
		self.current_units_name = self.selected_units

	def run(self):
		_res = Gtk.ResponseType.APPLY
		while _res == Gtk.ResponseType.APPLY:
			_res = self.window.run()
			if _res == Gtk.ResponseType.APPLY or _res == Gtk.ResponseType.CLOSE:
				if _res == Gtk.ResponseType.CLOSE and not len(self.realtypes):
					break
				t0 = time.perf_counter()
				try:
					self._apply_selection()
				except Exception as e:
					self.browser.reporter.reportError(str(e))
					continue
				t1 = time.perf_counter()
				by_name = self.name_override_check.get_active()
				if by_name:
					self.browser.modelview.refresh_display_units(instance=self.instance)
				else:
					self.browser.modelview.refresh_display_units(instance_type=self.T)
				t2 = time.perf_counter()
				for _obs in self.browser.observers:
					if _obs.alive:
						_obs.units_refresh(self.T)
				t3 = time.perf_counter()
				self.update_unitsview(self.T)
				t4 = time.perf_counter()
				elapsed = t4 - t0
				if elapsed > 0.2:
					self.browser.reporter.reportNote(
						"Units Apply timing: set %.3fs, tree %.3fs, observers %.3fs, dialog %.3fs, total %.3fs"
						% (t1 - t0, t2 - t1, t3 - t2, t4 - t3, elapsed)
					)
		self.window.hide()
