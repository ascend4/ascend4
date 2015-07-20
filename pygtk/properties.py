# GUI for ASCEND solver_var properties

from gi.repository import Gtk, Gdk
import ascpy
from celsiusunits import CelsiusUnits
from preferences import Preferences
from varentry import *
from infodialog import *

class RelPropsWin:
	def __init__(self,browser,instance):
		self.instance = instance
		self.browser = browser

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["relpropswin"])
		self.window = self.browser.builder.get_object("relpropswin")
		self.window.set_transient_for(self.browser.window)

		self.relname = self.browser.builder.get_object("relname")
		self.residual = self.browser.builder.get_object("residual")
		self.expr = self.browser.builder.get_object("expr")
		self.included = self.browser.builder.get_object("included")
		self.active = self.browser.builder.get_object("active")
		self.exprbuff = Gtk.TextBuffer()
		self.expr.set_buffer(self.exprbuff)
		self.morepropsbutton = self.browser.builder.get_object("morepropsbutton1")

		self.statusimg = self.browser.builder.get_object("rel_statusimg")
		self.statusmessage = self.browser.builder.get_object("rel_statusmessage")

		self.fill_values()
		self.browser.builder.connect_signals(self)

	def fill_values(self):
		self.window.set_title(self.instance.getKindStr())
		self.relname.set_text(self.browser.sim.getInstanceName(self.instance))
		_status = self.instance.getStatus()
		self.statusimg.set_from_pixbuf(self.browser.statusicons[_status])
		self.statusmessage.set_text(self.browser.statusmessages[_status])
		if self.instance.isRelation():
			self.exprbuff.set_text(self.instance.getRelationAsString(self.browser.sim.getModel()))
		elif self.instance.isLogicalRelation():
			self.exprbuff.set_text(self.instance.getLogrelAsString(self.browser.sim.getModel()))
		elif self.instance.isWhen():
			self.exprbuff.set_text(self.instance.getWhenAsString(self.browser.sim.getModel()))

		if str(self.instance.getType()) == "relation":
			self.residual.set_text(str(self.instance.getResidual()))
			self.included.set_active(self.instance.isIncluded())
		else:
			self.morepropsbutton.set_sensitive(False)
			self.residual.set_sensitive(False)
			self.included.set_sensitive(False)

	def on_relpropswin_close(self,*args):
		self.window.response(Gtk.ResponseType.CANCEL)

	def on_entry_key_press_event(self,widget,event):
		keyname = Gdk.keyval_name(event.keyval)
		if keyname=="Escape":
			self.window.response(Gtk.ResponseType.CLOSE)
			return True;
		return False;

	def run(self):
		self.window.run()
		self.window.hide()

	def on_morepropsbutton_clicked(self,*args):
		title = "All properties of '%s'" % self.browser.sim.getInstanceName(self.instance)
		text = title + "\n\n"
		c = self.instance.getChildren()
		if c:
			for i in c:
				text += "%s = %s\n" % (self.browser.sim.getInstanceName(i), i.getValue())
		else:
				text += "This relation has no 'child' properties"
		_dialog = InfoDialog(self.browser,self.window,text,title)
		_dialog.run()

	def on_included_toggled(self,widget,*args):
		self.instance.setIncluded(widget.get_active())
		self.browser.do_solve_if_auto()

class VarPropsWin:
	def __init__(self,browser,instance):
		self.instance = instance;
		self.browser = browser;

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["varpropswin"])
		self.window = self.browser.builder.get_object("varpropswin")
		self.window.set_transient_for(self.browser.window)

		self.varname = self.browser.builder.get_object("varname")
		self.valueentry= self.browser.builder.get_object("valueentry");
		self.lowerentry = self.browser.builder.get_object("lowerentry");
		self.upperentry = self.browser.builder.get_object("upperentry");
		self.nominalentry = self.browser.builder.get_object("nominalentry");
		self.fixed = self.browser.builder.get_object("fixed");
		self.free = self.browser.builder.get_object("free");

		self.statusimg = self.browser.builder.get_object("var_statusimg");
		assert self.statusimg
		self.statusmessage = self.browser.builder.get_object("var_statusmessage");

		self.cliquebutton = self.browser.builder.get_object("cliquebutton"); 
		self.morepropsbutton = self.browser.builder.get_object("morepropsbutton");

		self.convert = False
		self.fill_values()

		self.browser.builder.connect_signals(self)

	def fill_values(self):
		# all the values here use the same preferred units for this instance type
		_u = self.instance.getType().getPreferredUnits();
		if _u is None:
			_conversion = 1
			_u = self.instance.getDimensions().getDefaultUnits().getName().toString()
		else:
			_conversion = _u.getConversion() # displayvalue x conversion = SI
			_u = _u.getName().toString()

		_arr = {
			self.valueentry: self.instance.getRealValue()
			,self.lowerentry: self.instance.getLowerBound()
			,self.upperentry: self.instance.getUpperBound()
			,self.nominalentry: self.instance.getNominal()
		}

		##### CELSIUS TEMPERATURE WORKAROUND
		if self.instance.getType().isRefinedReal() and str(self.instance.getType().getDimensions()) == 'TMP':
			units = Preferences().getPreferredUnitsOrigin(str(self.instance.getType().getName()))
			if units == CelsiusUnits.get_celsius_sign():
				self.convert = True
		##### CELSIUS TEMPERATURE WORKAROUND

		for _k,_v in _arr.iteritems():	
			_t = str(_v / _conversion)+" "+_u
			##### CELSIUS TEMPERATURE WORKAROUND
			if self.convert:
				_t = CelsiusUnits.convert_kelvin_to_celsius(_v, str(self.instance.getType())) + " " + CelsiusUnits.get_celsius_sign()
			##### CELSIUS TEMPERATURE WORKAROUND
			_k.set_text(_t)
			self.parse_entry(_k)

		self.varname.set_text(self.browser.sim.getInstanceName(self.instance));

		if self.instance.isFixed():
			self.fixed.set_active(True);
		else:
			self.free.set_active(True);

		_status = self.instance.getStatus()
		
		self.statusimg.set_from_pixbuf(self.browser.statusicons[_status]);
		self.statusmessage.set_text(self.browser.statusmessages[_status]);

	def apply_changes(self):
		print "APPLY"
		# check the units of the entered values are acceptable
		
		_arr = {
			self.valueentry: self.instance.setRealValue
			,self.lowerentry: self.instance.setLowerBound
			,self.upperentry: self.instance.setUpperBound
			,self.nominalentry: self.instance.setNominal
		}
		failed = False;
		for _k,_v in _arr.iteritems():
			newtext = _k.get_text()
			##### CELSIUS TEMPERATURE WORKAROUND
			if self.convert:
				if len(newtext.split(" ")) == 1 or newtext.split(" ")[1] == CelsiusUnits.get_celsius_sign():
					newtext = CelsiusUnits.convert_celsius_to_kelvin(newtext.split(" ")[0], str(self.instance.getType()))
			##### CELSIUS TEMPERATURE WORKAROUND

			i = RealAtomEntry(self.instance, newtext)
			try:
				i.checkEntry()
				self.taint_entry(_k,"white");
				_v(i.getValue())
			except InputError, e:
				print "INPUT ERROR: ",str(e)
				self.taint_entry(_k,"#FFBBBB");
				failed = True;
		
		self.instance.setFixed(self.fixed.get_active())

		if failed:
			raise InputError(None) # no message

		self.browser.do_solve_if_auto()

	def taint_entry(self, entry, color):
		entry.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse(color))
		entry.modify_bg(Gtk.StateType.ACTIVE, Gdk.color_parse(color))
		entry.modify_base(Gtk.StateType.NORMAL, Gdk.color_parse(color))
		entry.modify_base(Gtk.StateType.ACTIVE, Gdk.color_parse(color))
		if color == "#FFBBBB":
			entry.set_property("secondary-icon-stock", 'gtk-dialog-error')
		elif color == "white":
			entry.set_property("secondary-icon-stock", 'gtk-yes')
			entry.set_property("secondary-icon-tooltip-text", "")

	def parse_entry(self, entry):
		# A simple function to get the real value from the entered text
		# and taint the entry box accordingly
		newtext = entry.get_text()
		##### CELSIUS TEMPERATURE WORKAROUND
		if self.convert:
			if len(newtext) > 0 and (len(newtext.split(" ")) == 1 or newtext.split(" ")[1] == CelsiusUnits.get_celsius_sign()):
				newtext = CelsiusUnits.convert_celsius_to_kelvin(newtext.split(" ")[0], str(self.instance.getType()))
		##### CELSIUS TEMPERATURE WORKAROUND
		i = RealAtomEntry(self.instance, newtext)
		try:
			i.checkEntry()
			_value = i.getValue()
		except InputError, e:
			_value = None
			_error = re.split('Input Error: ', str(e), 1)
			entry.set_property("secondary-icon-tooltip-text", _error[1])
		
		if _value is not None:
			self.taint_entry(entry, "white")
		else:
			self.taint_entry(entry, "#FFBBBB")
		return _value
		
	def on_varpropswin_close(self,*args):
		self.window.response(Gtk.ResponseType.CANCEL)

	def on_entry_key_press_event(self,widget,event):
		keyname = Gdk.keyval_name(event.keyval)
		if keyname=="Return":
			self.window.response(Gtk.ResponseType.OK)
			return True
		elif keyname=="Escape":
			self.window.response(Gtk.ResponseType.CANCEL)
			return True;
		return False;

	def on_cliquebutton_clicked(self,*args):
		title = "Clique of '%s'"%self.browser.sim.getInstanceName(self.instance)
		text = title + "\n\n"
		s = self.instance.getClique();
		if s:
			for i in s:
				text += "%s\n"%self.browser.sim.getInstanceName(i)
		else:
			text += "CLIQUE IS EMPTY"
		_dialog = InfoDialog(self.browser,self.window,text,title)
		_dialog.run()

	def on_aliasesbutton_clicked(self,*args):
		title = "Aliases of '%s'"%self.browser.sim.getInstanceName(self.instance)
		text = title + "\n\n"
		s = self.instance.getAliases();
		if s:
			for i in sorted(s):
				text += "%s\n" % i
		else:
			text += "NO ALIASES"
		_dialog = InfoDialog(self.browser,self.window,text,title)
		_dialog.run()

	def on_morepropsbutton_clicked(self,*args):
		title = "All properties of '%s'" % self.browser.sim.getInstanceName(self.instance)
		text = title + "\n\n"
		c = self.instance.getChildren()
		if c:
			for i in c:
				text += "%s = %s\n" % (self.browser.sim.getInstanceName(i), i.getValue())
		else:
				text += "This variable has no 'child' properties"
		_dialog = InfoDialog(self.browser,self.window,text,title)
		_dialog.run()

	def run(self):
		_continue = True
		while _continue:
			_res = self.window.run();
			try:
				if _res == Gtk.ResponseType.APPLY or _res == Gtk.ResponseType.OK:
					self.apply_changes();
			except InputError:
				# if input error, assume that the gui has been updated appropriately
				continue;

			if _res == Gtk.ResponseType.OK or _res==Gtk.ResponseType.CANCEL:
				_continue = False;
		
		self.window.destroy();
