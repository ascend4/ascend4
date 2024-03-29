import threading
from math import log, exp

from gi.repository import Gdk
from gi.repository import GObject

from celsiusunits import CelsiusUnits
from varentry import *
from studyreporter import *

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
		self.step_menu_model = Gtk.ListStore(int,str)
		self.step_menu_model.append([STEP_NUMBER,'No. of steps'])
		self.step_menu_model.append([STEP_INCREM,'Step size'])
		self.step_menu_model.append([STEP_RATIO, 'Step ratio'])
		renderer = Gtk.CellRendererText()
		self.step_menu.set_model(self.step_menu_model)
		self.step_menu.pack_start(renderer, True)
		self.step_menu.add_attribute(renderer, 'text',1)
		self.step_menu.set_active(0)
		#self.step_type="No. of steps"

		_p = self.browser.prefs
		_continue_on_fail = _p.getBoolPref("StudyReporter", "continue_on_fail", True)
		self.checkbutton.set_active(_continue_on_fail)

		# set up the distributions combobox
		dist_model = Gtk.ListStore(str)
		dist_model.append([DIST_LINEAR])
		dist_model.append([DIST_LOG])
		self.dist.set_model(dist_model)
		self.dist.set_active(0)

		# set up the methods combobox
		_methodstore = self.browser.methodstore
		_methodrenderer = Gtk.CellRendererText()
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

		for _k,_v in _arr.items():
			_t = str(_v / _conversion)+" "+_u
			##### CELSIUS TEMPERATURE WORKAROUND
			_t = CelsiusUnits.convert_show(self.instance, str(_v), True, default=_t)
			##### CELSIUS TEMPERATURE WORKAROUND
			_k.set_text(_t)
		
		self.browser.builder.connect_signals(self)
		self.lowerb.select_region(0, -1)
		self.solve_interrupt = False
		self.data = {}
	
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
			if _res == Gtk.ResponseType.OK:
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
			elif _res==Gtk.ResponseType.CANCEL:
				# cancel... exit Study
				break
		self.studywin.destroy()
		
	def on_studywin_close(self,*args):
		self.studywin.response(Gtk.ResponseType.CANCEL)

	def on_key_press_event(self,widget,event):
		keyname = Gdk.keyval_name(event.keyval)
		if keyname=="Return":
			self.studywin.response(Gtk.ResponseType.OK)
			return True
		elif keyname=="Escape":
			self.studywin.response(Gtk.ResponseType.CANCEL)
			return True;
		return False;
		
	def on_methodrun_changed(self, *args):
		index = self.methodrun.get_active()
		piter = self.methodrun.get_model().get_iter(Gtk.TreePath.new_from_string(str(index)))
		_sel = self.methodrun.get_model().get_value(piter, 0)
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
			self.check_dist.set_from_stock('gtk-yes', Gtk.IconSize.BUTTON)
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
		for s in [Gtk.StateType.NORMAL, Gtk.StateType.ACTIVE]:
			entry.modify_bg(s, Gdk.color_parse(color))
			entry.modify_base(s, Gdk.color_parse(color))
		# FIXME don't apply logic to hard-wired colour codes
		if not good:
			entry.set_property("secondary-icon-stock", 'gtk-dialog-error')
		else:
			entry.set_property("secondary-icon-stock", 'gtk-yes')

		# causes gtk-critical errors
			# entry.set_property("secondary-icon-tooltip-text", "")
		# entry.set_property("secondary-icon-tooltip-text", msg)

	def taint_dist(self, good=0, msg=None):
		"""
		Taint the distribution combobox, using the icon placed to its right.
		Add a message as tooltip on the error icon, if provided.
		"""
		if good:
			_icon = 'gtk-yes'
		else:
			_icon = 'gtk-dialog-error'		
		self.check_dist.set_from_stock(_icon, Gtk.IconSize.BUTTON)
		if msg!=None:
		    self.check_dist.set_tooltip_text(msg)

	def parse_entry(self, entry):
		"""
		Parse an input box and enforce dimensional agreement with self.instance.
		"""
		newtext = entry.get_text()
		##### CELSIUS TEMPERATURE WORKAROUND
		newtext = CelsiusUnits.convert_edit(self.instance, newtext, False)
		##### CELSIUS TEMPERATURE WORKAROUND
		# FIXME Add missing units if they have not been entered.
		i = RealAtomEntry(self.instance, newtext)
		_msg = None
		try:
			i.checkEntry()
			_value = i.getValue()
		except InputError as e:
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

		self.data = {}
		for tab in self.browser.observers:
			if tab.alive:
				self.data[tab.name] = []
		self.solve_interrupt = False
		thread = threading.Thread(target=self.solve_thread, args=(_b, reporter, _start, _step, _nsteps, _dist))
		thread.daemon = True
		thread.start()

	def solve_thread(self, browser, reporter, start, step, nsteps, dist):
		i = 0
		while i <= nsteps and not self.solve_interrupt:
			# run a method, if requested
			if self.method:
				try:
					browser.sim.run(self.method)
				except RuntimeError as e:
					browser.reporter.reportError(str(e))

			# any issue with accumulation of rounding errors here?
			if dist == DIST_LOG:
				_val = exp(log(start) + i * step)
			else:
				_val = start + i * step

			# set the value (do it inside the loop to avoid METHOD possibly unfixing)
			if self.instance.getType().isRefinedSolverVar():
				# for solver vars, set the 'fixed' flag as well
				self.instance.setFixedValue(_val)
			else:
				# what other kind of variable is it possible to study, if not a solver_var? integer? not suppported?
				self.instance.setRealValue(_val)

			GObject.idle_add(self.solve_update, reporter, i)
			try:
				browser.sim.presolve(browser.solver)
				status = browser.sim.getStatus()
				while status.isReadyToSolve() and not self.solve_interrupt:
					res = browser.sim.iterate()
					status.getSimulationStatus(browser.sim)
					GObject.idle_add(self.solve_update_step, reporter, status)
					# 'make' some time for gui update
					time.sleep(0.001)
					if res != 0:
						break
				self.save_data()
				GObject.idle_add(self.solve_finish_step, reporter, status)
				browser.sim.postsolve(status)
			except RuntimeError as err:
				browser.reporter.reportError(str(err))

			i += 1

		GObject.idle_add(self.solve_update, reporter, i)
		GObject.idle_add(self.solve_finish, browser, reporter)

	def save_data(self):
		for tab in self.browser.observers:
			if tab.alive:
				v = tab.get_values()
				self.data[tab.name].append(v)

	def solve_update(self, reporter, i):
		reporter.updateVarDetails(i)
		return False

	def solve_update_step(self, reporter, status):
		self.solve_interrupt = reporter.report(status)
		return False

	def solve_finish_step(self, reporter, status):
		reporter.finalise(status)
		return False

	def solve_finish(self, browser, reporter):
		reporter.report_observed(self.data)
		browser.stop_waiting()
		browser.modelview.refreshtree()
		return False
