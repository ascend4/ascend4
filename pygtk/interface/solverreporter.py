import ascend
import time
import gtk
import gtk.glade
import config

class PythonSolverReporter(ascend.SolverReporter):
	def __init__(self,browser):
		self.browser=browser
		self.updateinterval = self.browser.prefs.getBoolPref("SolverReporter","update_interval", 0.5)
		self.reporter = self.browser.reporter
		if self.reporter==None:
			raise RuntimeError("Can't find reporter")
		self.starttime = time.clock()
		self.statusbarcontext = self.browser.statusbar.get_context_id("pythonsolverreporter")
		self.browser.statusbar.push(self.statusbarcontext,"Solving...")
		ascend.SolverReporter.__init__(self)

	def report_to_browser(self,status):
		self.browser.statusbar.pop(self.statusbarcontext)

		if status.isConverged():
			self.reporter.reportSuccess("Converged")
			return
		elif status.hasExceededTimeLimit():
			_msg = "Solver exceeded time limit"
		elif status.hasExceededIterationLimit():
			_msg = "Solver exceeded iteration limit"
		elif status.isDiverged():
			_msg = "Solver diverged"
		else:
			_msg = "Solve failed (unknown reason: check console)"

		_msg = _msg + " while solving block %d/%d (%d vars in block)" % (status.getCurrentBlockNum(),
				status.getNumBlocks(),status.getCurrentBlockSize() )
		self.reporter.reportError(_msg)



class PopupSolverReporter(PythonSolverReporter):
	def __init__(self,browser,numvars):
		PythonSolverReporter.__init__(self,browser)

		_xml = gtk.glade.XML(config.GLADE_FILE,"solverstatusdialog")
		_xml.signal_autoconnect(self)

		self.window = _xml.get_widget("solverstatusdialog")
		self.window.set_icon(self.browser.icon)
		self.window.set_transient_for(self.browser.window)
		
		self.numvars = _xml.get_widget("numvarsentry")
		self.numblocks = _xml.get_widget("numblocksentry")
		self.elapsedtime = _xml.get_widget("elapsedtimeentry")
		self.numiterations = _xml.get_widget("numiterationsentry")
		self.blockvars = _xml.get_widget("blockvarsentry")
		self.blockiterations = _xml.get_widget("blockiterationsentry")
		self.blockresidual = _xml.get_widget("blockresidualentry")
		self.blockelapsedtime = _xml.get_widget("blockelapsedtimeentry")
	
		self.progressbar = _xml.get_widget("progressbar")
		self.closebutton = _xml.get_widget("closebutton")
		self.stopbutton = _xml.get_widget("stopbutton")
			
		print "SOLVER REPORTER ---- PYTHON"

		self.solvedvars = 0;

		self.lasttime = 0;
		self.blockstart = self.starttime;
		self.blocktime = 0;
		self.elapsed = 0;
		self.blocknum = 0;
		self.guiinterrupt = False;
		self.guitime = 0;

		self.nv = numvars

		while gtk.events_pending():
			gtk.main_iteration()

	def run(self):
		self.window.run()

	def on_stopbutton_clicked(self,*args):
		print "STOPPING..."
		self.guiinterrupt = True;

	def on_solverstatusdialog_close(self,*args):
		self.window.response(gtk.RESPONSE_CLOSE)

	def on_solverstatusdialog_response(self,response,*args):
		self.window.hide()
		del(self.window)
		
	def fill_values(self,status):
		self.numblocks.set_text("%d of %d" % (status.getCurrentBlockNum(),status.getNumBlocks()))
		self.numvars.set_text("%d of %d" % (status.getNumConverged(), self.nv))
		self.elapsedtime.set_text("%0.1f s" % self.elapsed)
		self.numiterations.set_text(str(status.getIterationNum()))
		self.blockvars.set_text(str(status.getCurrentBlockSize()))
		self.blockiterations.set_text(str(status.getCurrentBlockIteration()))
		self.blockresidual.set_text("%8.5e" % status.getBlockResidualRMS())
		self.blockelapsedtime.set_text("%0.1f s" % self.blocktime)

		_frac = float(status.getNumConverged()) / self.nv
		self.progressbar.set_text("%d vars converged..." % status.getNumConverged());
		self.progressbar.set_fraction(_frac)

	def report(self,status):
		_time = time.clock();
		_sincelast = _time - self.lasttime
		if status.getCurrentBlockNum() > self.blocknum:
			self.blocknum = status.getCurrentBlockNum()
			self.blockstart = _time

		if self.lasttime==0 or _sincelast > self.updateinterval or status.isConverged():
			self.lasttime = _time;
			self.elapsed = _time - self.starttime
			self.blocktime = _time - self.blockstart
			#print "UPDATING!"
			self.fill_values(status)

		while gtk.events_pending():
			gtk.main_iteration()		

		self.guitime = self.guitime + (time.clock() - _time)

		if status.isConverged() or status.isDiverged() or status.isInterrupted():
			return 1
		if self.guiinterrupt:
			return 2
		return 0

	def finalise(self,status):
		_time = time.clock()

		_p = self.browser.prefs;
		_close_on_converged = _p.getBoolPref("SolverReporter","close_on_converged",True);
		_close_on_nonconverged = _p.getBoolPref("SolverReporter","close_on_nonconverged",False);


		if status.isConverged() and _close_on_converged:
			self.report_to_browser(status)
			self.window.response(gtk.RESPONSE_CLOSE)
			return
		
		if not status.isConverged() and _close_on_nonconverged:
			self.report_to_browser(status)
			self.window.response(gtk.RESPONSE_CLOSE)
			return

		self.fill_values(status)

		if status.isConverged():
			self.progressbar.set_fraction(1.0)
			self.progressbar.set_text("Converged")
		elif status.hasExceededTimeLimit():
			self.progressbar.set_text("Exceeded time limit")
		elif status.hasExceededIterationLimit():
			self.progressbar.set_text("Exceeded iteration limit")
		elif status.isDiverged():
			self.progressbar.set_text("Diverged")
				
		self.closebutton.set_sensitive(True)
		self.stopbutton.set_sensitive(False)

		self.report_to_browser(status)

		self.guitime = self.guitime + (time.clock() - _time)
		print "TIME SPENT UPDATING SOLVER: %0.2f s" % self.guitime


class SimpleSolverReporter(PythonSolverReporter):
	def __init__(self,browser):
		print "CREATING SIMPLESOLVERREPORTER..."
		PythonSolverReporter.__init__(self,browser)
		self.lasttime = self.starttime

	def report(self,status):
		_time = time.clock()
		if _time - self.lasttime > self.updateinterval:
			self.lasttime = _time
			_msg = "Solved %d vars in %d iterations" % (status.getNumConverged(),status.getIterationNum())
			self.browser.statusbar.push(self.statusbarcontext, _msg )

		while gtk.events_pending():
			gtk.main_iteration()
		return 0

	def finalise(self,status):
		self.report_to_browser(status)
		
