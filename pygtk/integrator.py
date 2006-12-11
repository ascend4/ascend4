import ascpy
import time
import gtk
import gtk.glade
import time
from varentry import *
from preferences import *
from integratorreporter import *

class IntegratorError(RuntimeError):
	def __init__(self,msg):
		self.msg = msg
	def __str__(self):
		return "Input Error: %s" % self.msg;

class IntegratorWindow:
	def __init__(self,browser,sim):
		# create a new integrator straight away
		self.integrator = ascpy.Integrator(sim)
		self.engines = self.integrator.getEngines()

		self.browser=browser
		self.prefs = Preferences()

		# locate all the widgets
		_xml = gtk.glade.XML(browser.glade_file,"integratorwin")
		_xml.signal_autoconnect(self)

		self.window = _xml.get_widget("integratorwin")
		self.window.set_transient_for(self.browser.window)

		self.engineselect = _xml.get_widget("engineselect")
		self.beginentry = _xml.get_widget("beginentry")
		self.durationentry = _xml.get_widget("durationentry")
		self.nstepsentry = _xml.get_widget("nstepsentry")
		self.timedistributionselect = _xml.get_widget("timedistributionselect")

		self.settings = {
			# input field: [pref name, default value, export-to-integrator function]
			"initialstep": [1,lambda x:self.integrator.setInitialSubStep(float(x))]
			,"minstep":[0.001,lambda x:self.integrator.setMinSubStep(float(x))]
			,"maxstep":[1000,lambda x:self.integrator.setMaxSubStep(float(x))]
			,"maxsteps":[100,lambda x:self.integrator.setMaxSubSteps(int(x))]
		}

		self.integratorentries={}
		for _k in self.settings.keys():
			_w = _xml.get_widget(_k+"entry")
			if not _w:
				raise RuntimeError("Couldn't find entry for"+_k)
			self.integratorentries[_k]=_w

		# fill values from user preferences, system values, etc
		self.fill_values()

	def fill_values(self):
		_enginestore = gtk.ListStore(str)
		self.engineselect.set_model(_enginestore)
		_cell = gtk.CellRendererText()
		self.engineselect.pack_start(_cell, True)
		self.engineselect.add_attribute(_cell, 'text', 1)
		
		_engpref = self.prefs.getStringPref("Integrator","engine","LSODE")
		_engindex = 0
		_i = 0
		if len(self.engines):
			for k in self.engines:
				_row = _enginestore.append()
				_enginestore.set(_row,0,self.engines[k])
				if self.engines[k]==_engpref:
					_engindex=_i
				_i += 1

			# set preferred integrator 	
			self.engineselect.set_active(_engindex)
			self.engineselect.set_sensitive(True)
		else:
			_row = _enginestore.append()
			_enginestore.set(_row,0,"No integrators available")
			self.engineselect.set_active(0)			
			self.engineselect.set_sensitive(False)


		# set preferred timesteps etc
		for _k,_v in self.settings.iteritems():
			self.integratorentries[_k].set_text(self.prefs.getStringPref("Integrator",_k,str(_v[0])))

		# get the current time value as the beginentry...
		print "SEARCHING FOR TIME VAR..."
		if self.integrator.findIndependentVar():
			print "FOUND time var..."	
			_t = self.integrator.getCurrentTime();
			print "Found time = %f" % _t
			self.beginentry.set_text(str(_t))
		else:
			self.browser.reporter.reportNote("No indep var found");
			self.beginentry.set_text("0")

		_dur = self.prefs.getStringPref("Integrator","duration","100")
		self.durationentry.set_text(_dur)
		self.nstepsentry.set_text("100")
		self.timedistributionselect.set_active(0)

	def on_integratorcancel_clicked(self,*args):
		self.browser.reporter.reportNote("CANCELLING");
		self.window.response(gtk.RESPONSE_CANCEL)

	def on_entry_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		if keyname=="Return":
			self.window.response(gtk.RESPONSE_OK)
			return True
		elif keyname=="Escape":
			self.window.response(gtk.RESPONSE_CANCEL)
			return True;
		return False;

	def run(self):
		# focus the engine select box when we start...
		self.engineselect.grab_focus()

		# loop around unitil either cancelled or valid entries are input
		_ok = False
		while True:
			try:
				_res = self.window.run()
				if _res == gtk.RESPONSE_OK:
					self.check_inputs()
					_ok=True
					break
				elif _res == gtk.RESPONSE_CANCEL or _res == gtk.RESPONSE_NONE:
					self.browser.reporter.reportNote("CANCEL event received");
					break
			except IntegratorError,e:
				self.browser.reporter.reportError(str(e))
				# continue

		if _ok:
			_res = self.integrator.analyse()
			if _res:				
				# if we're all ok, create the reporter window and close this one
				_integratorreporter = IntegratorReporterPython(self.browser,self.integrator)
				self.integrator.setReporter(_integratorreporter)
				self.window.destroy()
				return _integratorreporter # means proceed to solve

		self.window.destroy()
		return None # can't solve

	def color_entry(self,entry,color):
		# colour an input box if it doesn't have acceptable contents
		# error messages would be reported by the 'errors panel' in the main win
		entry.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_bg(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse(color))
		entry.modify_base(gtk.STATE_ACTIVE, gtk.gdk.color_parse(color))

	def check_inputs(self):
		# set the timesteps (samples)
		_val = {}
		for _k,_v in {
			self.beginentry:[lambda x:float(x),"begin"]
			, self.durationentry:[lambda x:float(x),"duration"]
			, self.nstepsentry:[lambda x:int(x),"num"]
		}.iteritems():
			_val[_v[1]]=_v[0](_k.get_text())
		
		self.integrator.setLinearTimesteps(ascpy.Units("s"), _val["begin"], (_val["begin"]+_val["duration"]), _val["num"]);
		self.begin=_val["begin"]
		self.duration=_val["duration"]

		self.prefs.setStringPref("Integrator","duration",str(self.duration))
		
		# set substep parameters (ie settings common to any integrator engine)
		_failed=False
		for _k,_v in self.settings.iteritems():
			try:
				_f = self.integratorentries[_k];
				# pass the substep setting to the integrator
				_v[1](_f.get_text())
				self.color_entry(_f,"white")
			except ValueError,e:
				self.color_entry(_f,"#FFBBBB")
				_failed=True
		
		if _failed:
			raise IntegratorError("Invalid step parameter(s)");

		for _k,_v in self.settings.iteritems():
			# store those inputs for next time
			_f = self.integratorentries[_k];
			self.prefs.setStringPref("Integrator",_k,str(_f.get_text()))

		# set engine (and check that it's OK with this system)
		engine=self.engineselect.get_active()
		if engine==-1:
			self.color_entry(self.engineselect,"#FFBBBB")
			raise IntegratorError("No engine selected")

		self.color_entry(self.engineselect,"white")

		self.prefs.setStringPref("Integrator","engine",self.engines.values()[engine])

		try:
			_res = self.integrator.setEngine(self.engines.keys()[engine])			
		except IndexError,e:
			raise IntegratorERror("Unable to set engine: %s",e) 
