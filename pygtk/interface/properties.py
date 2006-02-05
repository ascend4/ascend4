# GUI for ASCEND solver_var properties

import gtk, gtk.glade
import ascend
from varentry import *

class RelPropsWin:
	def __init__(self,GLADE_FILE,browser,instance):
		self.instance = instance;
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(GLADE_FILE,"relpropswin")
		self.window = _xml.get_widget("relpropswin")

		self.relname = _xml.get_widget("relname")
		self.residual = _xml.get_widget("residual")
		self.expr = _xml.get_widget("expr")
		self.exprbuff = gtk.TextBuffer();
		self.expr.set_buffer(self.exprbuff)

		self.fill_values()
		_xml.signal_autoconnect(self)

	def fill_values(self):
		self.relname.set_text( self.browser.sim.getInstanceName(self.instance) )
		self.residual.set_text( str( self.instance.getResidual() ) )
		self.exprbuff.set_text( self.instance.getRelationAsString(self.browser.sim.getModel() ) )

	def on_varpropswin_close(self,*args):
		self.window.response(gtk.RESPONSE_CANCEL)

	def on_entry_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		if keyname=="Escape":
			self.window.response(gtk.RESPONSE_CLOSE)
			return True;
		return False;

	def run(self):
		self.window.run()
		self.window.hide()

class VarPropsWin:
	def __init__(self,GLADE_FILE,browser,instance):
		self.instance = instance;
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(GLADE_FILE,"varpropswin")
		self.varpropswin = _xml.get_widget("varpropswin")

		self.varname = _xml.get_widget("varname")
		self.valueentry= _xml.get_widget("valueentry");
		self.lowerentry = _xml.get_widget("lowerentry");
		self.upperentry = _xml.get_widget("upperentry");
		self.nominalentry = _xml.get_widget("nominalentry");
		self.fixed = _xml.get_widget("fixed");
		self.free = _xml.get_widget("free");

		self.statusimg = _xml.get_widget("statusimg");
		self.statusmessage = _xml.get_widget("statusmessage");

		self.othernames = _xml.get_widget("othernames"); 

		self.fill_values()

		_xml.signal_autoconnect(self)

	def fill_values(self):
		# all the values here use the same preferred units for this instance type

		_u = self.instance.getType().getPreferredUnits();
		if _u == None:
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
		for _k,_v in _arr.iteritems():	
			_t = str(_v / _conversion)+" "+_u
			_k.set_text(_t)

		self.varname.set_text(self.browser.sim.getInstanceName(self.instance));

		if self.instance.isFixed():
			self.fixed.set_active(True);
		else:
			self.free.set_active(True);

		_status = self.instance.getVarStatus()
		
		self.statusimg.set_from_pixbuf(self.browser.statusicons[_status]);
		self.statusmessage.set_text(self.browser.statusmessages[_status]);

		#self.clique = self.instance.getClique()
		#print "CLIQUE:",self.clique
		#
		#if len(self.clique) > 1:
		#	self.othernames.set_label("%d other names..." % len(self.clique));
		#	self.othernames.set_sensitive(True)
		#else:
		#	self.othernames.set_label("No other names");
		#	self.othernames.set_sensitive(False)
		#
		self.othernames.set_label("Clique not implemented");
		self.othernames.set_sensitive(False)

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
				self.color_entry(_k,"white");
				_v(i.getValue())
			except InputError, e:
				print "INPUT ERROR: ",str(e)
				self.color_entry(_k,"#FFBBBB");
				failed = True;
		
		self.instance.setFixed(self.fixed.get_active())

		if failed:
			raise InputError(None) # no message

		self.browser.do_solve_if_auto()

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

	def on_othernames_clicked(self,*args):
		print "OTHER NAMES..."
		s = self.instance.getClique();
		for i in s:
			print self.browser.sim.getInstanceName(i)

	def run(self):
		self.valueentry.grab_focus()
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



