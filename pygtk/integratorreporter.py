import ascpy
import time
import sys
import gtk
import time
from varentry import *
from preferences import *
from infodialog import *
from observer import *
import tempfile

import gobject
try:
	import pylab
except:
	pass
		
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
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["integratorstatusdialog"])
		self.browser.builder.connect_signals(self)
		self.window=self.browser.builder.get_object("integratorstatusdialog")
		self.window.set_transient_for(self.browser.window)
		self.label=self.browser.builder.get_object("integratorlabel")
		self.label.set_text("Solving with "+self.getIntegrator().getName())
		self.progress=self.browser.builder.get_object("integratorprogress")
		self.data = None

		self.cancelrequested=False
		
	def run(self):
		# run the dialog: start solution, monitor use events
		try:
			self.getIntegrator().solve()

		except RuntimeError,e:
			self.browser.reporter.reportError("Integrator failed: %s" % e)

			if self.browser.prefs.getBoolPref("Integrator","writeendmatrix",True):
				if platform.system()=="Windows":
					_deffn = "\\TEMP\\ascintegratormatrix.mtx"
				else:
					_deffn = "/tmp/ascintegratormatrix.mtx"
				_fn = self.browser.prefs.getStringPref("Integrator","matrixfilepath",_deffn)
				self.browser.reporter.reportNote("Writing matrix to file '%s'" % _fn)
				_fp = file(_fn,"w")
				try:
					try: 
						self.getIntegrator().writeMatrix(_fp)
					except RuntimeError,e:
						self.browser.reporter.reportError(str(e))
				finally:
					_fp.close()
		self.window.destroy()
		
		return

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
			_label = gtk.Label();
			INTEGRATOR_NUM = INTEGRATOR_NUM + 1
			_name = "Integrator %d" % INTEGRATOR_NUM
			_tab = self.browser.maintabs.append_page(self.browser.builder.get_object("observervbox"),_label)
			_obs = ObserverTab(name=_name, browser=self.browser, tab=_tab, alive=False)
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
			sys.stderr.write("\n\n\nIntegratorReporter::closeOutput: error: %s\n\n\n" % str(e))
			return 1
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

class IntegratorReporterFile(ascpy.IntegratorReporterCxx):
	def __init__(self,integrator,filep):
		self.filep=filep
		self.numsteps=0
		self.indepname="t"
	 	ascpy.IntegratorReporterCxx.__init__(self,integrator)
		
		
	def run(self):
		self.getIntegrator().solve()

	def initOutput(self):
		try:
			sys.stderr.write("Integrating...\n")
			I = self.getIntegrator()
			self.numsteps=I.getNumSteps()
			self.indepname = I.getIndependentVariable().getName()
			names = [I.getObservedVariable(i).getName() for i \
				in range(I.getNumObservedVars())
			]
			self.filep.write("#%s\t" % self.indepname)
			self.filep.write("\t".join(names)+"\n")
		except Exception,e:
			print "ERROR %s" % str(e)
			return 0
		return 1

	def closeOutput(self):
		sys.stderr.write(" "*20+chr(8)*20)
		sys.stderr.write("Finished, %d samples recorded.\n" % self.numsteps)
		self.filep.write("#end\n")
		return 0

	def updateStatus(self):
		try:
			I = self.getIntegrator()
			t = I.getCurrentTime()
			pct = 100.0 * I.getCurrentStep() / self.numsteps;
			sys.stderr.write("%3.0f%% (%s = %6.3f)           \r" % (pct,self.indepname,t))
		except Exception,e:
			print "ERROR %s" % str(e)
			return 0
		return 1

	def recordObservedValues(self):
		try:
			I = self.getIntegrator()
			obs = I.getCurrentObservations()
			#print str(obs)
			self.filep.write("%f\t" % I.getCurrentTime())
			self.filep.write("\t".join([str(i) for i in obs])+"\n")
		except Exception,e:
			print "ERROR %s" % str(e)
			return 0
		return 1

class IntegratorReporterPlot(ascpy.IntegratorReporterCxx):
	"""Plotting integrator reporter"""
	def __init__(self,integrator):
		self.numsteps=0
		self.indepname="t"
	 	ascpy.IntegratorReporterCxx.__init__(self,integrator)		
		
	def run(self):
		import loading
		loading.load_matplotlib(throw=True)
		self.getIntegrator().solve()

	def initOutput(self):
		try:
			sys.stderr.write("Integrating...\n")
			self.ax = pylab.subplot(111)
			self.canvas = self.ax.figure.canvas
			I = self.getIntegrator()
			self.numsteps=I.getNumSteps()
			self.indepname = I.getIndependentVariable().getName()
			self.x = []
			self.y = []	
			self.line, = pylab.plot(self.x,self.y,animated=True)	
			self.bg = self.canvas.copy_from_bbox(self.ax.bbox)
			gobject.idle_add(self.plotupdate)
			pylab.show()
		except Exception,e:
			print "ERROR %s" % str(e)
			return 0
		return 1

	def closeOutput(self):
		sys.stderr.write(" "*20+chr(8)*20)
		sys.stderr.write("Finished, %d samples recorded.\n" % self.numsteps)
		return 0

	def updateStatus(self):
		return 1

	def plotupdate(self):
		sys.stderr.write("%d...\r " % len(self.x))
		try:
			self.canvas.restore_region(self.bg)
			self.line.set_data(self.x, self.y)
			self.ax.draw_artist(self.line)
			self.canvas.blit(self.ax.bbox)
		except Exception,e:
			print "ERROR %s" % str(e)

	def recordObservedValues(self):
		try:
			I = self.getIntegrator()
			obs = I.getCurrentObservations()
			self.x.append(I.getCurrentTime())
			self.y.append(obs[0])
		except Exception,e:
			print "ERROR %s" % str(e)
			return 0
		return 1
	

