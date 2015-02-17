import gtk
import pango
import ascpy

from varentry import *
from studyreporter import *
from math import log, exp

class StudyWin:
	def __init__(self, browser, instance):
		
		self.instance = instance
		self.browser = browser
		self.browser.builder.add_objects_from_file(self.browser.glade_file,["list_of_dist","studywin"])
		
		widgets = ["lowerb","upperb","nsteps","methodrun","dist","check_dist","studywin","var_to_study"]
		for n in widgets:
			setattr(self,n,self.browser.builder.get_object(n))
		
		self.checkbutton = self.browser.builder.get_object("on_fail_continue")
		self.method = None
		
		_p = self.browser.prefs
		_continue_on_fail = _p.getBoolPref("StudyReporter", "continue_on_fail", True)
		self.checkbutton.set_active(_continue_on_fail)
		#--------------------
		# set up the distributions combobox
		
		_cell = gtk.CellRendererText()
		self.dist.pack_start(_cell, True)
		self.dist.add_attribute(_cell, 'text', 0)
		
		#--------------------
		# set up the methods combobox

		_methodstore = self.browser.methodstore
		_methodrenderer = gtk.CellRendererText()
		self.methodrun.set_model(_methodstore)
		self.methodrun.pack_start(_methodrenderer, True)
		self.methodrun.add_attribute(_methodrenderer, 'text',0)
		
		self.browser.builder.connect_signals(self)
		self.fill_values()
		self.lowerb.select_region(0, -1)
		
		
	def fill_values(self):
	  
		self.var_to_study.set_text( self.browser.sim.getInstanceName(self.instance) )
		_nsteps = self.browser.prefs.getStringPref("Study","nsteps","10")
		self.nsteps.set_text(_nsteps)
		_u = self.instance.getType().getPreferredUnits();
		if _u is None:
			_conversion = 1
			_u = self.instance.getDimensions().getDefaultUnits().getName().toString()
		else:
			_conversion = _u.getConversion() # displayvalue x conversion = SI
			_u = _u.getName().toString()

		_arr = {
			self.lowerb: self.instance.getRealValue()
			,self.upperb: self.instance.getUpperBound()
		}
		for _k,_v in _arr.iteritems():
			_t = str(_v / _conversion)+" "+_u
			_k.set_text(_t)
		
	def ready(self):
	# To check if all the input is ok
		if self.on_dist_edited() == 1 and self.on_nsteps_edited() == 1:
			return True
		else:
			return False
		
	def run(self):
		_continue = True
		while _continue:
			_res = self.studywin.run();
			if _res == gtk.RESPONSE_OK:
				if self.ready():
					self.on_ok_clicked()
					_continue = False
				else:
					self.browser.reporter.reportError("Cannot continue. Please review the errors.")
					continue
			elif _res==gtk.RESPONSE_CANCEL:
				_continue = False
		self.studywin.destroy()
		
	def on_studywin_close(self,*args):
		self.studywin.response(gtk.RESPONSE_CANCEL)

	def on_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		if keyname=="Return":
			self.studywin.response(gtk.RESPONSE_OK)
			return True
		elif keyname=="Escape":
			self.studywin.response(gtk.RESPONSE_CANCEL)
			return True;
		return False;
		
	def on_methodrun_changed(self, *args):
		_sel = self.methodrun.get_active_text()
		if _sel:
			_methods = self.browser.sim.getType().getMethods()
			for _m in _methods:
				if _m.getName()==_sel:
					self.method = _m
					break
		
	def taint_entry(self, entry, color):
		entry.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_bg(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))
		if color == "#FFBBBB":
			entry.set_property("secondary-icon-stock", 'gtk-dialog-error')
		elif color == "white":
			entry.set_property("secondary-icon-stock", 'gtk-yes')
			entry.set_property("secondary-icon-tooltip-text", "")

	def parse_entry(self, entry):
		# A simple function to get the real value from the entered text
		# and taint the entry box accordingly
		i = RealAtomEntry(self.instance, entry.get_text())
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
	
	def on_dist_edited(self, *args):
	# To update the check_dist image and do a lot of input checking
		_dist = self.dist.get_active_text()
		_start = self.parse_entry(self.lowerb)
		_end = self.parse_entry(self.upperb)
		
		if _start == None or _end == None:
			self.check_dist.clear()
			return 0
		if _start == _end:
			self.check_dist.set_from_stock('gtk-dialog-error', gtk.ICON_SIZE_BUTTON)
			self.check_dist.set_tooltip_text("The bounds should not be equal.")
			self.taint_entry(self.lowerb, "#FFBBBB")
			self.taint_entry(self.upperb, "#FFBBBB")
			self.lowerb.set_property("secondary-icon-tooltip-text", "The bounds should not be equal.")
			self.upperb.set_property("secondary-icon-tooltip-text", "The bounds should not be equal.")
			return 0
		if _dist == "Linear":
			self.check_dist.set_from_stock('gtk-yes', gtk.ICON_SIZE_BUTTON)
			self.check_dist.set_tooltip_text("")
			return 1
		if _dist == "Logarithmic":
			if _start == 0 or _end == 0:
				self.check_dist.set_from_stock('gtk-dialog-error', gtk.ICON_SIZE_BUTTON)
				self.check_dist.set_tooltip_text("Neither of the bounds can be 0 for logarithmic distribution.")
				if _start == 0:
					self.taint_entry(self.lowerb, "#FFBBBB")
					self.lowerb.set_property("secondary-icon-tooltip-text", "Cannot be 0 for logarithmic distribution")
				else:
					self.taint_entry(self.upperb, "#FFBBBB")
					self.upperb.set_property("secondary-icon-tooltip-text", "Cannot be 0 for logarithmic distribution")
				return 0
			if (_start/_end) < 0:
				self.check_dist.set_from_stock('gtk-dialog-error', gtk.ICON_SIZE_BUTTON)
				self.check_dist.set_tooltip_text("The bounds cannot be of opposite sign in case of logarithmic distribution.")
				self.taint_entry(self.lowerb, "#FFBBBB")
				self.taint_entry(self.upperb, "#FFBBBB")
				self.lowerb.set_property("secondary-icon-tooltip-text", "The bounds cannot be of opposite sign in case of logarithmic distribution.")
				self.upperb.set_property("secondary-icon-tooltip-text", "The bounds cannot be of opposite sign in case of logarithmic distribution.")
				return 0
			self.check_dist.set_from_stock('gtk-yes', gtk.ICON_SIZE_BUTTON)
			self.check_dist.set_tooltip_text("")
			return 1
		
	def on_nsteps_edited(self, *args):
	# To update the icon in the entry
		_failed = False
		
		try:
			_nsteps = int(self.nsteps.get_text())
		except:
			_failed = True
			
		if _failed or _nsteps == 0:
			self.nsteps.set_text("")
			self.taint_entry(self.nsteps,"#FFBBBB")
			self.nsteps.set_property("secondary-icon-tooltip-text", "Please give a valid entry")
			return 0
		else:
			self.taint_entry(self.nsteps,"white")
			return 1
			
	def on_check_toggled(self, *args):
	# To update the preference for behaviour on solver fail
	
		_p = self.browser.prefs
		_p.setBoolPref("StudyReporter", "continue_on_fail", self.checkbutton.get_active())
		
	def on_ok_clicked(self, *args):
	# check that the units of the entered values are acceptable
		
		_arr = [self.lowerb, self.upperb]
		_failed = False
		_parameters = []
		for _k in _arr:
			i = RealAtomEntry(self.instance, _k.get_text())
			try:
				i.checkEntry()
				_parameters.append(i.getValue())
			except InputError, e:
				print "INPUT ERROR: ",str(e)
				_failed = True
				
		if _failed:
			raise InputError(None) # no message
		
		else:
			self.solve(_parameters)
			
	def solve(self, parameters):
		_nsteps = int(self.nsteps.get_text())
		_dist = self.dist.get_active_text()
		_browser = self.browser
		
		if _dist == "Linear":
			_step = (parameters[1] - parameters[0])/_nsteps
			_log = False
		else:
			_diff = log(parameters[1]/parameters[0])/_nsteps
			_log = True
			
		if not hasattr(self.browser,'solver'):
			_browser.reporter.reportError("No solver assigned!")
			return
		
		if _browser.no_built_system():
			return
		_browser.start_waiting("Solving with %s..." % _browser.solver.getName())
		_browser.prefs.setStringPref("Study","nsteps",str(_nsteps))
		self.studywin.destroy()
		reporter = StudyReporter(_browser, _browser.sim.getNumVars(), self.instance, _nsteps, self)
		i = 0
		while i<=_nsteps and reporter.guiinterrupt == False:
			
			#run method
			if self.method:
				try:
					_browser.sim.run(method)
				except RuntimeError,e:
					_browser.reporter.reportError(str(e))
				
			#set the value
			## FIXME do this test outside the loop...
			if self.instance.getType().isRefinedSolverVar():
				# for solver vars, set the 'fixed' flag as well
				## FIXME shouldn't be necessary to set the 'fixed' flag each time.
				## FIXME this function seems to somehow be repeatedly parsing units: avoid doing that every step.
				self.instance.setFixedValue(parameters[0])
			else:
				## why would we NOT want to fix this variable??
				self.instance.setRealValue(parameters[0])
			
			#solve
			try:
				reporter.updateVarDetails(i)
				_browser.sim.solve(_browser.solver, reporter)
			except RuntimeError,e:
				_browser.reporter.reportError(str(e))

			i = i+1
			# any issue with accumulation of rounding errors here?
			if _log == True:
				parameters[0] = parameters[0]*exp(_diff)
			else:
				parameters[0] = parameters[0] + _step
		if reporter.continue_on_fail == True:
			reporter.updateVarDetails(i)
		
		_browser.stop_waiting()
		_browser.modelview.refreshtree()
		
