import gtk
import ascpy
import time

class PythonSolverReporter(ascpy.SolverReporter):
	def __init__(self,browser,message=None):
		self.browser=browser
		self.reporter = self.browser.reporter
		self.starttime = time.clock()
		ascpy.SolverReporter.__init__(self)

class PopupSolverReporter(PythonSolverReporter):
	def __init__(self,browser,numvars):
		PythonSolverReporter.__init__(self,browser)
		self.progress_dialog = gtk.Dialog("Solver Status")

		
		self.numvars = gtk.Label()
		self.numblocks = gtk.Label()
		self.elapsedtime = gtk.Label()
		self.numiterations = gtk.Label()
	
		self.progressbar = gtk.ProgressBar()
		self.progress_dialog.vbox.pack_start(self.numvars)
		self.progress_dialog.vbox.pack_start(self.numblocks)
		self.progress_dialog.vbox.pack_start(self.elapsedtime)
		self.progress_dialog.vbox.pack_start(self.numiterations)
		self.progress_dialog.vbox.pack_start(self.progressbar)
		
		self.closebutton = self.progress_dialog.add_button(gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE)
		self.stopbutton = self.progress_dialog.add_button(gtk.STOCK_QUIT, gtk.RESPONSE_CANCEL)
		self.closebutton.connect("clicked", self.on_progressdialog_close)
		self.stopbutton.connect("clicked",self.on_progressdialog_stop)
		self.closebutton.grab_default()
		
		self.progress_dialog.show_all()
		self.solvedvars = 0
		self.lasttime = 0
		self.blockstart = self.starttime
		self.blocktime = 0;
		self.elapsed = 0
		self.blocknum = 0
		self.guiinterrupt = False
		self.guitime = 0

		self.nv = numvars

		while gtk.events_pending():
			gtk.main_iteration()
			
	def on_progressdialog_stop(self,*args):
		self.guiinterrupt = True;

	def on_progressdialog_close(self,widget):
		self.progress_dialog.destroy()

		
	def fill_values(self,status):
		self.numblocks.set_text("Total Blocks : %d of %d" % (status.getCurrentBlockNum(),status.getNumBlocks()))
		self.numvars.set_text("Converged : %d of %d" % (status.getNumConverged(), self.nv))
		self.elapsedtime.set_text("Time Elapsed : %0.1f s" % self.elapsed)
		self.numiterations.set_text("Iterations done : "+str(status.getIterationNum()))
		_frac = float(status.getNumConverged()) / self.nv
		self.progressbar.set_text("%d vars converged..." % status.getNumConverged());
		self.progressbar.set_fraction(_frac)

	def report(self,status):
		_time = time.clock();
		_sincelast = _time - self.lasttime
		if status.getCurrentBlockNum() > self.blocknum:
			self.blocknum = status.getCurrentBlockNum()
			self.blockstart = _time
		if self.lasttime==0 or status.isConverged():
			self.lasttime = _time;
			self.elapsed = _time - self.starttime
			self.blocktime = _time - self.blockstart
			print "UPDATING!"
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
		try:
			_time = time.clock()

			self.fill_values(status)

			if status.isConverged():
				self.progressbar.set_fraction(1.0)
				self.progressbar.set_text("Converged")
				self.browser.reporter.reportNote( "Solving successful : Canvasmodel Converged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Solved'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.hasExceededTimeLimit():
				self.progressbar.set_text("Exceeded time limit")
				self.browser.reporter.reportError(" Solving failed : Solver exceeded time limit ")
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.hasExceededIterationLimit():
				self.progressbar.set_text("Exceeded iteration limit")
				self.browser.reporter.reportError( " Solving failed : Solver exceeded iteration limit " )
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.isDiverged():
				self.progressbar.set_text("Diverged")
				self.browser.reporter.reportError("Solving failed : Canvasmodel Diverged ! ")
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.isOverDefined():
				self.progressbar.set_text("Over-defined")
				self.browser.reporter.reportError(" Solving failed : Canvasmodel over-defined ! ")
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.isUnderDefined():
				self.progressbar.set_text("Under-defined")
				self.browser.reporter.reportError(" Solving failed : Canvasmodel under-defined ! ")
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
			elif status.hasResidualCalculationErrors():
				self.progressbar.set_text("Residual Calculation Error")
				self.browser.reporter.reportError(" Solving failed : Solver had residual calculation errors ")
				self.browser.reporter.reportNote( " Canvasmodel Diverged ! " )
				self.browser.view.canvas.canvasmodelstate = 'Diverged'
				self.browser.status.push(0,"CanvasModel State :: %s" % self.browser.view.canvas.canvasmodelstate)
					
			self.closebutton.set_sensitive(True)
			self.stopbutton.set_sensitive(False)
			
			self.guitime = self.guitime + (time.clock() - _time)
			print "TIME SPENT UPDATING SOLVER: %0.2f s" % self.guitime
		except Exception,e:
			print "SOME PROBLEM: %s" % str(e)
		
