import gtk
import pango
import ascpy

from varentry import *
from studyreporter import *
from math import log, exp

STEP_NUMBER = 0
STEP_INCREM = 1
STEP_RATIO = 2
DIST_LINEAR = "Linear"
DIST_LOG = "Logarithmic"

class StudyWin:
	def __init__(self, browser, instance):
		"""
		Study dialog: allow user to fill an Observer by varying the value in 
		a particular column, and solving for each case (also there is the option
		of running a selected METHOD before each step)
		"""
		# we will be using the instance to determine valid units for bounds and step size
		self.instance = instance
		self.browser = browser
		self.browser.builder.add_objects_from_file(self.browser.glade_file,["list_of_dist","studywin"])
		
		widgets = ["lowerb","upperb","step_menu","nsteps","methodrun","dist","check_dist","studywin","var_to_study"]
		for n in widgets:
			setattr(self,n,self.browser.builder.get_object(n))
		
		self.checkbutton = self.browser.builder.get_object("on_fail_continue")
		self.method = None
	
		# TODO add an integer index to the ListStore as well, to avoid string conversion
		self.step_menu_model = gtk.ListStore(int,str)
		self.step_menu_model.append([STEP_NUMBER,'No. of steps'])
		self.step_menu_model.append([STEP_INCREM,'Step size'])
		self.step_menu_model.append([STEP_RATIO, 'Step ratio'])
		renderer = gtk.CellRendererText()
		self.step_menu.set_model(self.step_menu_model)
		self.step_menu.pack_start(renderer, True)
		self.step_menu.add_attribute(renderer, 'text',1)
		self.step_menu.set_active(0)
		#self.step_type="No. of steps"

		_p = self.browser.prefs
		_continue_on_fail = _p.getBoolPref("StudyReporter", "continue_on_fail", True)
		self.checkbutton.set_active(_continue_on_fail)

		# set up the distributions combobox		
		_cell = gtk.CellRendererText()
		self.dist.pack_start(_cell, True)
		self.dist.add_attribute(_cell, 'text', 0)
		
		# set up the methods combobox
		_methodstore = self.browser.methodstore
		_methodrenderer = gtk.CellRendererText()
		self.methodrun.set_model(_methodstore)
		self.methodrun.pack_start(_methodrenderer, True)
		self.methodrun.add_attribute(_methodrenderer, 'text',0)

		# user preferences for this dialog
		self.nsteps_number = str(int(float(self.browser.prefs.getStringPref("Study","nsteps","10"))))
		self.nsteps_increm = self.browser.prefs.getStringPref("Study","nsteps_increm","")
		self.nsteps_ratio = self.browser.prefs.getStringPref("Study","nsteps_ratio","1.1")
		self.nsteps_type = int(self.browser.prefs.getStringPref("Study","step_type",str(STEP_NUMBER)))

		# fill in step type and default nsteps
		self.var_to_study.set_text(self.browser.sim.getInstanceName(self.instance))
		self.set_step_type(self.nsteps_type)
		self.nsteps.set_text({
			STEP_NUMBER:self.nsteps_number
			,STEP_INCREM:self.nsteps_increm
			,STEP_RATIO:self.nsteps_ratio
		}[self.nsteps_type])
		# if using an increment by preference, only permit it if dimensionally compatible with
		# selected variable, else default back to number of steps
		if not self.validate_nsteps():
			self.set_step_type(STEP_NUMBER)
			self.nsteps.set_text(self.nsteps_number)
			self.taint_entry(self.nsteps,good=1)

		# fill in upper/.lower bound
		_u = self.instance.getType().getPreferredUnits();
		if _u is None:
			_conversion = 1
			_u = self.instance.getDimensions().getDefaultUnits().getName().toString()
		else:
			_conversion = _u.getConversion() # displayvalue x conversion = SI
			_u = _u.getName().toString()

		_arr = {self.lowerb: self.instance.getRealValue()
			,self.upperb: self.instance.getUpperBound() # this upper bound is probably stoopid
		}
		for _k,_v in _arr.iteritems():
			_t = str(_v / _conversion)+" "+_u
			_k.set_text(_t)
		
		self.browser.builder.connect_signals(self)
		self.lowerb.select_region(0, -1)
	
	def get_step_type(self):
		_s = self.step_menu.get_active_iter()
		return self.step_menu_model.get_value(_s,0)

	def set_step_type(self,st):
		#FIXME this depends on the ordering being equal to the 'id' column values?
		self.step_menu.set_active(st)

	def set_dist(self,dist):
		#FIXME this depends on the ordering, is there a better way?
		self.dist.set_active({DIST_LINEAR:0, DIST_LOG:1}[dist])

	def run(self):
		while 1:
			_res = self.studywin.run();
			if _res == gtk.RESPONSE_OK:
				if self.validate_inputs():
					# store inputs for later recall
					_p = self.browser.prefs
					_p.setStringPref("Study", "nsteps", self.nsteps_number)
					_p.setStringPref("Study", "nsteps_increm", self.nsteps_increm)
					_p.setStringPref("Study", "nsteps_ratio", self.nsteps_ratio)
					_p.setStringPref("Study", "nsteps_type", self.get_step_type())
					# run the study
					self.solve()
					break
				else:
					self.browser.reporter.reportError("Please review input errors in Study dialog.")
					continue
			elif _res==gtk.RESPONSE_CANCEL:
				# cancel... exit Study
				break
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
		
	def on_dist_changed(self, *args):
		_dist = self.dist.get_active_text()
		if _dist == DIST_LINEAR:
			if self.get_step_type() == STEP_RATIO:
				self.set_step_type(STEP_INCREM)
				self.nsteps.set_text(self.nsteps_number)
		elif _dist == DIST_LOG:
			if self.get_step_type() == STEP_INCREM:
				self.set_step_type(STEP_RATIO)
				self.nsteps.set_text(self.nsteps_ratio)
		self.validate_inputs()

	def validate_inputs(self):
		"""
		Check that inputs make sense in terms of log dist constraints & others.
		Returns 1 if all is valid. If all is not valid, relevant inputs are
		tainted for user correction.
		"""
		_dist = self.dist.get_active_text()

		_start = self.parse_entry(self.lowerb)
		_end = self.parse_entry(self.upperb)
		steps = self.validate_nsteps()
		
		if _start is None or _end is None:
			# error/empty start/end values will already have been tainted
			self.taint_dist()
			return 0

		if _start == _end:
			# can't distribute over a zero-width range (and no point)
			_msg = "Bounds cannot not be equal."
			self.taint_dist(msg=_msg)
			self.taint_entry(self.lowerb,msg=_msg)
			self.taint_entry(self.upperb,msg=_msg)
			return 0

		if _dist == DIST_LINEAR:
			flag = 0
			if self.get_step_type() == STEP_RATIO:
				self.set_step_type(STEP_INCREM)
			self.taint_dist(good=1)
			return 1
		if _dist == DIST_LOG:
			flag = 0
			if self.get_step_type() == STEP_INCREM:
				self.set_step_type(STEP_RATIO)
				flag = 1
			self.step_menu.remove_text(1)
			self.step_menu.insert_text(1,"Step ratio")
			if flag == 1:
				self.step_menu.set_active(1)
			if _start == 0 or _end == 0:
				_msg = "Bounds cannot be 0 for logarithmic distribution."
				self.taint_dist(_msg)
				if _start == 0:
					self.taint_entry(self.lowerb,msg=_msg)
				else:
					self.taint_entry(self.upperb,msg=_msg)
				return 0
			if (_start/_end) < 0:
				_msg = "Bounds cannot be of opposite sign for logarithmic distribution."
				self.taint_dist(_msg)
				self.taint_entry(self.lowerb,msg=_msg)
				self.taint_entry(self.upperb,msg=_msg)
				return 0
			self.check_dist.set_from_stock('gtk-yes', gtk.ICON_SIZE_BUTTON)
			self.check_dist.set_tooltip_text("")
			return 1

	def on_step_menu_changed(self, *args):
		"""
		If the step menu is changed, recall previous 'sane' input for nsteps.
		"""	
		_st = self.get_step_type()
		self.nsteps.set_text({
			STEP_NUMBER:self.nsteps_number
			,STEP_INCREM:self.nsteps_increm
			,STEP_RATIO:self.nsteps_ratio
		}[_st])
		_dist = self.dist.get_active_text()
		if _st==STEP_RATIO and _dist==DIST_LINEAR:
			self.set_dist(DIST_LOG)
		elif _st==STEP_INCREM and _dist==DIST_LOG:
			self.set_dist(DIST_LINEAR)
		self.validate_nsteps()

	def validate_nsteps(self):
		_st = self.get_step_type()
		_val = self.nsteps.get_text()
		if _st==STEP_RATIO:
			try:
				_fl = float(_val)
			except:
				_fl = None
			if _fl is None or _fl <= 0:
				self.taint_entry(self.nsteps,"Value must be positive")
				return 0
		elif _st==STEP_INCREM:
			# will also handle the tainting, if required:
			return self.parse_entry(self.nsteps)
		elif _st==STEP_NUMBER:
			try:
				_int = int(float(_val))
			except:
				_int = 0
			if _val != _int or _int < 2:
				self.taint_entry(self.nsteps,"Number of steps must be positive integer >= 2")
				return 0
			self.nsteps.set_text(_int)
		self.taint_entry(self.nsteps, good=1)
	
	def on_nsteps_changed(self, *args):
		self.validate_nsteps()
		setattr(self,("nsteps_number","nsteps_increm","nsteps_ratio")[self.get_step_type()],self.nsteps.get_text())
		
	def on_check_toggled(self, *args):
		# update the preference for behaviour on solver fail
		_p = self.browser.prefs
		_p.setBoolPref("StudyReporter", "continue_on_fail", self.checkbutton.get_active())
		
	def taint_entry(self, entry, good=0, msg=None):
		"""
		Color an input box red/white according to value of 'good'; add a further
		error message via the tooltip on the secondary icon, if provided.
		"""
		color = "white"
		if not good:
			color = "#FFBBBB"
		for s in [gtk.STATE_NORMAL, gtk.STATE_ACTIVE]:
			entry.modify_bg(s, gtk.gdk.color_parse(color))
			entry.modify_base(s, gtk.gdk.color_parse(color))
		# FIXME don't apply logic to hard-wired colour codes
		if not good:
			entry.set_property("secondary-icon-stock", 'gtk-dialog-error')
		else:
			entry.set_property("secondary-icon-stock", 'gtk-yes')
			entry.set_property("secondary-icon-tooltip-text", "")
		entry.set_property("secondary-icon-tooltip-text", msg)

	def taint_dist(self, good=0, msg=None):
		"""
		Taint the distribution combobox, using the icon placed to its right.
		Add a message as tooltip on the error icon, if provided.
		"""
		if good:
			_icon = 'gtk-yes'
		else:
			_icon = 'gtk-dialog-error'		
		self.check_dist.set_from_stock(_icon, gtk.ICON_SIZE_BUTTON)
		self.check_dist.set_tooltip_text(msg)

	def parse_entry(self, entry):
		"""
		Parse an input box and enforce dimensional agreement with self.instance.
		"""
		# FIXME Add missing units if they have not been entered.
		i = RealAtomEntry(self.instance, entry.get_text())
		_msg = None
		try:
			i.checkEntry()
			_value = i.getValue()
		except InputError, e:
			# FIXME does the following line actually clear out the entry box?
			_value = None
			_error = re.split('Input Error: ', str(e), 1)
			_msg = _error[1]
		
		if _value is not None:
			self.taint_entry(entry, good=1)
		else:
			self.taint_entry(entry, msg=_msg)
		return _value
				
	def solve(self):
		# we can assume that all the inputs have been validated
		_start = self.parse_entry(self.lowerb)
		_end = self.parse_entry(self.upperb)

		_dist = self.dist.get_active_text()
		_st = self.get_step_type()
		_step = None
		_nsteps = None
		if _dist==DIST_LINEAR:
			if _st==STEP_NUMBER:
				_nsteps = int(self.nsteps.get_text())
				_step = (_end - _start)/(_nsteps)
			elif _st==STEP_INCREM:
				_step = self.parse_entry(self.nsteps)
				# TODO convert to real value without units?
				_nsteps = int((_end - _start)/_step)
		elif _dist==DIST_LOG:
			if _st==STEP_RATIO:
				_step = log(float(self.nsteps.get_text()))
				_nsteps = int((log(_end) - log(_start))/_step)
			elif _st==STEP_NUMBER:
				_nsteps = int(self.nsteps.get_text())
				_step = (log(_end) - log(_start))/_nsteps

		if _step is None or _nsteps is None:	
			raise RuntimeError("invalid step selection")

		_b = self.browser
		
		if not hasattr(self.browser,'solver'):
			_b.reporter.reportError("No solver assigned!")
			return
		
		if _b.no_built_system():
			return
		_b.start_waiting("Solving with %s..." % _b.solver.getName())
		self.studywin.destroy()
		reporter = StudyReporter(_b, _b.sim.getNumVars(), self.instance, _nsteps, self)

		# FIXME move following code to the StudyReporter class?
		i = 0
		_val = _start
		while i<=_nsteps and reporter.guiinterrupt == False:
			# run a method, if requested
			if self.method:
				try:
					_b.sim.run(method)
				except RuntimeError,e:
					_b.reporter.reportError(str(e))
				
			# set the value (do it inside the loop to avoid METHOD possibly unfixing)
			if self.instance.getType().isRefinedSolverVar():
				# for solver vars, set the 'fixed' flag as well
				## FIXME this function seems to somehow be repeatedly parsing units: avoid doing that every step.
				self.instance.setFixedValue(_val)
			else:
				# what other kind of variable is it possible to study, if not a solver_var? integer? not suppported?
				self.instance.setRealValue(_val)
			
			#solve
			# FIXME where is the continue_on_fail thing?
			try:
				reporter.updateVarDetails(i)
				_b.sim.solve(_b.solver, reporter)
			except RuntimeError,e:
				_b.reporter.reportError(str(e))

			i += 1
			# any issue with accumulation of rounding errors here?
			if _dist==DIST_LOG:
				_val = exp(log(_start)+i*_step)
			else:
				_val = _start + i*_step

		if reporter.continue_on_fail == True:
			reporter.updateVarDetails(i)
		
		_b.stop_waiting()
		_b.modelview.refreshtree()
		
