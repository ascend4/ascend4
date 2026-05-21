import sys
from gi.repository import GObject

import loading
from preferences import *
from observer import *
from plotutils import COLOR_CYCLE, add_series_legend, finish_time_series_layout, group_series, group_ylabel, style_time_axis

try:
	import matplotlib.pyplot as plt
except:
	pass
		
# When writing this class, we assume that the integrator class has already had
# its "analyse" method called, so we know all that stuff like the number of
# observed variables, what our time samples are, what the independent variable 
# is, etc.

INTEGRATOR_NUM = 0

def _observed_instances(integrator):
	return [integrator.getObservedInstance(i) for i in range(0, integrator.getNumObservedItems())]

def _observed_values(integrator):
	values = []
	for inst in _observed_instances(integrator):
		if inst.isSelector():
			values.append(str(inst.getSelectorValue()))
		elif inst.isSymbol():
			values.append(str(inst.getSymbolValue()))
		elif inst.isBool():
			values.append(bool(inst.getBoolValue()))
		elif inst.isInt():
			values.append(int(inst.getIntValue()))
		else:
			values.append(inst.getRealValue())
	return values

class IntegratorReporterPython(ascpy.IntegratorReporterCxx):
	def __init__(self,browser,integrator):
		self.browser=browser
		self.integrator_ref=integrator
		ascpy.IntegratorReporterCxx.__init__(self,integrator)
		self.autoplot_results = True
		
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
		self.integration_succeeded = False
		self.observed_rows = []

	def _get_observed_instances(self):
		return _observed_instances(self.getIntegrator())

	def _get_current_observed_values(self):
		return _observed_values(self.getIntegrator())

	def solve_thread(self):
		try:
			self.getIntegrator().solve()
			self.integration_succeeded = True
		except RuntimeError as e:
			self.integration_succeeded = False
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
			try:
				self.getIntegrator().writeMatrix(_fn, None)
			except RuntimeError as e:
				self.browser.reporter.reportError(str(e))

	def finish(self):
		self.window.destroy()
		if self.integration_succeeded:
			try:
				self.getIntegrator().processVarStatus()
			except Exception as e:
				sys.stderr.write("\n\n\nIntegratorReporter.finish: status update error: %s: %s\n\n\n" % (e.__class__,str(e)))
		self.browser.modelview.refreshtree()
		self.browser.update_simulation_statusbar()
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
		self.observed_rows = []

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
			self.browser.currentobservertab = _tab
			self.browser.currentpage = _tab

			# add the columns
			_obs.add_instance(integrator.getIndependentVariable().getInstance())
			for _inst in self._get_observed_instances():
				_obs.add_instance(_inst)

			for data in self.observed_rows:
				_vals, _time = data[:-1], data[-1]
				_obs.do_add_row([_time] + [_v for _v in _vals])
			self.browser.maintabs.set_current_page(_tab)
			if self.autoplot_results and self.browser.prefs.getBoolPref("Integrator", "autoplotresults", True):
				GObject.idle_add(self.autoplot_observer, _obs)
		except Exception as e:
			sys.stderr.write("\n\n\nIntegratorReporter.close_output: error: %s: %s\n\n\n" % (e.__class__,str(e)))
			self.solve_status = 1

		self.cancelrequested = True
		return False

	def autoplot_observer(self, obs):
		try:
			_plottable = [idx for idx, col in obs.cols.items() if col.is_plottable()]
			if len(_plottable) >= 2:
				obs.plot(x=_plottable[0], y=_plottable[1:])
		except Exception as e:
			sys.stderr.write("\n\n\nIntegratorReporter.autoplot_observer: error: %s: %s\n\n\n" % (e.__class__,str(e)))
			self.solve_status = 1
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
		except Exception as e:
			print("\n\nERROR IN UPDATESTATUS!",str(e))
			self.solve_status = 0
			return False

		return True

	def updateStatus(self):
		# time needed to update gui, due to GIL
		time.sleep(0.000001)
		return self.solve_status

	def recordObservedValues(self):
		I = self.getIntegrator()
		self.observed_rows.append(self._get_current_observed_values() + [I.getCurrentTime()])
		if I.getNumObservedVars() > 0:
			self.getIntegrator().saveObservations()
		return 1

# no need to move solving to background task because there is no way to interrupt it
class IntegratorReporterFile(ascpy.IntegratorReporterCxx):
	def __init__(self,integrator,filep):
		self.integrator_ref=integrator
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
			names = [inst.getName() for inst in _observed_instances(I)]
			self.filep.write("#%s\t" % self.indepname)
			self.filep.write("\t".join(names)+"\n")
		except Exception as e:
			print("ERROR %s" % str(e))
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
		except Exception as e:
			print("ERROR %s" % str(e))
			return 0
		return 1

	def recordObservedValues(self):
		try:
			I = self.getIntegrator()
			#print str(obs)
			self.filep.write("%f\t" % I.getCurrentTime())
			self.filep.write("\t".join([str(i) for i in _observed_values(I)])+"\n")
		except Exception as e:
			print("ERROR %s" % str(e))
			return 0
		return 1

class IntegratorReporterPlot(IntegratorReporterPython):
	"""Plotting integrator reporter"""
	def __init__(self, browser, integrator, start, stop):
		self.start = start
		self.stop = stop
		self.x = []
		self.live_series = []
		self.live_groups = []
		self.live_axes = []
		self.figure = None
		self.lines = []
		loading.load_matplotlib(alert=True)
		IntegratorReporterPython.__init__(self, browser, integrator)
		self.autoplot_results = False

	def init_output(self):
		IntegratorReporterPython.init_output(self)

		for index, inst in enumerate(self._get_observed_instances()):
			if not inst.isReal():
				continue
			col = ObserverColumn(inst, index + 1, browser=self.browser)
			self.live_series.append({
				"index": index,
				"column": col,
				"values": [],
				"line": None,
			})

		if len(self.live_series) == 0:
			raise RuntimeError("Plot reporter requires at least one real-valued observed instance")

		grouped, group_order = group_series(
			self.live_series,
			lambda series: series["column"].display_unit_name()
		)
		for group in group_order:
			entries = grouped[group]
			self.live_groups.append({
				"series": entries,
				"ylabel": group_ylabel(
					entries,
					lambda series: series["column"].display_unit_name(),
					lambda series: series["column"].title,
				),
			})

		self.figure, axes = plt.subplots(
			len(self.live_groups),
			1,
			squeeze=False,
			sharex=True,
			figsize=(10.5, max(4.2, 1.75 * len(self.live_groups))),
		)
		self.live_axes = [ax[0] for ax in axes]
		has_outside_legend = any(len(group["series"]) > 1 for group in self.live_groups)
		for ax_index, (ax, group) in enumerate(zip(self.live_axes, self.live_groups)):
			ax.set_xlim(self.start, self.stop)
			ax.set_autoscaley_on(True)
			ax.set_ylabel(group["ylabel"], labelpad=20)
			style_time_axis(ax)
			for series_index, series in enumerate(group["series"]):
				color = COLOR_CYCLE[series_index % len(COLOR_CYCLE)]
				line, = ax.plot([], [], "-", color=color, linewidth=1.6, label=series["column"].name)
				series["line"] = line
				self.lines.append(line)
			leg = add_series_legend(ax, len(group["series"]))
			if ax_index + 1 != len(self.live_axes):
				plt.setp(ax.get_xticklabels(), visible=False)
		self.live_axes[-1].set_xlabel("t")
		finish_time_series_layout(self.figure, self.live_axes, has_outside_legend)
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
			for series in self.live_series:
				series["line"].set_xdata(self.x)
				series["line"].set_ydata(series["values"])
			for ax in self.live_axes:
				# need both of these in order to rescale
				ax.relim()
				ax.autoscale_view()
			# we need to draw *and* flush
			self.figure.canvas.draw()
			self.figure.canvas.flush_events()
		except Exception as e:
			print("ERROR plotupdate %s" % str(e))
			self.solve_status = 0

		return IntegratorReporterPython.update_status(self)

	def recordObservedValues(self):
		try:
			i = self.getIntegrator()
			obs = self._get_current_observed_values()
			self.x.append(i.getCurrentTime())
			for series in self.live_series:
				series["values"].append(obs[series["index"]])
		except Exception as e:
			print("ERROR record %s" % str(e))
			self.solve_status = 0

		return IntegratorReporterPython.recordObservedValues(self)
