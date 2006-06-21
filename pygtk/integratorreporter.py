import ascpy
import time
import gtk
import gtk.glade
import time
from varentry import *
from preferences import *

# When writing this class, we assume that the integrator class has already had
# its "analyse" method called, so we know all that stuff like the number of
# observed variables, what our time samples are, what the independent variable 
# is, etc.

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

	def run(self):
		# run the dialog: start solution, monitor use events
		_res = self.getIntegrator().solve()
		self.window.destroy()
		return _res

	def initOutput(self):
		# empty out the data table
		self.data=[]
		self.nsteps = self.getIntegrator().getNumSteps()
		self.progress.set_text("Starting...")
		self.progress.set_fraction(0.0);
		#update the GUI
		while gtk.events_pending():
			gtk.main_iteration()
		return 1

	def closeOutput(self):
		# output the results (to the console, for now)
		for _k,_v in self.data:
			print _k,_v

		self.progress.set_fraction(1.0);
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
