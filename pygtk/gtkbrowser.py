#!/usr/bin/env python

import sys
def print_loading_status(status,msg=None):
	sys.stderr.write("\r                                                 \r")
	if msg!=None:
		sys.stderr.write(msg+"\n")
	sys.stderr.write(status+"...\r")
	sys.stderr.flush()

try:
	#print_loading_status("Loading PSYCO")
	#try:
	#	import psyco
	#	psyco.full()
	#	print "Running with PSYCO optimisation..."
	#except ImportError:
	#	pass


	print_loading_status("Loading python standard libraries")

	import re
	import urlparse
	import optparse
	import platform
	import sys
	import os.path

	if platform.system() != "Windows":
		import dl
		# This sets the flags for dlopen used by python so that the symbols in the
		# ascend library are made available to libraries dlopened within ASCEND:
		sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

	print_loading_status("Loading LIBASCEND/ascpy")
	import ascpy

	print_loading_status("Loading PyGTK, glade, pango")

	import pygtk 
	pygtk.require('2.0') 
	import gtk
	import gtk.glade
	import pango

	print_loading_status("Loading python matplotlib")
	try:
		import matplotlib
		matplotlib.use('GTKAgg')

		try:
			print_loading_status("Trying python numarray")
			import numarray
			matplotlib.rcParams['numerix'] = 'numarray'  
			print_loading_status("","Using python module numarray")
		except ImportError:
			try:
				print_loading_status("Trying python numpy")
				import numpy
				matplotlib.rcParams['numerix'] = 'numpy'  
				print_loading_status("","Using python module numpy")
			except ImportError:
				try:
					print_loading_status("Trying python Numeric")
					import Numeric
					matplotlib.rcParams['numerix'] = 'Numeric'  
					print_loading_status("","Using python module Numeric")
				except ImportError:
					print_loading_status("","FAILED TO LOAD A NUMERIC MODULE FOR PYTHON")

	except ImportError,e:
		print_loading_status("","FAILED TO LOAD MATPLOTLIB")
		raise RuntimeError("Failed to load MATPLOTLIB (is it installed?). Details:"+str(e))

	print_loading_status("Loading IPython")
	import console;
	if not console.have_ipython:
		print_loading_status("","IPython couldn't be loaded")

	print_loading_status("Loading ASCEND python modules")
	from preferences import *      # loading/saving of .ini options
	from solverparameters import * # 'solver parameters' window
	from help import *             # viewing help files
	from incidencematrix import *  # incidence/sparsity matrix matplotlib window
	from observer import *         # observer tab support
	from properties import *       # solver_var properties dialog
	from varentry import *         # for inputting of variables with units
	from diagnose import * 	       # for diagnosing block non-convergence
	from solverreporter import *   # solver status reporting
	from modelview import *        # model browser
	from integrator import *       # integrator dialog	
	from infodialog import *       # general-purpose textual information dialog
	import config

except RuntimeError, e:
	print "ASCEND had problems starting up. Please report the following"
	print "error message at http://mantis.cruncher2.dyndns.org/."
	print "\n\nFull error message:",str(e)
	print "\n\nPress ENTER to close this window."
	sys.stdout.flush()
	sys.stdin.readline();
	sys.exit();

except ImportError, e:
	print "\n\n------------------  ERROR  ---------------------"
	print     "ASCEND had problems importing required models."
	print "\nPlease ensure you have all the runtime prerequisites installed."
	print "Please then report a bug if you continue to have problems."
	print "\nFull error message:",str(e)
	if platform.system()=="Windows":
		print "\nYou will also need to report the contents of any popup error"
		print "messages from Windows if any were shown."
	print "\n\nPress ENTER to close this window."
	sys.stdout.flush()
	sys.stdin.readline();
	sys.exit();

print_loading_status("Starting GUI")

# This is my first ever GUI code so please be nice :)
# But I *have* at least read 
# http://www.joelonsoftware.com/uibook/chapters/fog0000000057.html
# and leafed through
# http://developer.gnome.org/projects/gup/hig/

# The fancy tree-view gizmo is the GtkTreeView object. See the article
# http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/300304
# for the original source code on which my implementation was based.

ESCAPE_KEY = 65307

HELP_ROOT = None

#======================================
# Browser is the main ASCEND library/model browser window

class Browser:

#   ---------------------------------
#   SETUP

	def __init__(self,librarypath=None,assetspath=None):

		if assetspath==None:
			assetspath=config.PYGTK_ASSETS

		#--------
		# load the file referenced in the command line, if any

		print_loading_status("Parsing options","CONFIG = %s"%config.VERSION)
		
		parser = optparse.OptionParser(usage="%prog [[-m typename] file]", version="gtkbrowser $rev$" )
		# add options here if we want

		parser.add_option("-m", "--model"
			,action="store", type="string", dest="model"
			,help="specify the model to instantiate upon loading modules")		

		parser.add_option("--pygtk-assets"
			,action="store", type="string", dest="assets_dir"
			,help="override the configuration value for the location of assets"\
				+" required by PyGTK for the ASCEND GUI, optional"
			,default=assetspath
		)

		parser.add_option("--library"
			,action="store", type="string", dest="library_path"
			,help="override the configuration value for the library path"
			,default=librarypath
		)

		parser.add_option("--no-auto-sim"
			,action="store_false", dest="auto_sim"
			,help="disable auto-instantiation of MODEL named as the file stem"
			,default=True
		)

		(self.options, args) = parser.parse_args()

		#print "OPTIONS_______________:",self.options

		self.assets_dir = self.options.assets_dir
		
		self.observers = []
		self.clip = None

		#--------
		# load up the preferences ini file

		print_loading_status("Loading preferences")

		self.prefs = Preferences()

		_prefpath = self.prefs.getStringPref("Directories","librarypath",None)
		_preffileopenpath = self.prefs.getStringPref("Directories","fileopenpath",None)

		#--------
		# set up library path and the path to use for File->Open dialogs
		
		if self.options.library_path != None:
			_path = os.path.abspath(self.options.library_path)
			_pathsrc = "command line options"
			# when a special path is specified, use that as the file-open location
			self.fileopenpath = _path
		else:
			if _prefpath:
				_path = _prefpath
				_pathsrc = "user preferences"
			else:
				_path = config.LIBRARY_PATH
				_pathsrc = "default (config.py)"
			
			if _preffileopenpath:
				self.fileopenpath = _preffileopenpath
			else:
				self.fileopenpath = _path
					
		#--------
		# Create the ASCXX 'Library' object
		
		print_loading_status("Creating ASCEND 'Library' object","PATH = "+_path+" FROM "+_pathsrc)
		self.library = ascpy.Library(_path)

		self.sim = None

		#-------------------
		# Set up the window and main widget actions

		self.glade_file = os.path.join(self.assets_dir,config.GLADE_FILE)

		print_loading_status("Setting up windows") #,"GLADE_FILE = %s" % self.glade_file)

		glade = gtk.glade.XML(self.glade_file,"browserwin")

		self.window = glade.get_widget("browserwin")


		if not self.window:
			raise RuntimeError("Couldn't load window from glade file")

		_display = self.window.get_screen().get_display().get_name()
		_geom=self.prefs.getGeometrySizePosition(_display,"browserwin")
		if _geom:
			self.window.resize(_geom[0],_geom[1])
			self.window.move(_geom[2],_geom[3])
		
		self.window.connect("delete_event", self.delete_event)

		self.browserpaned=glade.get_widget("browserpaned")
		_geom2=self.prefs.getGeometryValue(_display,"browserpaned")
		if _geom2:
			self.browserpaned.set_position(_geom2)

		self.openbutton=glade.get_widget("openbutton")
		self.openbutton.connect("clicked",self.open_click)

		self.reloadbutton=glade.get_widget("reloadbutton")
		self.reloadbutton.connect("clicked",self.reload_click)
		
		self.solvebutton=glade.get_widget("solvebutton")
		self.solvebutton.connect("clicked",self.solve_click)

		self.integratebutton=glade.get_widget("integratebutton")
		self.integratebutton.connect("clicked",self.integrate_click)

		self.checkbutton=glade.get_widget("checkbutton")
		self.checkbutton.connect("clicked",self.check_click)

		self.autotoggle=glade.get_widget("autotoggle")
		self.automenu = glade.get_widget("automenu")
		self.autotoggle.connect("toggled",self.auto_toggle)

		self.methodrunbutton=glade.get_widget("methodrunbutton")
		self.methodrunbutton.connect("clicked",self.methodrun_click)

		self.methodsel=glade.get_widget("methodsel")

		self.maintabs = glade.get_widget("maintabs")

		self.statusbar = glade.get_widget("statusbar")

		self.menu = glade.get_widget("browsermenu")

		self.show_solving_popup=glade.get_widget("show_solving_popup")
		self.show_solving_popup.set_active(self.prefs.getBoolPref("SolverReporter","show_popup",True))
		self.close_on_converged=glade.get_widget("close_on_converged")
		self.close_on_converged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_converged",True))
		self.close_on_nonconverged=glade.get_widget("close_on_nonconverged")
		self.close_on_nonconverged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_nonconverged",True))
		self.solver_engine=glade.get_widget("solver_engine")

		self.use_relation_sharing=glade.get_widget("use_relation_sharing")
		self.use_relation_sharing.set_active(self.prefs.getBoolPref("Compiler","use_relation_sharing",True))

		glade.signal_autoconnect(self)

		#-------
		# Status icons

		self.fixedimg = gtk.Image()
		self.fixedimg.set_from_file(os.path.join(self.options.assets_dir,'locked.png'))

		self.iconstatusunknown = None
		self.iconfixed = self.fixedimg.get_pixbuf()
		self.iconsolved = self.window.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconactive = self.window.render_icon(gtk.STOCK_NO,gtk.ICON_SIZE_MENU)
		self.iconunsolved = None

		self.statusicons={
			ascpy.ASCXX_VAR_STATUS_UNKNOWN: self.iconstatusunknown
			,ascpy.ASCXX_VAR_FIXED: self.iconfixed
			,ascpy.ASCXX_VAR_SOLVED: self.iconsolved
			,ascpy.ASCXX_VAR_ACTIVE: self.iconactive
			,ascpy.ASCXX_VAR_UNSOLVED: self.iconunsolved
		}


		self.statusmessages={
			ascpy.ASCXX_VAR_STATUS_UNKNOWN: "Status unknown"
			,ascpy.ASCXX_VAR_FIXED: "Fixed"
			,ascpy.ASCXX_VAR_SOLVED: "Converged"
			,ascpy.ASCXX_VAR_ACTIVE: "Active (unconverged)"
			,ascpy.ASCXX_VAR_UNSOLVED: "Not yet visited"
		}

		#-------------------
		# waitwin

		self.waitwin = gtk.gdk.Window(self.window.window,
			gtk.gdk.screen_width(),
			gtk.gdk.screen_height(),
			gtk.gdk.WINDOW_CHILD,
			0,
			gtk.gdk.INPUT_ONLY)

		_cursor = gtk.gdk.Cursor(gtk.gdk.WATCH)
		self.waitwin.set_cursor(_cursor)

		#-------------------
		# pixbufs to be used in the error listing

		self.iconok = self.window.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconinfo = self.window.render_icon(gtk.STOCK_DIALOG_INFO,gtk.ICON_SIZE_MENU)
		self.iconwarning = self.window.render_icon(gtk.STOCK_DIALOG_WARNING,gtk.ICON_SIZE_MENU)
		self.iconerror = self.window.render_icon(gtk.STOCK_DIALOG_ERROR,gtk.ICON_SIZE_MENU)

		#--------------------
		# pixbufs for solver_var status

		#--------------------
		# set up the error view

		self.errorview = glade.get_widget("errorview")	
		errstorecolstypes = [gtk.gdk.Pixbuf,str,str,str,int]
		self.errorstore = gtk.TreeStore(*errstorecolstypes)
		errtitles = ["","Location","Message"];
		self.errorview.set_model(self.errorstore)
		self.errcols = [ gtk.TreeViewColumn() for _type in errstorecolstypes]

		i = 0
		for tvcolumn in self.errcols[:len(errtitles)]:
			tvcolumn.set_title(errtitles[i])
			self.errorview.append_column(tvcolumn)			

			if i>0:
				_renderer = gtk.CellRendererText()
				tvcolumn.pack_start(_renderer, True)				
				tvcolumn.add_attribute(_renderer, 'text', i)
				if(i==2):
					tvcolumn.add_attribute(_renderer, 'foreground', 3)
					tvcolumn.add_attribute(_renderer, 'weight', 4)
			else:
				_renderer1 = gtk.CellRendererPixbuf()
				tvcolumn.pack_start(_renderer1, False)				
				tvcolumn.add_attribute(_renderer1, 'pixbuf', int(0))

			i = i + 1


		#--------------------
		# set up the error reporter callback
		self.reporter = ascpy.getReporter()
		self.reporter.setPythonErrorCallback(self.error_callback)


		#-------
		# Solver engine list

		_slvlist = ascpy.getSolvers()
		self.solver_engine_menu = gtk.Menu()
		self.solver_engine_menu.show()
		self.solver_engine.set_submenu(self.solver_engine_menu)
		self.solver_engine_menu_dict = {}
		_fmi = None
		for _s in _slvlist:
			_mi = gtk.RadioMenuItem(_fmi,_s.getName(),False)
			if _fmi==None:
				_fmi = _mi
			_mi.show()
			_mi.connect('toggled',self.on_select_solver_toggled,_s.getName())
			self.solver_engine_menu.append(_mi)
			self.solver_engine_menu_dict[_s.getName()]=_mi	
		
		_pref_solver = self.prefs.getStringPref("Solver","engine","QRSlv")
		_mi = self.solver_engine_menu_dict.get(_pref_solver)
		if _mi:
			_mi.set_active(1)

		#--------
		# Assign an icon to the main window

		self.icon = None
		if config.ICON_EXTENSION:
			_iconpath = ""
			try:
				_icon = gtk.Image()
				_iconpath = os.path.join(self.assets_dir,'ascend'+config.ICON_EXTENSION)
				_icon.set_from_file(_iconpath)
				_iconpbuf = _icon.get_pixbuf()
				self.window.set_icon(_iconpbuf)
				self.icon = _iconpbuf
			except Exception, e:
				print "FAILED TO SET APPLICATION ICON PATH '%s': %s" % (_iconpath,str(e))
				self.reporter.reportError("FAILED to set application icon '%s': %s"
					 % (_iconpath,str(e)) 
				)

		#-------------------
		# set up the module view

		self.modtank = {}
		self.moduleview = glade.get_widget("moduleview")
		modulestorecoltypes = [str, str, int] # bool=can-be-instantiated
		self.modulestore = gtk.TreeStore(*modulestorecoltypes)
		moduleviewtitles = ["Module name", "Filename"]
		self.moduleview.set_model(self.modulestore)
		self.modcols = [ gtk.TreeViewColumn() for _type in modulestorecoltypes]
		i = 0
		for modcol in self.modcols[:len(moduleviewtitles)]:
			modcol.set_title(moduleviewtitles[i])
			self.moduleview.append_column(modcol)
			_renderer = gtk.CellRendererText()
			modcol.pack_start(_renderer, True)
			modcol.add_attribute(_renderer, 'text', i)
			modcol.add_attribute(_renderer,'weight',2)
			i = i + 1
		self.moduleview.connect("row-activated", self.module_activated )
	
		#--------------------
		# set up the methods combobox

		self.methodstore = gtk.ListStore(str)
		self.methodsel.set_model(self.methodstore)
		_methodrenderer = gtk.CellRendererText()
		self.methodsel.pack_start(_methodrenderer, True)
		self.methodsel.add_attribute(_methodrenderer, 'text',0)

		#--------
		# set up the instance browser view

		self.modelview = ModelView(self, glade)

		#--------
		# set up the tabs
		self.tabs = {}
		self.activetab = None # most recent observer tab

		#--------
		# set the state of the 'auto' toggle

		self.is_auto = self.prefs.getBoolPref("Browser","auto_solve",True)
		self.autotoggle.set_active(self.is_auto)
		self.automenu.set_active(self.is_auto)

		#--------
		# tell libascend about this 'browser' object

		print dir(ascpy.Registry())
		ascpy.Registry().set("browser",self)

		#--------
		# options

		if(len(args)==1):
			self.do_open(args[0])

			print "Options: ",self.options

			_model = None
			if self.options.model:
				_model = self.options.model
				print "MODEL: '%s'" % _model
			elif self.options.auto_sim:
				_head, _tail = os.path.split(args[0])
				if(_tail):
					_model, _ext = os.path.splitext(_tail)

			if _model:
				try:
					_t=self.library.findType(_model)
					try:
						self.do_sim(_t)
						if not self.options.model:
							self.reporter.reportNote("Instantiating self-titled model '%s'" %_model)
					except RuntimeError, e:
						self.reporter.reportError("Failed to create instance of '%s': %s" 
							%(_model, str(e))
						);
				except RuntimeError, e:
					if self.options.model:
						self.reporter.reportError("Unknown model type '%s': %s" 
							%(_model, str(e))
						);		

	def run(self):
		self.window.show()
		print_loading_status("ASCEND is now running")
		gtk.main()

#   ------------------
#   SOLVER LIST

	def set_solver(self,solvername):
		self.solver = ascpy.Solver(solvername)
		self.prefs.setStringPref("Solver","engine",solvername)
		self.reporter.reportNote("Set solver engine to '%s'" % solvername)

#   --------------------------------------------
# 	MAJOR GUI COMMANDS

	def on_fix_variable_activate(self,*args):
		self.modelview.on_fix_variable_activate(*args)

	def on_free_variable_activate(self,*args):
		self.modelview.on_free_variable_activate(*args)

	def on_select_solver_toggled(self,widget,solvername):
		if widget.get_active():
			self.set_solver(solvername)

	def do_open(self,filename):
		# TODO does the user want to lose their work?
		# TODO do we need to chdir?

		_context = self.statusbar.get_context_id("do_open")

		self.errorstore.clear()
		self.modelview.clear()
	
		# self.library.clear()

		print "Filename =",filename
		self.statusbar.push(_context,"Loading '"+filename+"'")
		self.library.load(filename)
		print "Statusbar =",self.statusbar
		try:
			self.statusbar.pop(_context)
		except TypeError,e:
			print "For some reason, a type error (context=%s,filename=%s): %s" % (_context,filename,e)

		self.filename = filename

		# Load the current list of modules into self.modules
		self.modtank = {}
		self.modulestore.clear()
		modules = self.library.getModules()
		#self.library.listModules()
		try:
			_lll=len(modules)
		except:
			_msg = "UNABLE TO ACCESS MODULES LIST. This is bad.\n"+\
			"Check your SWIG configuration (check for warnings during build)."
			
			self.reporter.reportError(_msg)
			raise RuntimeError(_msg)
			
		for m in reversed(modules):
			_n = str( m.getName() )
			_f = str( m.getFilename() )
			#print "ADDING ROW name %s, file = %s" % (_n, _f)
			_r = self.modulestore.append(None,  [ _n, _f, pango.WEIGHT_NORMAL ])
			for t in self.library.getModuleTypes(m):
				_n = t.getName()
				_hasparams = t.hasParameters()
				if _hasparams:
					_w = pango.WEIGHT_NORMAL
				else:
					_w = pango.WEIGHT_BOLD
				
				#print "ADDING TYPE %s" % _n
				_piter = self.modulestore.append(_r , [ _n, "", _w ])
				_path = self.modulestore.get_path(_piter)
				self.modtank[_path]=t

		#print "DONE ADDING MODULES"

		self.sim = None;
		self.maintabs.set_current_page(0);
	
	# See http://www.daa.com.au/pipermail/pygtk/2005-October/011303.html
	# for details on how the 'wait cursor' is done.
	def start_waiting(self, message):
		self.waitcontext = self.statusbar.get_context_id("waiting")
		self.statusbar.push(self.waitcontext,message)

		if self.waitwin:
			self.waitwin.show()

		while gtk.events_pending():
			gtk.main_iteration()
		
	def stop_waiting(self):
		if self.waitwin:
			self.statusbar.pop(self.waitcontext)
			self.waitwin.hide()
		
	def do_sim(self, type_object):
		self.sim = None;
		# TODO: clear out old simulation first!

		print "DO_SIM(%s)" % str(type_object.getName())		
		self.start_waiting("Compiling...")

		try:
			_v = self.prefs.getBoolPref("Compiler","use_relation_sharing",True)
			ascpy.getCompiler().setUseRelationSharing(_v)

			self.sim = type_object.getSimulation(str(type_object.getName())+"_sim")
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return

		print "...DONE 'getSimulation'"		
		self.stop_waiting()

		self.start_waiting("Building simulation...")
		print "BUILDING SIMULATION"

		try:
			self.sim.build()
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return;

		print "DONE BUILDING"
		self.stop_waiting()

		self.sim.setSolver(self.solver)

		# methods
		self.methodstore.clear()
		_methods = self.sim.getType().getMethods()
		_activemethod = None;
		for _m in _methods:
			_i = self.methodstore.append([_m.getName()])
			if _m.getName()=="on_load":
				self.methodsel.set_active_iter(_i)

		self.modelview.setSimulation(self.sim)
	
	def do_solve_if_auto(self):
		if self.is_auto:
			self.sim.checkInstance()
			self.do_solve()
		else:
			self.sim.processVarStatus()
			self.modelview.refreshtree()

		self.sync_observers()
		
	def do_solve(self):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return

		self.start_waiting("Solving with %s..." % self.solver.getName())

		if self.prefs.getBoolPref("SolverReporter","show_popup",True):
			reporter = PopupSolverReporter(self,self.sim.getNumVars())
		else:
			reporter = SimpleSolverReporter(self)

		self.sim.solve(self.solver,reporter)

		self.stop_waiting()
		
		self.sim.processVarStatus()
		self.modelview.refreshtree()

	def do_integrate(self):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return
		integwin = IntegratorWindow(self,self.sim)		
		_integratorreporter = integwin.run()
		if _integratorreporter!=None:
			_integratorreporter.run()
			self.sim.processVarStatus()
			self.modelview.refreshtree()
		

	def do_check(self):
		if not self.sim:
			self.reporter.reportError("No simulation yet")
			return

		self.start_waiting("Checking system...")

		try:
			self.sim.checkInstance()
			self.reporter.reportWarning("System instance check run, check above for error (if any).")
			# the above gives output but doesn't throw errors or return a status.
			# ... this is a problem (at the C level)

			status = self.sim.checkDoF()
			if status==ascpy.ASCXX_DOF_UNDERSPECIFIED:
				self.on_show_fixable_variables_activate(None)
			elif status==ascpy.ASCXX_DOF_OVERSPECIFIED:
				self.on_show_freeable_variables_activate(None)
			elif status==ascpy.ASCXX_DOF_STRUCT_SINGULAR:
				if not self.sim.checkStructuralSingularity():
					sing = self.sim.getSingularityInfo()
					title = "Structural singularity"
					text = title
					text += "\n\nThe singularity can be reduced by freeing the following variables:"
					msgs = {
						"The singularity can be reduced by freeing the following variables" : sing.freeablevars
						,"Relations involved in the structural singularity" : sing.rels
						,"Variables involved in the structural singularity" : sing.vars
					}
					for k,v in msgs.iteritems():
						text+="\n\n%s:" % k
						if len(v):
							_l = [j.getName() for j in v]
							_l.sort()
							text+= "\n\t" + "\n\t".join(_l)
						else:
							text += "\nnone"

					_dialog = InfoDialog(self,self.window,text,title)
					_dialog.run()

			self.reporter.reportNote("System DoF check OK")

		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return

		self.stop_waiting()

		self.modelview.refreshtree()

	def do_method(self,method):
		if not self.sim:
			self.reporter.reportError("No model selected yet")

		self.sim.run(method)
		self.modelview.refreshtree()

	def do_quit(self):
		print_loading_status("Saving window location")		
		self.reporter.clearPythonErrorCallback()

		_w,_h = self.window.get_size()
		_t,_l = self.window.get_position()
		_display = self.window.get_screen().get_display().get_name()
		self.prefs.setGeometrySizePosition(_display,"browserwin",_w,_h,_t,_l );

		_p = self.browserpaned.get_position()
		self.prefs.setGeometryValue(_display,"browserpaned",_p);

		print_loading_status("Saving current directory")			
		self.prefs.setStringPref("Directories","fileopenpath",self.fileopenpath)

		self.prefs.setBoolPref("Browser","auto_solve",self.is_auto)

		print_loading_status("Saving preferences")
		# causes prefs to be saved unless they are still being used elsewher
		del(self.prefs)

		print_loading_status("Closing down GTK")
		gtk.main_quit()

		print_loading_status("Clearing error callback")		
		self.reporter.clearPythonErrorCallback()
		
		print_loading_status("Quitting")
		return False

	def on_tools_sparsity_click(self,*args):

		self.reporter.reportNote("Preparing incidence matrix...")
		_im = self.sim.getIncidenceMatrix();

		self.reporter.reportNote("Plotting incidence matrix...")

		_sp = IncidenceMatrixWindow(_im);
		_sp.run();

	def on_diagnose_blocks_click(self,*args):
		try:
			_bl = self.sim.getActiveBlock()
		except RuntimeError, e:
			self.reporter.reportError(str(e))
			return
		_db = DiagnoseWindow(self,_bl)
		_db.run();

	def on_add_observer_click(self,*args):
		self.create_observer()

	def on_keep_observed_click(self,*args):
		print "KEEPING..."
		if len(self.observers) <= 0:
			self.reporter.reportError("No observer defined!")
			return
		self.tabs[self.currentobservertab].do_add_row()

	def on_copy_observer_matrix_click(self,*args):
		if self.clip == None:
			self.clip = gtk.Clipboard()

		if len(self.observers) <= 0:
			self.reporter.reportError("No observer defined!")
			return
		self.tabs[self.currentobservertab].copy_to_clipboard(self.clip)

	def on_use_relation_sharing_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("Compiler","use_relation_sharing",_v)
		self.reporter.reportNote("Relation sharing set to "+str(_v))

	def on_show_solving_popup_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","show_popup",_v)
		print "SET TO",_v
		
	def on_close_on_converged_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","close_on_converged",_v)

	def on_close_on_nonconverged_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","close_on_nonconverged",_v)

	def on_show_variables_near_bounds_activate(self,*args):
		_epsilon = 1e-4;
		text = "Variables Near Bounds"
		title=text;
		text += "\n"
		_vars = self.sim.getVariablesNearBounds(_epsilon)
		if len(_vars):
			for _v in _vars:
				text += "\n%s"%_v.getName()
		else:
			text +="\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

#   --------------------------------------------
#   MODULE LIST

	def module_activated(self, treeview, path, column, *args):
		modules = self.library.getModules()
		print "PATH",path
		if len(path)==1:
			self.reporter.reportNote("Launching of external editor not yet implemented")
		elif len(path)==2:
			if(self.modtank.has_key(path)):
				_type = self.modtank[path];
				self.reporter.reportNote("Creating simulation for type %s" % str(_type.getName()) )
				self.do_sim(_type)
			else:
				self.reporter.reportError("Didn't find type corresponding to row")

#   ----------------------------------
#   ERROR PANEL

	def get_error_row_data(self,sev,filename,line,msg):
		_sevicon = {
			0: self.iconok
			,1: self.iconinfo
			,2: self.iconwarning
			,3: self.iconerror
			,4: self.iconinfo
			,5: self.iconwarning
			,6: self.iconerror
		}[sev]

		_fontweight = pango.WEIGHT_NORMAL
		if sev==6:
			_fontweight = pango.WEIGHT_BOLD
		
		_fgcolor = "black"
		if sev==4:
			_fgcolor = "#888800"
		elif sev==5:
			_fgcolor = "#884400"
		elif sev==6:
			_fgcolor = "#880000"
		elif sev==0:
			_fgcolor = BROWSER_FIXED_COLOR
		
		if not filename and not line:
			_fileline = ""
		else:
			if(len(filename) > 25):
				filename = "..."+filename[-22:]
			_fileline = filename + ":" + str(line)

		_res = [_sevicon,_fileline,msg.rstrip(),_fgcolor,_fontweight]
		#print _res
		return _res  

	def error_callback(self,sev,filename,line,msg):
		try:
			pos = self.errorstore.append(None, self.get_error_row_data(sev, filename,line,msg))
			path = self.errorstore.get_path(pos)
			col = self.errorview.get_column(3)
			self.errorview.scroll_to_cell(path,col)
		except Exception,e:
			print "UNABLE TO DISPLAY ERROR MESSAGE '%s'"%msg
		
		return 0;

#   --------------------------------
#   BUTTON METHODS

	def open_click(self,*args):
		#print_loading_status("CURRENT FILEOPENPATH is",self.fileopenpath)
		dialog = gtk.FileChooserDialog("Open ASCEND model...",
			self.window,
			gtk.FILE_CHOOSER_ACTION_OPEN,
			(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK)
		)
		dialog.set_current_folder(self.fileopenpath)
		dialog.set_default_response(gtk.RESPONSE_OK)
		dialog.set_transient_for(self.window)
		dialog.set_modal(True)

		filter = gtk.FileFilter()
		filter.set_name("*.a4c, *.a4l")
		filter.add_pattern("*.[Aa]4[Cc]")
		filter.add_pattern("*.[Aa]4[Ll]")
		dialog.add_filter(filter)

		filter = gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		response = dialog.run()
		_filename = dialog.get_filename()
		print "\nFILENAME SELECTED:",_filename
		
		_path = dialog.get_current_folder()
		if _path:
			self.fileopenpath = _path
		
		dialog.hide()

		if response == gtk.RESPONSE_OK:
			self.reporter.reportNote("File %s selected." % dialog.get_filename() )
			self.library.clear()
			self.do_open( _filename)		   

	def reload_click(self,*args):
		_type = None
		if(self.sim):
			_type = self.sim.getType().getName().toString();

		self.library.clear()
		self.do_open(self.filename)
		
		if _type:
			_t = self.library.findType(_type)
			self.do_sim(_t)	

	def props_activate(self,widget,*args):
		return self.modelview.props_activate(self,widget,*args)

	def observe_activate(self,widget,*args):
		return self.modelview.observe_activate(self,widget,*args)

	def solve_click(self,*args):
		#self.reporter.reportError("Solving simulation '" + self.sim.getName().toString() +"'...")
		self.do_solve()

	def console_click(self,*args):
		try:
			console.start(self)
		except RuntimeError,e:
			self.reporter.reportError("Unable to start console: "+str(e));

	def integrate_click(self,*args):
		self.do_integrate()
	
	def check_click(self,*args):
		self.do_check()
		#self.reporter.reportError("CHECK clicked")

	def preferences_click(self,*args):
		if not self.sim:
			self.reporter.reportError("No simulation created yet!");
		
		_paramswin = SolverParametersWindow(self)
		_paramswin.show()

	def methodrun_click(self,*args):
		_sel = self.methodsel.get_active_text()
		if _sel:
			_method = None
			_methods = self.sim.getType().getMethods()
			for _m in _methods:
				if _m.getName()==_sel:
					_method = _m
			if not _method:
				self.reporter.reportError("Method is not valid")
				return
			self.do_method(_method)
		else:
			self.reporter.reportError("No method selected")
	
	def auto_toggle(self,button,*args):
		self.is_auto = button.get_active()
		if hasattr(self,'automenu'):
			self.automenu.set_active(self.is_auto)
		else:
			raise RuntimeError("no automenu")

		#if self.is_auto:
		#	self.reporter.reportSuccess("Auto mode is now ON")
		#else:
		#	self.reporter.reportSuccess("Auto mode is now OFF")

	def on_file_quit_click(self,*args):
		self.do_quit()

	def on_tools_auto_toggle(self,checkmenuitem,*args):
		self.is_auto = checkmenuitem.get_active()
		self.autotoggle.set_active(self.is_auto)

	def on_help_about_click(self,*args):
		_xml = gtk.glade.XML(self.glade_file,"aboutdialog")
		_about = _xml.get_widget("aboutdialog")
		_about.set_position(gtk.WIN_POS_CENTER_ON_PARENT)
		_about.set_transient_for(self.window);
		_about.set_version(config.VERSION)
		_about.run()
		_about.destroy()

	def on_help_contents_click(self,*args):
		_help = Help(HELP_ROOT)
		_help.run()

	def on_show_fixable_variables_activate(self,*args):
		v = self.sim.getFixableVariables()
		text = "Fixable Variables"
		title = text
		text += "\n"
		if len(v):
			for var in v:
				text += "\n%s"%var
		else:
			text += "\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_freeable_variables_activate(self,*args):
		v = self.sim.getFreeableVariables()

		text = "Freeable Variables"
		title = text
		text += "\n"
		if len(v):
			for var in v:
				text += "\n%s" % var
		else:
			text += "\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()
		
	def on_show_external_functions_activate(self,*args):
		v = self.library.getExtMethods()
		text = "External Functions"
		title = text
		text +="\nHere is the list of external functions currently present in"
		text +=" the Library:"

		if len(v):
			for ext in v:
				text += "\n\n%s (%d inputs, %d outputs):" % \
					(ext.getName(), ext.getNumInputs(), ext.getNumOutputs())
				text += "\n%s" % ext.getHelp()
		else:
			text +="\n\nNone"		
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_maintabs_switch_page(self,notebook,page,pagenum):
		print("Page switched to %d" % pagenum)
		if pagenum in self.tabs.keys():
			self.currentobservertab = pagenum

	def create_observer(self,name=None):
		_xml = gtk.glade.XML(self.glade_file,"observervbox");
		_label = gtk.Label();
		_tab = self.maintabs.append_page(_xml.get_widget("observervbox"),_label);
		_obs = ObserverTab(xml=_xml, name=name, browser=self, tab=_tab)
		_label.set_text(_obs.name)
		self.observers.append(_obs)
		self.tabs[_tab] = _obs
		self.currentobservertab = _tab
		return _obs
	
	def sync_observers(self):
		for _o in self.observers:
			_o.sync()
	
	def delete_event(self, widget, event):
		self.do_quit()	
		return False

	def observe(self,instance):
			if len(self.observers) ==0:
				self.create_observer()
			_observer = self.tabs[self.currentobservertab]
			_observer.add_instance(instance)

if __name__ == "__main__":
	b = Browser();
	b.run()
