import ascpy
import time
import gtk
import time
from varentry import *
from preferences import *
from integratorreporter import *
from solverparameters import *
from infodialog import *
import tempfile

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

		try:	 
			self.integrator.findIndependentVar()
			self.indepvar = self.integrator.getIndependentVariable()
		except RuntimeError,e:
			self.browser.reporter.reportNote(str(e))
			self.indepvar = None
			return

		# locate all the widgets
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["integratorwin","list_of_td"])
		self.browser.builder.connect_signals(self)

		self.window = self.browser.builder.get_object("integratorwin")
		self.window.set_transient_for(self.browser.window)

		self.engineselect = self.browser.builder.get_object("engineselect")
		self.beginentry = self.browser.builder.get_object("beginentry")
		self.durationentry = self.browser.builder.get_object("durationentry")
		self.nstepsentry = self.browser.builder.get_object("nstepsentry")
		self.timedistributionselect = self.browser.builder.get_object("timedistributionselect")
		self.settings = {
			# input field: [pref name, default value, export-to-integrator function]
			"initialstep": [1,lambda x:self.integrator.setInitialSubStep(float(x))]
			,"minstep":[0.001,lambda x:self.integrator.setMinSubStep(float(x))]
			,"maxstep":[1000,lambda x:self.integrator.setMaxSubStep(float(x))]
			,"maxsteps":[100,lambda x:self.integrator.setMaxSubSteps(int(x))]
		}

		self.integratorentries={}
		for _k in self.settings.keys():
			_w = self.browser.builder.get_object(_k+"entry")
			if not _w:
				raise RuntimeError("Couldn't find entry for"+_k)
			self.integratorentries[_k]=_w

		# fill values from user preferences, system values, etc
		self.fill_values()

		# set the engine initially
		try:
			self.integrator.setEngine(self.engines[self.engineselect.get_active()])
		except:
			# perhaps no integrators are available
			pass

	def fill_values(self):
		_enginestore = gtk.ListStore(str)
		self.engineselect.set_model(_enginestore)
		_cell = gtk.CellRendererText()
		self.engineselect.pack_start(_cell, True)
		self.engineselect.add_attribute(_cell, 'text', 0)
		
		_engpref = self.prefs.getStringPref("Integrator","engine","LSODE")
		_engindex = 0
		_i = 0
		if len(self.engines):
			for k in self.engines:
				_row = _enginestore.append()
				_enginestore.set(_row,0,k)
				if k==_engpref:
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

		# get the current time value as the beginentry...
		print "SEARCHING FOR TIME VAR..."
		try:
			_u = self.indepvar.getInstance().getType().getPreferredUnits()
			if _u is None:
				_u = self.indepvar.getInstance().getType().getDimensions().getDefaultUnits()
			#_t = self.integrator.getCurrentTime();
			_t = str(self.indepvar.getInstance().getRealValue() / _u.getConversion())
			self.beginentry.set_text(str(_t)+" "+_u.getName().toString())
			self.parse_entry(self.beginentry)
			_dur = self.prefs.getStringPref("Integrator","duration","100")
			self.durationentry.set_text(_dur+" "+_u.getName().toString())
			self.parse_entry(self.durationentry)
		except RuntimeError,e:
			self.browser.reporter.reportNote(str(e))
			self.beginentry.set_text("0")

		self.nstepsentry.set_text("100")
		
		# set preferred timesteps etc
		for _k,_v in self.settings.iteritems():
			if _k!="maxsteps":
				_a = _u.getName().toString()
				self.integratorentries[_k].set_text(self.prefs.getStringPref("Integrator",_k,str(_v[0]))+" "+_a)
				self.parse_entry(self.integratorentries[_k])
			else :
				self.integratorentries[_k].set_text(self.prefs.getStringPref("Integrator",_k,str(_v[0])))


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

	def on_engineselect_changed(self,widget,*args):
		index = widget.get_active()
		print "Setting engine to %d" % index
		print "Engines are",self.engines
		print "Selection is %s" % self.engines[index]
		self.integrator.setEngine(self.engines[index])

	def on_moreparametersbutton_clicked(self,*args):
		print "ZO YOU WANT MORE PAHAMETERS EH!"
		try:
			_name = self.integrator.getName()
			print "NAME = %s" % _name
			_params = self.integrator.getParameters()
		except RuntimeError,e:
			self.browser.reporter.reportError(str(e))
			return
		print "CREATING SOLVERPARAMETERSWINDOW"
		_paramswin = SolverParametersWindow(self.browser,_params,_name)
		print "RUNNING SOLVERPARAMETERSWINDOW"
		if _paramswin.run() == gtk.RESPONSE_OK:
			print "GOT OK RESPONSE"
			self.integrator.setParameters(_params)
			print "PARAMETERS UPDATED"

	def run(self):
		if self.indepvar == None:
			return
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
				else:
					#self.browser.reporter.reportNote("CANCEL event received");
					break
			except IntegratorError,e:
				self.browser.reporter.reportError(str(e))
				# continue

		if _ok:
			try:
				self.integrator.analyse()
			except RuntimeError,e:
				self.browser.reporter.reportError(str(e))
				self.window.destroy()
				if self.prefs.getBoolPref("Integrator","debuganalyse",True):
					fp = tempfile.TemporaryFile()
					self.integrator.writeDebug(fp)
					fp.seek(0)
					text = fp.read()
					fp.close()
					title = "Integrator Analysis Failed"
					_dialog = InfoDialog(self.browser,self.browser.window,text,title,tabs=(70,200,300,400,500))
					_dialog.run()					

				return None							
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

	def taint_entry(self,entry,color):
		# colour an input box if it doesn't have acceptable contents
		# error messages would be reported by the 'errors panel' in the main win
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
		i = RealAtomEntry(self.indepvar.getInstance(), entry.get_text())
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

	def check_inputs(self):
		# set the timesteps (samples)
		_val = {}
		units = ''
		for _k,_v in {
			self.beginentry:[lambda x:float(x),"begin"]
			, self.durationentry:[lambda x:float(x),"duration"]
			, self.nstepsentry:[lambda x:int(x),"num"]
		}.iteritems():
			x = RealAtomEntry(self.indepvar.getInstance(), _k.get_text())
			x.checkEntry()
			if _k == self.beginentry: 
				units = x.units
				#self.indepvar.getInstance().setRealValueWithUnits()
			if _k == self.nstepsentry:
				_val[_v[1]] = _v[0](_k.get_text())
			else :
				_val[_v[1]]=_v[0](x.getValue()/ascpy.Units(units).getConversion())
		
		self.integrator.setLinearTimesteps(ascpy.Units(units), _val["begin"], (_val["begin"]+_val["duration"]), _val["num"]);
		self.begin=_val["begin"]
		self.duration=_val["duration"]

		self.prefs.setStringPref("Integrator","duration",str(self.duration))
		
		# set substep parameters (ie settings common to any integrator engine)
		_failed=False
		x={}
		for _k,_v in self.settings.iteritems():
			try:
				_f = self.integratorentries[_k];
				# pass the substep setting to the integrator
				x[_f] = RealAtomEntry(self.indepvar.getInstance(), _f.get_text())
				x[_f].checkEntry()
				if _k!="maxsteps":
					self.taint_entry(_f,"white")
					_v[1](x[_f].getValue()/ascpy.Units(units).getConversion())
				else:	
					_v[1](_f.get_text())
					self.color_entry(_f,"white")
			except ValueError,e:
				if _k!="maxsteps":
					self.taint_entry(_f,"#FFBBBB")
				else:	
					self.color_entry(_f,"#FFBBBB")
				_failed=True
		
		if _failed:
			raise IntegratorError("Invalid step parameter(s)");

		for _k,_v in self.settings.iteritems():
			# store those inputs for next time
			_f = self.integratorentries[_k]
			if _k!="maxsteps":
				self.prefs.setStringPref("Integrator",_k,str(x[_f].getValue()/ascpy.Units(units).getConversion()))
			else:	
				self.prefs.setStringPref("Integrator",_k,str(_f.get_text()))

		# set engine (and check that it's OK with this system)
		engine=self.engineselect.get_active()
		if engine==-1:
			self.color_entry(self.engineselect,"#FFBBBB")
			raise IntegratorError("No engine selected")

		self.color_entry(self.engineselect,"white")

		try:
			self.prefs.setStringPref("Integrator","engine",self.engines[engine])
			_res = self.integrator.setEngine(self.engines[engine])			
		except IndexError,e:
			raise IntegratorError("Unable to set engine: %s" % e) 
