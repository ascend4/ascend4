import ascend
import time
import gtk
import gtk.glade

class PythonSolverReporter(ascend.SolverReporter):
	def __init__(self,GLADE_FILE,browser,numvars):
		self.browser=browser
		_xml = gtk.glade.XML(GLADE_FILE,"solverstatusdialog")
		_xml.signal_autoconnect(self)

		self.window = _xml.get_widget("solverstatusdialog")

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
		self.updateinterval = 0.1;

		_time = time.clock();
		self.starttime = _time;
		self.lasttime = 0;
		self.blockstart = _time;
		self.blocktime = 0;
		self.elapsed = 0;
		self.blocknum = 0;
		self.guiinterrupt = False;

		self.nv = numvars
		self.numvars.set_text(str(self.nv))

		ascend.SolverReporter.__init__(self)

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
		print "FILLING VALUES..."
		self.numblocks.set_text("%d of %d" % (status.getCurrentBlockNum(),status.getNumBlocks()))
#		try:
#			
		self.elapsedtime.set_text("%0.1f s" % self.elapsed)
		self.numiterations.set_text(str(status.getIterationNum()))
		self.blockvars.set_text(str(status.getCurrentBlockSize()))
		self.blockiterations.set_text(str(status.getCurrentBlockIteration()))
		self.blockresidual.set_text("%8.5e" % status.getBlockResidualRMS())
		self.blockelapsedtime.set_text("%0.1f s" % self.blocktime)

#			_frac = self.status.getNumConverged() / self.numvars()
#			self.progressbar.set_text("%d vars converged..." % self.status.getNumConverged())
#			self.progressbar.set_fraction(_frac)
#			print "TRYING..."
#		except RuntimeError,e:
#			print "ERROR OF SOME SORT"
		_frac = float(status.getNumConverged()) / self.nv
		print "FRACTION = ",_frac
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
			print "UPDATING!"
			self.fill_values(status)

		while gtk.events_pending():
			gtk.main_iteration()		

		if status.isConverged() or status.isDiverged() or status.isInterrupted():
			return 1
		if self.guiinterrupt:
			return 2
		return 0

	def finalise(self,status):
		_p = self.browser.prefs;
		_close_on_converged = _p.getBoolPref("SolverReporter","close_on_converged");

		if status.isConverged() and _close_on_converged:
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
		
			
