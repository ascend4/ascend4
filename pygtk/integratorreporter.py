import ascpy
import time
import sys
import gtk
import gtk.glade
import time
from varentry import *
from preferences import *

from observer import *

# When writing this class, we assume that the integrator class has already had
# its "analyse" method called, so we know all that stuff like the number of
# observed variables, what our time samples are, what the independent variable 
# is, etc.

INTEGRATOR_NUM = 0

class IntegratorReporterPython(ascpy.IntegratorReporterCxx):
	def __init__(self,browser,integrator):
		self.browser=browser
	 	ascpy.IntegratorReporterCxx.__init__(self,integrator)
		
		# GUI elements
		_xml = gtk.glade.XML(browser.glade_file,"integratorstatusdialog")
		_xml.signal_autoconnect(self)
		self.window=_xml.get_widget("integratorstatusdialog")
		self.window.set_transient_for(self.browser.window)
		self.label=_xml.get_widget("integratorlabel")
		self.label.set_text("Solving with "+self.getIntegrator().getEngineName())
		self.progress=_xml.get_widget("integratorprogress")
		self.data = None

		self.cancelrequested=False

	def run(self):
		# run the dialog: start solution, monitor use events
		_res = self.getIntegrator().solve()
		self.window.destroy()
		return _res

	def on_cancelbutton_clicked(self,*args):
		self.cancelrequested=True

	def initOutput(self):
		# empty out the data table
		self.data=[]
		self.nsteps = self.getIntegrator().getNumSteps()
		self.progress.set_text("Starting...")
		self.progress.set_fraction(0.0)
		#update the GUI
		while gtk.events_pending():
			gtk.main_iteration()
		return 1

	def closeOutput(self):
		global INTEGRATOR_NUM
		integrator = self.getIntegrator()
		# create an empty observer
		try:
			_xml = gtk.glade.XML(self.browser.glade_file,"observervbox")
			_label = gtk.Label();
			INTEGRATOR_NUM = INTEGRATOR_NUM + 1
			_name = "Integrator %d" % INTEGRATOR_NUM
			_tab = self.browser.maintabs.append_page(_xml.get_widget("observervbox"),_label)
			_obs = ObserverTab(xml=_xml, name=_name, browser=self.browser, tab=_tab, alive=False)
			_label.set_text(_obs.name)
			self.browser.observers.append(_obs)
			self.browser.tabs[_tab]=_obs
			
			# add the columns
			_obs.add_instance(integrator.getIndependentVariable().getInstance())
			for _v in [integrator.getObservedVariable(_i) for _i in range(0,integrator.getNumObservedVars())]:
				_obs.add_instance(_v.getInstance())

			for _time,_vals in self.data:
				_obs.do_add_row([_time]+[_v for _v in _vals])
		except Exception,e:
			sys.stderr.write("\n\n\nCAUGHT EXCEPTION: %s\n\n\n" % str(e))
		return 0

	def closeOutput1(self):
		# output the results (to the console, for now)
		for _t,_vals in self.data:
			print _t,_vals

		self.progress.set_fraction(1.0)
		self.progress.set_text("Finished.")
		return 1

	def updateStatus(self):
		# outdate the GUI
		try:
			# TODO: change so it's not updating every step!
			t = self.getIntegrator().getCurrentTime()
			_frac = float(self.getIntegrator().getCurrentStep())/self.nsteps
			self.progress.set_text("t = %f" % (self.getIntegrator().getCurrentTime()))
			self.progress.set_fraction(_frac)
			while gtk.events_pending():
				gtk.main_iteration()
			if self.cancelrequested:
				return 0	
			return 1
		except Exception,e:
			print "\n\nERROR IN UPDATESTATUS!",str(e)
			return 0

	def recordObservedValues(self):
		# just add to our in-memory data structure for now...
		try:
			i = self.getIntegrator()
			print str(i.getCurrentObservations())		
			self.data.append((i.getCurrentTime(),i.getCurrentObservations()))
		except Exception,e:
			print "\n\nERROR IN RECORDOBSERVEDVALUES!",str(e)
			return 0
		return 1



