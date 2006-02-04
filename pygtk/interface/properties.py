# GUI for ASCEND solver_var properties

import gtk, gtk.glade
import ascend
from varentry import *

class VarPropsWin:
	def __init__(self,GLADE_FILE,browser,instance):
		self.instance = instance;
		self.browser = browser;

		# values being edited:
		self.lower = 0
		self.upper = 1
		self.nominal = 0.5
		self.fixed = True
		self.status = ascend.ASCXX_VAR_STATUS_UNKNOWN

		# GUI config
		_xml = gtk.glade.XML(GLADE_FILE,"varpropswin")
		self.varpropswin = _xml.get_widget("varpropswin")

		self.varname = _xml.get_widget("varname")
		self.valueentry= _xml.get_widget("valueentry");
		self.lowerentry = _xml.get_widget("lowerentry");
		self.upperentry = _xml.get_widget("upperentry");
		self.nominalentry = _xml.get_widget("nominalentry");
			
		self.statusimg = _xml.get_widget("statusimg"); self.statusimg = None

		self.othernamesbutton = _xml.get_widget("othernamesbutton"); self.othernamesbutton.set_label("100 other names...")

		self.fill_values()

		_xml.signal_autoconnect(self)

	def fill_values(self):
		# all the values here use the same preferred units for this instance type

		_u = self.instance.getType().getPreferredUnits();
		if _u == None:
			_conversion = 1
			_u = self.getDimensions().getDefaultUnits().getName().toString()
		else:
			_conversion = _u.getConversion() # displayvalue x conversion = SI
			_u = _u.getName().toString()

		_arr = {
			self.valueentry: self.instance.getRealValue()
			,self.lowerentry: self.instance.getLowerBound()
			,self.upperentry: self.instance.getUpperBound()
			,self.nominalentry: self.instance.getNominal()
		}
		for _k,_v in _arr.iteritems():	
			_t = str(_v / _conversion)+" "+_u
			_k.set_text(_t)

		self.varname.set_text(self.browser.sim.getInstanceName(self.instance));

		if self.instance.isFixed():
			pass

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
			i = RealAtomEntry(self.instance, _k.get_text())
			try:
				i.checkEntry()
				_v(i.getValue())
			except InputError, e:
				print "INPUT ERROR: ",str(e)
				self.color_entry(_k,"#FFBBBB");
				failed = True;
		
		if failed:
			raise InputError("Invalid inputs are highlighted in the GUI")

		self.browser.refreshtree()

	def color_entry(self,entry,color):
		entry.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_bg(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))

	def on_varpropswin_close(self,*args):
		self.varpropswin.response(gtk.RESPONSE_CANCEL)

	def on_entry_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		if keyname=="Return":
			self.varpropswin.response(gtk.RESPONSE_OK)
			return True
		elif keyname=="Escape":
			self.varpropswin.response(gtk.RESPONSE_CANCEL)
			return True;
		return False;

	def run(self):
		_continue = True;
		while _continue:
			_res = self.varpropswin.run();
			try:
				if _res == gtk.RESPONSE_APPLY or _res == gtk.RESPONSE_OK:
					self.apply_changes();
			except InputError:
				# if input error, assume that the gui has been updated appropriately
				continue;

			if _res == gtk.RESPONSE_OK or _res==gtk.RESPONSE_CANCEL:
				_continue = False;
		
		self.varpropswin.destroy();



