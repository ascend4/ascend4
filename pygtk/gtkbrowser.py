import sys
try:
	import loading
	#loading.print_status("Loading PSYCO")
	#try:
	#	import psyco
	#	psyco.full()
	#	print "Running with PSYCO optimisation..."
	#except ImportError:
	#	pass

	loading.print_status("Loading python standard libraries")

	import pygtk 
	pygtk.require('2.0') 
	import gtk

	import re
	import urlparse
	import optparse
	import platform
	import sys

	if platform.system() != "Windows":
		try:
			import dl
			_dlflags = dl.RTLD_GLOBAL|dl.RTLD_NOW
		except:
			# On platforms that unilaterally refuse to provide the 'dl' module
			# we'll just set the value and see if it works.
			loading.print_status("Setting dlopen flags","Python 'dl' module not available on this system")
			_dlflags = 258
		# This sets the flags for dlopen used by python so that the symbols in the
		# ascend library are made available to libraries dlopened within ASCEND:
		sys.setdlopenflags(_dlflags)



	loading.print_status("Loading LIBASCEND/ascpy")
	import ascpy
	import os.path

	loading.print_status("Loading PyGTK, glade, pango")

	import gtk.glade
	import pango

	loading.load_matplotlib()

	loading.print_status("Loading ASCEND python modules")
	from preferences import *      # loading/saving of .ini options
	from solverparameters import * # 'solver parameters' window
	from help import *             # viewing help files
	from incidencematrix import *  # incidence/sparsity matrix matplotlib window
	from observer import *         # observer tab support
	from properties import *       # solver_var properties dialog
	from varentry import *         # for inputting of variables with units
	from diagnose import * 	       # for diagnosing block non-convergence
	from solverreporter import *   # solver status reporting
	from moduleview import *       # module browser
	from modelview import *        # model browser
	from integrator import *       # integrator dialog	
	from infodialog import *       # general-purpose textual information dialog
	from versioncheck import *     # version check (contacts ascend.cruncher2.dyndns.org)
	import config

	loading.complete();

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

loading.print_status("Starting GUI")

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

		loading.print_status("Parsing options","CONFIG = %s"%config.VERSION)
		
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
		loading.create_window(self.assets_dir)
		
		self.observers = []
		self.clip = None

		#--------
		# load up the preferences ini file

		loading.print_status("Loading preferences")

		self.prefs = Preferences()

		_prefpath = self.prefs.getStringPref("Directories","librarypath",None)
		_preffileopenpath = self.prefs.getStringPref("Directories","fileopenpath",None)

		#--------
		# set up library path and the path to use for File->Open dialogs
		
		if self.options.library_path != None:
			_path = os.path.abspath(self.options.library_path)
			_pathsrc = "command line options"
			# when a special path is specified, use the last path component as the file-open location
			if platform.system()=="Windows":
				self.fileopenpath = _path.split(":").pop()
			else:
				self.fileopenpath = _path.split(":").pop()
		else:
			if _prefpath:
				_path = _prefpath
				_pathsrc = "user preferences"
			else:
				# default setting, but override with Windows registry if present
				_path = config.LIBRARY_PATH
				_pathsrc = "default (config.py)"

				if platform.system()=="Windows":
					# use the registry
					try:
						import _winreg
						x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
						y= _winreg.OpenKey(x,r"SOFTWARE\ASCEND")
						_regpath,t = _winreg.QueryValueEx(y,"ASCENDLIBRARY")
						_winreg.CloseKey(y)
						_winreg.CloseKey(x)
						_path = _regpath						
						os.environ['ASCENDLIBRARY'] = _regpath
						_pathsrc = "Windows registry"
					except:
						# otherwise keep using the default
						pass
			
			if _preffileopenpath:
				self.fileopenpath = _preffileopenpath
			else:
				self.fileopenpath = _path
					
		#--------
		# Create the ASCXX 'Library' object
		
		loading.print_status("Creating ASCEND 'Library' object","ASCENDLIBRARY = "+_path+" FROM "+_pathsrc)
		self.library = ascpy.Library(str(_path))

		self.sim = None

		#-------------------
		# Set up the window and main widget actions

		self.glade_file = os.path.join(self.assets_dir,config.GLADE_FILE)

		loading.print_status("Setting up windows") #,"GLADE_FILE = %s" % self.glade_file)

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

		buttons = ["open","reload","solve","integrate","check","methodrun"]
		for n in buttons:
			name = "%sbutton"%n
			setattr(self,name,glade.get_widget(name))
			getattr(self,name).connect("clicked",getattr(self,"%s_click"%n))

		widgets = ["autotoggle","automenu","methodsel","maintabs","lowertabs","consolescroll","statusbar","browsermenu"]
		for n in widgets:
			setattr(self,n,glade.get_widget(n))

		self.autotoggle.connect("toggled",self.auto_toggle)

		self.show_solving_popup=glade.get_widget("show_solving_popup")
		self.show_solving_popup.set_active(self.prefs.getBoolPref("SolverReporter","show_popup",True))
		self.close_on_converged=glade.get_widget("close_on_converged")
		self.close_on_converged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_converged",True))
		self.close_on_nonconverged=glade.get_widget("close_on_nonconverged")
		self.close_on_nonconverged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_nonconverged",True))
		self.solver_engine=glade.get_widget("solver_engine")

		self.use_relation_sharing=glade.get_widget("use_relation_sharing")
		self.use_relation_sharing.set_active(self.prefs.getBoolPref("Compiler","use_relation_sharing",True))

		self.use_binary_compilation=glade.get_widget("use_binary_compilation")
		self.use_binary_compilation.set_active(self.prefs.getBoolPref("Compiler","use_binary_compilation",False))
		self.use_binary_compilation.set_sensitive(self.use_relation_sharing.get_active())

		glade.signal_autoconnect(self)

		#-------
		# Status icons

		self.fixedimg = gtk.Image()
		self.fixedimg.set_from_file(os.path.join(self.options.assets_dir,'locked.png'))

		self.inactiveimg = gtk.Image()
		self.inactiveimg.set_from_file(os.path.join(self.options.assets_dir,'unattached.png'))

		self.iconstatusunknown = None
		self.iconfixed = self.fixedimg.get_pixbuf()
		self.iconsolved = self.window.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconactive = self.window.render_icon(gtk.STOCK_NO,gtk.ICON_SIZE_MENU)
		self.iconinactive = self.inactiveimg.get_pixbuf()
		self.iconunsolved = None

		self.statusicons={
			ascpy.ASCXX_INST_STATUS_UNKNOWN: self.iconstatusunknown
			,ascpy.ASCXX_VAR_FIXED: self.iconfixed
			,ascpy.ASCXX_VAR_SOLVED: self.iconsolved
			,ascpy.ASCXX_VAR_ACTIVE: self.iconactive
			,ascpy.ASCXX_VAR_UNSOLVED: self.iconunsolved
			,ascpy.ASCXX_REL_INACTIVE: self.iconinactive
		}


		self.statusmessages={
			ascpy.ASCXX_INST_STATUS_UNKNOWN: "Status unknown"
			,ascpy.ASCXX_VAR_FIXED: "Fixed"
			,ascpy.ASCXX_VAR_SOLVED: "Converged"
			,ascpy.ASCXX_VAR_ACTIVE: "Active (unconverged)"
			,ascpy.ASCXX_VAR_UNSOLVED: "Not yet visited"
			,ascpy.ASCXX_REL_INACTIVE: "Inactive"
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
		self.moduleview = ModuleView(self,glade, self.library)
	
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
			try:
				self.do_open(args[0])
			except RuntimeError,e:
				self.reporter.reportError(str(e))
				return

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
							self.reporter.reportNote("Instantiated self-titled model '%s'" %_model)
					except RuntimeError, e:
						self.reporter.reportError("Failed to create instance of '%s': %s" 
							%(_model, str(e))
						);
				except RuntimeError, e:
					if self.options.model:
						self.reporter.reportError("Unknown model type '%s': %s" 
							%(_model, str(e))
						);		


		#--------
		# report absence of solvers if nec.

		if not len(ascpy.getSolvers()):
			print "NO SOLVERS LOADED!"
			self.reporter.reportError( "No solvers were loaded! ASCEND is probably not configured correctly." )

		#--------
		# IPython console, if available

		import console
		console.create_widget(self)

	def run(self):
		self.window.show()
		loading.print_status("ASCEND is now running")
		loading.complete()
		gtk.main()

#   ------------------
#   SOLVER LIST

	def set_solver(self,solvername):
		""" this sets the active solver in the GUI, which is the default applied to newly instantiated models """
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
		try:
			self.filename = filename
			# call the low-level 'load' command...
			self.library.load(filename)
		except RuntimeError,e:
			self.statusbar.pop(_context)
			raise

		print "Statusbar =",self.statusbar
		try:
			self.statusbar.pop(_context)
		except TypeError,e:
			print "For some reason, a type error (context=%s,filename=%s): %s" % (_context,filename,e)

		# Load the current list of modules into self.modules
		self.moduleview.refresh(self.library)

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

			_v = self.prefs.getBoolPref("Compiler","use_binary_compilation",True)
			ascpy.getCompiler().setBinaryCompilation(True)

			self.sim = type_object.getSimulation(str(type_object.getName())+"_sim",False)

			#self.reporter.reportNote("SIMULATION ASSIGNED")
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return

		self.stop_waiting()

		# get method names and load them into the GUI
		self.methodstore.clear()
		_methods = self.sim.getType().getMethods()
		_activemethod = None;
		for _m in _methods:
			_i = self.methodstore.append([_m.getName()])
			if _m.getName()=="on_load":
				self.methodsel.set_active_iter(_i)

		self.modelview.setSimulation(self.sim)

		# run the 'on_load' method
		self.start_waiting("Running default method...")
		try:
			#self.reporter.reportNote("SIMULATION CREATED, RUNNING DEFAULT METHOD NOW...")
			self.sim.runDefaultMethod()
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return			
		self.stop_waiting()

		self.modelview.refreshtree()
	
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

		try:
			self.sim.build()
		except RuntimeError,e:
			self.reporter.reportError("Couldn't build system: %s",str(e));
			return

		if not hasattr(self,'solver'):
			self.reporter.reportError("No solver assigned!")
			return

		self.start_waiting("Solving with %s..." % self.solver.getName())

		if self.prefs.getBoolPref("SolverReporter","show_popup",True):
			reporter = PopupSolverReporter(self,self.sim.getNumVars())
		else:
			reporter = SimpleSolverReporter(self)

		try:
			self.sim.solve(self.solver,reporter)
		except RuntimeError,e:
			self.reporter.reportError(str(e))	

		self.stop_waiting()
		
		self.modelview.refreshtree()

	def do_integrate(self):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return

		try:
			self.sim.build()
		except RuntimeError,e:
			self.reporter.reportError("Couldn't build system: %s",str(e))
			return

		integwin = IntegratorWindow(self,self.sim)		
		_integratorreporter = integwin.run()
		if _integratorreporter!=None:
			_integratorreporter.run()
			self.sim.processVarStatus()
			self.modelview.refreshtree()
		

	def do_check(self):
		try:
			self.sim.build()
		except RuntimeError,e:
			self.reporter.reportError("Couldn't build system: %s",str(e));
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
			else:
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

		try:
			self.sim.run(method)
		except RuntimeError,e:
			self.reporter.reportError(str(e))
		
		self.sim.processVarStatus()
		self.modelview.refreshtree()

	def do_quit(self):
		loading.print_status("Saving window location")		
		self.reporter.clearPythonErrorCallback()

		_w,_h = self.window.get_size()
		_t,_l = self.window.get_position()
		_display = self.window.get_screen().get_display().get_name()
		self.prefs.setGeometrySizePosition(_display,"browserwin",_w,_h,_t,_l );

		_p = self.browserpaned.get_position()
		self.prefs.setGeometryValue(_display,"browserpaned",_p);

		loading.print_status("Saving current directory")			
		self.prefs.setStringPref("Directories","fileopenpath",self.fileopenpath)

		self.prefs.setBoolPref("Browser","auto_solve",self.is_auto)

		loading.print_status("Saving preferences")
		# causes prefs to be saved unless they are still being used elsewher
		del(self.prefs)

		loading.print_status("Clearing error callback")		
		self.reporter.clearPythonErrorCallback()

		loading.print_status("Closing down GTK")
		gtk.main_quit()

		loading.print_status("Clearing library")			
		self.library.clear()
		
		loading.print_status("Quitting")

		return False

	def on_tools_sparsity_click(self,*args):

		self.reporter.reportNote("Preparing incidence matrix...")
		_im = self.sim.getIncidenceMatrix();

		self.reporter.reportNote("Plotting incidence matrix...")

		_sp = IncidenceMatrixWindow(_im);
		_sp.run();

	def on_tools_repaint_tree_activate(self,*args):
		self.reporter.reportNote("Repainting model view...")
		self.modelview.refreshtree()

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
		self.use_binary_compilation.set_sensitive(_v);

	def on_use_binary_compilation_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("Compiler","use_binary_compilation",_v)
		self.reporter.reportNote("Binary compilation set to "+str(_v))

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

	def on_show_vars_far_from_nominals_activate(self,*args):
		_bignum = self.prefs.getRealPref("Browser","far_from_nominals",10);
		text = "Variables Far from Nominals"
		title=text;
		text += "\n"
		_vars = self.sim.getVariablesFarFromNominals(_bignum)
		if len(_vars):
			for _v in _vars:
				text += "\n%s"%_v.getName()
		else:
			text +="\nnone"

		text+="\n\nAbove calculated using a relative error of %f" % float(_bignum)
		text+="\nModify this value in .ascend.ini, section '[Browser]', key 'far_from_nominals'."
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

#   ----------------------------------
#   ERROR PANEL

	def get_error_row_data(self,sev,filename,line,msg):
		try:
			_sevicon = {
				0   : self.iconok
				,1  : self.iconinfo
				,2  : self.iconwarning
				,4  : self.iconerror
				,8  : self.iconinfo
				,16 : self.iconwarning
				,32 : self.iconerror
				,64 : self.iconerror
			}[sev]
		except KeyError:
			_sevicon = self.iconerror

		_fontweight = pango.WEIGHT_NORMAL
		if sev==32 or sev==64:
			_fontweight = pango.WEIGHT_BOLD
		
		_fgcolor = "black"
		if sev==8:
			_fgcolor = "#888800"
		elif sev==16:
			_fgcolor = "#884400"
		elif sev==32 or sev==64:
			_fgcolor = "#880000"
		elif sev==0:
			_fgcolor = BROWSER_FIXED_COLOR
		
		if not filename and not line:
			_fileline = ""
		else:
			if(len(filename) > 25):
				filename = "..."+filename[-22:]
			_fileline = filename + ":" + str(line)

		_res = (_sevicon,_fileline,msg.rstrip(),_fgcolor,_fontweight)
		#print _res
		return _res  

	def error_callback(self,sev,filename,line,msg):
		#print "SEV =",sev
		#print "FILENAME =",filename
		#print "LINE =",line
		#print "MSG =",msg
		pos = self.errorstore.append(None, self.get_error_row_data(sev, filename,line,msg))
		path = self.errorstore.get_path(pos)
		col = self.errorview.get_column(3)
		self.errorview.scroll_to_cell(path,col)		
		return 0;

#   --------------------------------
#   BUTTON METHODS

	def open_click(self,*args):
		#loading.print_status("CURRENT FILEOPENPATH is",self.fileopenpath)
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

		try:
			self.do_open(self.filename)		
			if _type:
				_t = self.library.findType(_type)
				self.do_sim(_t)
		except RuntimeError,e:
			self.reporter.reportError(str(e))

	def props_activate(self,widget,*args):
		return self.modelview.props_activate(self,widget,*args)

	def observe_activate(self,widget,*args):
		return self.modelview.observe_activate(self,widget,*args)

	def solve_click(self,*args):
		#self.reporter.reportError("Solving simulation '" + self.sim.getName().toString() +"'...")
		self.do_solve()

	def console_click(self,*args):
		self.lowertabs.set_current_page(1)
		self.consoletext.grab_focus()

	def integrate_click(self,*args):
		self.do_integrate()
	
	def check_click(self,*args):
		self.do_check()
		#self.reporter.reportError("CHECK clicked")

	def preferences_click(self,*args):
		if not self.sim:
			self.reporter.reportError("No simulation created yet!");		
		self.sim.setSolver(self.solver)
		_params = self.sim.getParameters()
		_paramswin = SolverParametersWindow(
			browser=self
			,params=_params
			,name=self.solver.getName()
		)
		if _paramswin.run() == gtk.RESPONSE_OK:
			print "PARAMS UPDATED"
			self.sim.setParameters(_paramswin.params)
		else:
			print "PARAMS NOT UPDATED"

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

	def on_report_a_bug_click(self,*args):
		import urllib
		import platform
		_plat = str(platform.system())
		_version = config.VERSION
		_help = Help(
			url="http://ascendbugs.cheme.cmu.edu/report/?project_id=ascend&platform=%s&build=%s"
				% (_plat,_version)
		)
		_help.run()

	def on_help_check_for_updates_click(self,*args):
		v = VersionCheck()
		title = "Check for updates"
		text = "Your version is %s\n" % config.VERSION
		try:
			v.check()
			if config.VERSION==v.latest:
				text += "You are running the latest released version"
			else:
				text += "Latest version is %s\n" % v.latest
				if v.info:
					text += "Get more info at %s\n" % v.info
				if v.download:
					text += "Download from %s\n" % v.download
		except Exception, e:
			text += "\nUnable to check version\n"
			text += str(e)

		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_fixable_variables_activate(self,*args):
		try:
			v = self.sim.getFixableVariables()
		except RuntimeError,e:
			self.reporter.reportError(str(e))
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

	def on_show_fixed_vars_activate(self,*args):
		try:
			v = self.sim.getFixedVariables()
			text = "Fixed Variables"
			title = text
			text += "\n"
			if len(v):
				for var in v:
					text += "\n%s"%var
			else:
				text += "\nnone"
			_dialog = InfoDialog(self,self.window,text,title)
			_dialog.run()
		except RuntimeError,e:
			self.reporter.reportError(str(e))

	def on_show_freeable_variables_activate(self,*args):
		try:
			v = self.sim.getFreeableVariables()
		except RuntimeError,e:
			self.reporter.reportError(str(e))

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
