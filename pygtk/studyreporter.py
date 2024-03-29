import ascpy
import time
from gi.repository import Gtk
from solverreporter import PythonSolverReporter

class StudyReporter(PythonSolverReporter):
	def __init__(self, browser, numvars, instance, nsteps, study):
		PythonSolverReporter.__init__(self,browser)

		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["studystatusdialog"])
		self.study = study
		self.window = self.browser.builder.get_object("studystatusdialog")
		self.browser.builder.connect_signals(self)
		if self.browser.icon:
			self.window.set_icon(self.browser.icon)
		self.window.set_transient_for(self.browser.window)
		
		self.studyvar = self.browser.builder.get_object("studyvarentry")
		self.currentvalue = self.browser.builder.get_object("currentvalueentry")
		self.points = self.browser.builder.get_object("pointsentry")
		self.totaltime = self.browser.builder.get_object("totaltimeentry")
		self.currentrun = self.browser.builder.get_object("currentrunlabel")
		self.numvars = self.browser.builder.get_object("numvarsentry1")
		self.numblocks = self.browser.builder.get_object("numblocksentry1")
		self.elapsedtime = self.browser.builder.get_object("elapsedtimeentry1")
		self.numiterations = self.browser.builder.get_object("numiterationsentry1")
		self.blockvars = self.browser.builder.get_object("blockvarsentry1")
		self.blockiterations = self.browser.builder.get_object("blockiterationsentry1")
		self.blockresidual = self.browser.builder.get_object("blockresidualentry1")
		self.blockelapsedtime = self.browser.builder.get_object("blockelapsedtimeentry1")
		
		self.currentprogressbar = self.browser.builder.get_object("currentprogressbar")
		self.totalprogressbar = self.browser.builder.get_object("totalprogressbar")
		self.closebutton = self.browser.builder.get_object("closebutton6")
		self.stopbutton = self.browser.builder.get_object("stopbutton1")
			
		#print "SOLVER REPORTER ---- PYTHON"
		_p = self.browser.prefs
		self.continue_on_fail = _p.getBoolPref("StudyReporter", "continue_on_fail", True)
		self.solvedvars = 0;

		self.lasttime = 0;
		self.blockstart = self.starttime;
		self.blocktime = 0;
		self.elapsed = 0;
		self.blocknum = 0;
		self.guiinterrupt = False;
		self.guitime = 0;
		self.studybegintime = time.perf_counter()
		self.totalelapsed = 0
		self.nv = numvars
		self.instance = instance
		self.nsteps = nsteps
		self.pointsdone = 0
		self.allconverged = True

	def on_stopbutton_activate(self,*args):
		self.guiinterrupt = True

	def on_studystatusdialog_response(self,widget,response):
		self.guiinterrupt = True
		self.window.destroy()
		
	def fill_values(self,status):
		
		self.studyvar.set_text(self.browser.sim.getInstanceName(self.instance))
		self.currentvalue.set_text(str(self.instance.getRealValue()))
		self.points.set_text("%d out of %d solved" % (self.pointsdone, self.nsteps+1))
		self.currentrun.set_text("%s = %0.2f" % (self.browser.sim.getInstanceName(self.instance), self.instance.getRealValue()))
		self.totaltime.set_text("%0.1f s" % self.totalelapsed)
		self.numblocks.set_text("%d of %d" % (status.getCurrentBlockNum(),status.getNumBlocks()))
		self.numvars.set_text("%d of %d" % (status.getNumConverged(), self.nv))
		self.elapsedtime.set_text("%0.1f s" % self.elapsed)
		self.numiterations.set_text(str(status.getIterationNum()))
		self.blockvars.set_text(str(status.getCurrentBlockSize()))
		self.blockiterations.set_text(str(status.getCurrentBlockIteration()))
		self.blockresidual.set_text("%8.5e" % status.getBlockResidualRMS())
		self.blockelapsedtime.set_text("%0.1f s" % self.blocktime)
		
		_frac = float(status.getNumConverged()) / self.nv
		self.currentprogressbar.set_text("%d vars converged..." % status.getNumConverged());
		self.currentprogressbar.set_fraction(_frac)
		
		_frac2 = float(self.pointsdone) / (self.nsteps+1)
		self.totalprogressbar.set_text("%d points solved..." % self.pointsdone);
		self.totalprogressbar.set_fraction(_frac2)

	def updateVarDetails(self, pointsdone):
		self.pointsdone = pointsdone
		
		_frac2 = float(self.pointsdone) / (self.nsteps+1)
		self.totalprogressbar.set_text("%d points solved..." % self.pointsdone);
		self.totalprogressbar.set_fraction(_frac2)
		self.points.set_text("%d out of %d solved" % (self.pointsdone, self.nsteps+1))
		
		self.starttime = time.perf_counter()
		self.blocknum = 0
		
	def report(self,status):
		_time = time.perf_counter();
		_sincelast = _time - self.lasttime
		if status.getCurrentBlockNum() > self.blocknum:
			self.blocknum = status.getCurrentBlockNum()
			self.blockstart = _time

		if self.lasttime==0 or _sincelast > self.updateinterval:
			self.lasttime = _time;
			self.elapsed = _time - self.starttime
			self.totalelapsed = _time - self.studybegintime
			self.blocktime = _time - self.blockstart
			#print "UPDATING!"
			self.fill_values(status)

		self.guitime = self.guitime + (time.perf_counter() - _time)

		if self.guiinterrupt:
			return True

		return False

	def report_observed(self, data):
		#add row in the observer tabs
		for tabs in self.browser.observers:
			if tabs.alive:
				for v in data[tabs.name]:
					tabs.do_add_row(v)

	def finalise(self,status):
		try:
			_time = time.perf_counter()
			_sincelast = _time - self.lasttime
			if _sincelast > self.updateinterval:
				self.fill_values(status)
			
			if status.isConverged():
				self.report_to_browser(status)
				# print "Converged for %s = %s" % (self.browser.sim.getInstanceName(self.instance), self.instance.getRealValue())
				if self.pointsdone == (self.nsteps):
					self.window.response(Gtk.ResponseType.CLOSE)
				return
			
			if not status.isConverged():
				print("NOT Converged for %s = %s" % (self.browser.sim.getInstanceName(self.instance), 
				      self.instance.getRealValue()))
				self.allconverged = False
				self.report_to_browser(status)
				if self.continue_on_fail is True:
					if self.pointsdone == self.nsteps:
						self.closebutton.set_sensitive(True)
						self.stopbutton.set_sensitive(False)
					return
				else:
					self.guiinterrupt = True

			if status.isConverged():
				self.currentprogressbar.set_fraction(1.0)
				self.currentprogressbar.set_text("Converged")
			elif status.hasExceededTimeLimit():
				self.currentprogressbar.set_text("Exceeded time limit")
			elif status.hasExceededIterationLimit():
				self.currentprogressbar.set_text("Exceeded iteration limit")
			elif status.isDiverged():
				self.currentprogressbar.set_text("Diverged")
			elif status.isOverDefined():
				self.currentprogressbar.set_text("Over-defined")
			elif status.isUnderDefined():
				self.currentprogressbar.set_text("Under-defined")
					
			self.closebutton.set_sensitive(True)
			self.stopbutton.set_sensitive(False)

			self.report_to_browser(status)

			self.guitime = self.guitime + (time.perf_counter() - _time)
			print("TIME SPENT UPDATING SOLVER: %0.2f s" % self.guitime)
		except Exception as e:
			print("SOME PROBLEM: %s" % str(e))
