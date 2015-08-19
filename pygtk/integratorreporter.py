import sys
from gi.repository import GObject

import loading
from preferences import *
from observer import *

try:
	import matplotlib.pyplot as plt
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
		self.solve_status = 1
		self.cancelrequested = False

	def solve_thread(self):
		try:
			self.getIntegrator().solve()
		except RuntimeError, e:
			GObject.idle_add(self.report_error, e)

		GObject.idle_add(self.close_output)
		GObject.idle_add(self.finish)

	def report_error(self, e):
		self.browser.reporter.reportError("Integrator failed: %s" % e)

		if self.browser.prefs.getBoolPref("Integrator", "writeendmatrix", True):
			if platform.system() == "Windows":
				_deffn = "\\TEMP\\ascintegratormatrix.mtx"
			else:
				_deffn = "/tmp/ascintegratormatrix.mtx"
			_fn = self.browser.prefs.getStringPref("Integrator", "matrixfilepath", _deffn)
			self.browser.reporter.reportNote("Writing matrix to file '%s'" % _fn)
			_fp = file(_fn,"w")
			try:
				try:
					self.getIntegrator().writeMatrix(_fp, None)
				except RuntimeError, e:
					self.browser.reporter.reportError(str(e))
			finally:
				_fp.close()

	def finish(self):
		self.window.destroy()
		self.browser.sim.processVarStatus()
		self.browser.modelview.refreshtree()
		return False

	def run(self):
		self.solve_status = 1
		self.init_output()
		thread = threading.Thread(target=self.solve_thread)
		thread.daemon = True
		thread.start()
		GObject.idle_add(self.update_status)

	def on_cancelbutton_clicked(self,*args):
		self.cancelrequested=True

	def init_output(self):
		self.nsteps = self.getIntegrator().getNumSteps()
		self.progress.set_text("Starting...")
		self.progress.set_fraction(0.0)

	def initOutput(self):
		return 1

	def close_output(self):
		# update gui last time
		self.update_status()
		global INTEGRATOR_NUM
		integrator = self.getIntegrator()
		# create an empty observer
		try:
			_label = Gtk.Label();
			INTEGRATOR_NUM = INTEGRATOR_NUM + 1
			_name = "Integrator %d" % INTEGRATOR_NUM
			self.browser.builder.add_objects_from_file(self.browser.glade_file,
				["observervbox","observercontext"] + ["image%d"%n for n in range(7,12)]
			)
			_vbox = self.browser.builder.get_object("observervbox")
			toolbar_list = _vbox.get_children()
			toolbar = toolbar_list.__getitem__(0)
			toolitem6 = toolbar.get_nth_item(3)
			toolitem6_label = toolitem6.get_child()
			toolitem6_label.set_text('')
			_tab = self.browser.maintabs.append_page(_vbox,_label)
			_obs = ObserverTab(name=_name, browser=self.browser, tab=_tab, alive=False)
			_label.set_text(_obs.name)
			self.browser.observers.append(_obs)
			self.browser.tabs[_tab]=_obs

			# add the columns
			_obs.add_instance(integrator.getIndependentVariable().getInstance())
			for _v in [integrator.getObservedVariable(_i) for _i in range(0,integrator.getNumObservedVars())]:
				_obs.add_instance(_v.getInstance())

			obs = self.getIntegrator().getObservations()
			for data in obs:
				# time is always last element in tuple
				_vals, _time = data[:-1], data[-1]
				_obs.do_add_row([_time] + [_v for _v in _vals])
		except Exception,e:
			sys.stderr.write("\n\n\nIntegratorReporter.close_output: error: %s: %s\n\n\n" % (e.__class__,str(e)))
			self.solve_status = 1

		self.cancelrequested = True
		return False

	def closeOutput(self):
		return 0

	def update_status(self):
		try:
			if self.cancelrequested:
				self.solve_status = 0
				return False

			self.progress.set_text("t = %f" % (self.getIntegrator().getCurrentTime()))
			_frac = float(self.getIntegrator().getCurrentStep())/self.nsteps
			self.progress.set_fraction(_frac)
			self.solve_status = 1
		except Exception,e:
			print "\n\nERROR IN UPDATESTATUS!",str(e)
			self.solve_status = 0
			return False

		return True

	def updateStatus(self):
		# time needed to update gui, due to GIL
		time.sleep(0.000001)
		return self.solve_status

	def recordObservedValues(self):
		self.getIntegrator().saveObservations()
		return 1

# no need to move solving to background task because there is no way to interrupt it
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

class IntegratorReporterPlot(IntegratorReporterPython):
	"""Plotting integrator reporter"""
	def __init__(self, browser, integrator, start, stop):
		self.start = start
		self.stop = stop
		self.x = []
		self.y = []
		self.figure = None
		self.ax = None
		self.lines = None
		loading.load_matplotlib(alert=True)
		IntegratorReporterPython.__init__(self, browser, integrator)

	def init_output(self):
		IntegratorReporterPython.init_output(self)
		# set up plot
		self.figure, self.ax = plt.subplots()
		self.lines, = self.ax.plot([], [], 'o')
		# autoscale on unknown axis and known lims on the other
		self.ax.set_xlim(self.start, self.stop)
		self.ax.set_autoscaley_on(True)
		self.ax.grid()
		plt.ion()
		plt.show()

	def run(self):
		IntegratorReporterPython.run(self)

	def initOutput(self):
		return IntegratorReporterPython.initOutput(self)

	def closeOutput(self):
		return IntegratorReporterPython.closeOutput(self)

	def updateStatus(self):
		# more time for plot update
		time.sleep(0.01)
		return IntegratorReporterPython.updateStatus(self)

	def update_status(self):
		try:
			self.lines.set_xdata(self.x)
			self.lines.set_ydata(self.y)
			# need both of these in order to rescale
			self.ax.relim()
			self.ax.autoscale_view()
			# we need to draw *and* flush
			self.figure.canvas.draw()
			self.figure.canvas.flush_events()
		except Exception,e:
			print "ERROR plotupdate %s" % str(e)
			self.solve_status = 0

		return IntegratorReporterPython.update_status(self)

	def recordObservedValues(self):
		try:
			i = self.getIntegrator()
			obs = i.getCurrentObservations()
			self.x.append(i.getCurrentTime())
			self.y.append(obs[0])
		except Exception, e:
			print "ERROR record %s" % str(e)
			self.solve_status = 0

		return IntegratorReporterPython.recordObservedValues(self)
