#!/usr/bin/env python

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango
import re
import preferences # loading/saving of .ini options
import urlparse
import optparse

from solverparameters import * # 'solver parameters' window
from help import * # viewing help files

import sys, dl
# This sets the flags for dlopen used by python so that the symbols in the
# ascend library are made available to libraries dlopened within ASCEND:
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)
import ascend

# This is my first ever GUI code so please be nice :)

# The fancy tree-view gizmo is the GtkTreeView object. See the article
# http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/300304
# for the original source code on which my implementation was based.

GLADE_FILE = "/home/john/src/ascend/trunk/pygtk/interface/ascend.glade"

CHANGED_COLOR = "#FFFF88"
SOLVERPARAM_BOOL_TRUE = "Yes"
SOLVERPARAM_BOOL_FALSE = "No"

ESCAPE_KEY = 65307

HELP_ROOT = None

#======================================
# Browser is the main ASCEND library/model browser window

class Browser:

#   ---------------------------------
#   SETUP

	def __init__(self):
		#--------
		# load the file referenced in the command line, if any

		parser = optparse.OptionParser(usage="%prog [[-m typename] file]", version="gtkbrowser $rev$" )
		# add options here if we want

		parser.add_option("-m", "--model"
            ,action="store", type="string", dest="model"
			,help="specify the model to instantiate upon loading modules")		
		(options, args) = parser.parse_args()

		#print "OPTIONS_______________:",options
		
		#--------
		# load up the preferences ini file

		self.prefs = preferences.Preferences()

		#--------
		# initialise ASCEND

		self.library = ascend.Library()

		self.sim = None

		#-------------------
		# Set up the window and main widget actions

		glade = gtk.glade.XML(GLADE_FILE,"browserwin")

		self.window = glade.get_widget("browserwin")

		if not self.window:
			raise RuntimeError("Couldn't load window from glade file")

		_display = self.window.get_screen().get_display().get_name();
		_geom=self.prefs.getGeometrySizePosition(_display,"browserwin")
		if _geom:
			self.window.resize(_geom[0],_geom[1]);
			self.window.move(_geom[2],_geom[3]);
		
		self.window.connect("delete_event", self.delete_event)

		self.browserpaned=glade.get_widget("browserpaned");
		_geom2=self.prefs.getGeometryValue(_display,"browserpaned");
		if _geom2:
			self.browserpaned.set_position(_geom2);

		self.openbutton=glade.get_widget("openbutton")
		self.openbutton.connect("clicked",self.open_click)

		self.reloadbutton=glade.get_widget("reloadbutton")
		self.reloadbutton.connect("clicked",self.reload_click)
		
		self.solvebutton=glade.get_widget("solvebutton")
		self.solvebutton.connect("clicked",self.solve_click)

		self.checkbutton=glade.get_widget("checkbutton")
		self.checkbutton.connect("clicked",self.check_click)

		self.autotoggle=glade.get_widget("autotoggle")
		self.autotoggle.connect("toggled",self.auto_toggle)

		self.is_auto = self.autotoggle.get_active()

		self.methodrunbutton=glade.get_widget("methodrunbutton")
		self.methodrunbutton.connect("clicked",self.methodrun_click)

		self.methodsel=glade.get_widget("methodsel")

		self.maintabs = glade.get_widget("maintabs")

		self.statusbar = glade.get_widget("statusbar")

		self.menu = glade.get_widget("browsermenu")
		glade.signal_autoconnect(self)

		self.automenu = glade.get_widget("automenu")
		self.automenu.set_active(self.is_auto)
		if self.automenu == None:
			print "NO AUTOMENU FOUND"

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
		# set up the context menu for fixing/freeing vars

		# TODO import this menu from Glade (this code is a PITA)

		self.treecontext = gtk.Menu();
		self.fixmenuitem = gtk.ImageMenuItem("_Fix",True);
		_img = gtk.Image()
		_img.set_from_file('icons/locked.png')
		self.fixmenuitem.set_image(_img)

		self.freemenuitem = gtk.ImageMenuItem("F_ree",True);
		_img = gtk.Image()
		_img.set_from_file('icons/unlocked.png')
		self.freemenuitem.set_image(_img)

		self.plotmenuitem = gtk.ImageMenuItem("P_lot",True);
		_img = gtk.Image()
		_img.set_from_file('icons/plot.png')
		self.plotmenuitem.set_image(_img)

		self.propsmenuitem = gtk.ImageMenuItem("_Properties",True);
		_img = gtk.Image()
		_img.set_from_file('icons/properties.png')
		self.propsmenuitem.set_image(_img)

		self.fixmenuitem.show(); self.fixmenuitem.set_sensitive(False)
		self.freemenuitem.show(); self.freemenuitem.set_sensitive(False)
		self.plotmenuitem.show(); self.plotmenuitem.set_sensitive(False)
		self.propsmenuitem.show()
		self.treecontext.append(self.fixmenuitem);
		self.treecontext.append(self.freemenuitem);
		_sep = gtk.SeparatorMenuItem(); _sep.show()
		self.treecontext.append(_sep);
		self.treecontext.append(self.plotmenuitem);
		_sep = gtk.SeparatorMenuItem(); _sep.show()
		self.treecontext.append(_sep);
		self.treecontext.append(self.propsmenuitem);
		self.fixmenuitem.connect("activate",self.fix_activate)
		self.freemenuitem.connect("activate",self.free_activate)
		self.plotmenuitem.connect("activate",self.plot_activate)
		self.propsmenuitem.connect("activate",self.props_activate)
		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")
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
		self.reporter = ascend.getReporter()
		self.reporter.setPythonErrorCallback(self.error_callback)

		#-------------------
		# set up the module view

		self.modtank = {}
		self.moduleview = glade.get_widget("moduleview")
		modulestorecoltypes = [str, str]
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
			i = i + 1
		self.moduleview.connect("row-activated", self.module_activated )
	
		#-------------------
		# RE for units matching
		self.units_re = re.compile("([-+]?(\d+(\.\d*)?|\d*\.d+)([eE][-+]?\d+)?)\s*(.*)");

		#--------------------
		# set up the methods combobox

		self.methodstore = gtk.ListStore(str)
		self.methodsel.set_model(self.methodstore)
		_methodrenderer = gtk.CellRendererText()
		self.methodsel.pack_start(_methodrenderer, True)
		self.methodsel.add_attribute(_methodrenderer, 'text',0)

		#--------
		# set up the instance browser view

		self.otank = {}
		self.treeview = glade.get_widget("browserview")
		columns = [str,str,str,str,int,bool]
		self.treestore = gtk.TreeStore(*columns)
		titles = ["Name","Type","Value"];
		self.treeview.set_model(self.treestore)
		self.tvcolumns = [ gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		
		self.treeview.connect("row-expanded", self.row_expanded )
		self.treeview.connect("button-press-event", self.tree_click )

		# data columns are: name type value colour weight editable
		
		i = 0
		for tvcolumn in self.tvcolumns[:len(titles)]:
			tvcolumn.set_title(titles[i])
			self.treeview.append_column(tvcolumn)			

			renderer = gtk.CellRendererText()
			tvcolumn.pack_start(renderer, True)
			tvcolumn.add_attribute(renderer, 'text', i)
			tvcolumn.add_attribute(renderer, 'foreground', 3)
			tvcolumn.add_attribute(renderer, 'weight', 4)
			if(i==2):
				tvcolumn.add_attribute(renderer, 'editable', 5)
				renderer.connect('edited',self.cell_edited_callback)
			i = i + 1


		if(len(args)==1):
			self.do_open(args[0])

			print "Options: ",options

			if options.model:
				try:
					_t =self.library.findType(options.model);
					self.do_sim(_t);
				except RuntimeError, e:
					self.reporter.reportError("Failed to create instance of '%s': %s" %(options.model, str(e)));
		

	def run(self):
		self.window.show()
		gtk.main()

#   --------------------------------------------
# 	MAJOR GUI COMMANDS


	def do_open(self,filename):
		# TODO does the user want to lose their work?
		# TODO do we need to chdir?

		_context = self.statusbar.get_context_id("do_open")

		self.errorstore.clear()

		self.treestore.clear()
		self.otank = {}
	
		# self.library.clear()

		self.statusbar.push(_context,"Loading '"+filename+"'")
		self.library.load(filename)
		self.statusbar.pop(_context)

		self.filename = filename

		# Load the current list of modules into self.modules
		self.modtank = {}
		self.modulestore.clear()
		modules = self.library.getModules()
		for m in reversed(modules):
			_n = str( m.getName() )
			_f = str( m.getFilename() )
			#print "ADDING ROW name %s, file = %s" % (_n, _f)
			_r = self.modulestore.append(None,  [ _n, _f ])
			for t in self.library.getModuleTypes(m):
				_n = t.getName()
				#print "ADDING TYPE %s" % _n
				_piter = self.modulestore.append(_r , [ _n, "" ])
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

		self.sim = type_object.getSimulation(str(type_object.getName())+"_sim")
		print "...DONE 'getSimulation'"		
		self.stop_waiting()

		self.start_waiting("Building simulation...")
		print "BUILDING SIMULATION"
		self.sim.build()
		print "DONE BUILDING"
		self.stop_waiting()

		self.sim.setSolver(ascend.Solver("QRSlv"))

		# empty things out first
		self.methodstore.clear()
		self.treestore.clear()

		# methods
		_methods = self.sim.getType().getMethods()
		_activemethod = None;
		for _m in _methods:
			_i = self.methodstore.append([_m.getName()])
			if _m.getName()=="default_self":
				self.methodsel.set_active_iter(_i)

		# instance hierarchy
		self.otank = {} # map path -> (name,value)
		self.make( self.sim.getName(),self.sim.getModel() )
		self.maintabs.set_current_page(1);
		
	def do_solve(self):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return;

		self.start_waiting("Solving...")

		self.sim.solve(ascend.Solver("QRSlv"))

		self.stop_waiting()
		self.refreshtree()

	def do_check(self):
		if not self.sim:
			self.reporter.reportError("No model selected yet")

		self.start_waiting("Checking system...")

		if self.sim.check():
			self.reporter.reportNote("System check OK")

		self.sim.checkDoF()

		self.stop_waiting()

		self.refreshtree()

	def do_method(self,method):
		if not self.sim:
			self.reporter.reportError("No model selected yet")

		self.sim.run(method)
		self.refreshtree()

	def do_quit(self):
		self.reporter.clearPythonErrorCallback()
		_w,_h = self.window.get_size()
		_t,_l = self.window.get_position()
		_display = self.window.get_screen().get_display().get_name()
		self.prefs.setGeometrySizePosition(_display,"browserwin",_w,_h,_t,_l );

		_p = self.browserpaned.get_position()
		self.prefs.setGeometryValue(_display,"browserpaned",_p);

		# causes prefs to be saved unless they are still being used elsewher
		del(self.prefs)

		gtk.main_quit()
		print "GTK QUIT"
		return False

	def on_tools_sparsity_click(self,*args):
		self.reporter.reportNote("Preparing sparsity matrix...")

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
			
#   --------------------------------------------
#   INSTANCE TREE

	def get_tree_row_data(self,instance):
		_value = str(instance.getValue())
		_type = str(instance.getType())
		_name = str(instance.getName())
		_fgcolor = "black"
		_fontweight = pango.WEIGHT_NORMAL
		_editable = False
		if instance.getType().isRefinedSolverVar():
			_editable = True
			if instance.isFixed():
				_fgcolor = "#008800"
				_fontweight = pango.WEIGHT_BOLD
			else:
				_fgcolor = "#000088"
				_fontweight = pango.WEIGHT_BOLD
		elif instance.isBool() or instance.isReal() or instance.isInt():
			# TODO can't edit constants that have already been refined
			_editable = True

		#if(len(_value) > 80):
		#	_value = _value[:80] + "..."
		
		return [_name, _type, _value, _fgcolor, _fontweight, _editable]

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
			_fgcolor = "#008800"
		
		if not filename and not line:
			_fileline = ""
		else:
			if(len(filename) > 25):
				filename = "..."+filename[-22:]
			_fileline = filename + ":" + str(line)

		_res = [_sevicon,_fileline,msg.rstrip(),_fgcolor,_fontweight]
		#print _res
		return _res  

	def make_row( self, piter, name, value ):

		_piter = self.treestore.append( piter, self.get_tree_row_data(value) )
		return _piter

	def refreshtree(self):
		# @TODO FIXME use a better system than colour literals!
		for _path in self.otank: # { path : (name,value) }
			_iter = self.treestore.get_iter(_path)
			_name, _instance = self.otank[_path]
			self.treestore.set_value(_iter, 2, _instance.getValue())
			if _instance.getType().isRefinedSolverVar():
				if _instance.isFixed() and self.treestore.get_value(_iter,3)=="#000088":
					self.treestore.set_value(_iter,3,"#008800")
				elif not _instance.isFixed() and self.treestore.get_value(_iter,3)=="#008800":
					self.treestore.set_value(_iter,3,"#000088")

	def cell_edited_callback(self, renderer, path, newtext, **kwargs):
		# get back the Instance object we just edited (having to use this seems like a bug)
		path = tuple( map(int,path.split(":")) )

		if not self.otank.has_key(path):
			raise RuntimeError("cell_edited_callback: invalid path '%s'" % path)
			return

		_name, _instance = self.otank[path]

		if _instance.isReal():
			# only real-valued things can have units
			
			try:
				# match a float with option text afterwards, optionally separated by whitespace
				_match = re.match(self.units_re,newtext)
				if not _match:
					self.reporter.reportError("Not a valid value-and-optional-units")
					return

				_val = _match.group(1)
				_units = _match.group(5)
				#_val, _units = re.split("[ \t]+",newtext,2);
			except RuntimeError:
				self.reporter.reportError("Unable to split value and units")
				return
			print "val = ",_val
			print "units = ",_units

			# parse the units, throw an error if no good
			try:
				_val = float(_val)
			except RuntimeError:
				self.reporter.reportError("Unable to convert number part '%s' to float" % _val)

			if _units.strip() == "":
				_u = _instance.getType().getPreferredUnits()
				if _u == None:
					_u = _instance.getDimensions().getDefaultUnits()
				self.reporter.reportNote("Assuming units '%s'" % _u.getName().toString() )
			else:
				try:
					_u = ascend.Units(_units)
					self.reporter.reportNote("Parsed units %s" % _units)
				except RuntimeError:
					self.reporter.reportError("Unrecognisable units '%s'" % _units)
					return

				if _instance.getDimensions() != _u.getDimensions():

					if _u.getDimensions().isDimensionless():
						_units = "[dimensionless]"

					_my_dims = _instance.getDimensions().getDefaultUnits()
					if _instance.getDimensions().isDimensionless():
						_my_dims = "[dimensionless]"

					self.reporter.reportError("Incompatible units '%s' (must fit with '%s')" 
							% (_units, _my_dims) )
					return

			if _units.strip() != "" and not _instance.getDimensions().isDimensionless():
				self.prefs.setPreferredUnits(str(_instance.getType().getName()), _units);
		
			_conv = float(_u.getConversion())
			# self.reporter.reportNote("Converting: multiplying '%s %s' by factor %s to get SI units" % (_val, _units, _conv) )
			_val = _val * _conv;

			self.reporter.reportNote("Setting '%s' to '%f'" % (_name, _val))
			
			if _instance.getType().isRefinedSolverVar():
				# set the 'fixed' flag as well
				_instance.setFixedValue(float(_val))
			else:
				_instance.setRealValue(float(_val))
		else:
			if _instance.isBool():
				_lower = newtext.lower();
				if _lower.startswith("t") or _lower.startswith("y") or _lower.strip()=="1":
					newtext = 1
				elif _lower.startswith("f") or _lower.startswith("n") or _lower.strip()=="0":
					newtext = 0
				else:
					self.reporter.reportError("Invalid entry for a boolean variable: '%s'" % newtext)
					return
				_val = bool(newtext);
				if _val == _instance.getValue():
					self.reporter.reportNote("Boolean atom '%s' was not altered" % _instance.getName())
					return
				_instance.setBoolValue(_val)

			elif _instance.isInt():
				_val = int(newtext)
				if _val == _instance.getValue():
					self.reporter.reportNote("Integer atom '%s' was not altered" % _instance.getName())
					return
				_instance.setIntValue(_val)
			else:
				self.reporter.reportError("Attempt to set a non-real, non-boolean, non-integer value!")
				return

		# now that the variable is set, update the GUI and re-solve if desired
		_iter = self.treestore.get_iter(path)
		self.treestore.set_value(_iter,2,_instance.getValue())

		if _instance.getType().isRefinedSolverVar():
			self.treestore.set_value(_iter,3,"#008800") # set the row green as fixed
		
		if self.is_auto:
			self.sim.check()
			self.do_solve()
			#self.reporter.reportError("SOLVER completed")


	def make_children(self, value, piter ):
		if value.isCompound():
			children=value.getChildren();
			for child in children:
				_name = child.getName();
				_piter = self.make_row(piter,_name,child)
				_path = self.treestore.get_path(_piter)
				self.otank[_path]=(_name,child)
				#self.reporter.reportError("2 Added %s at path %s" % (_name,repr(_path)))

	def make(self, name=None, value=None, path=None, depth=1):
		if path is None:
			# make root node
			piter = self.make_row( None, name, value )
			path = self.treestore.get_path( piter )
			self.otank[ path ] = (name, value)
			#self.reporter.reportError("4 Added %s at path %s" % (name, path))
		else:
		    name, value = self.otank[ path ]

		piter = self.treestore.get_iter( path )
		if not self.treestore.iter_has_child( piter ):
		    self.make_children(value,piter)

		if depth:
		    for i in range( self.treestore.iter_n_children( piter ) ):
		        self.make( path = path+(i,), depth = depth - 1 )
		else:
			self.treeview.expand_row("0",False)

	def row_expanded( self, treeview, piter, path ):
		self.make( path = path )

#   ----------------------------------
#   ERROR PANEL

	def error_callback(self,sev,filename,line,msg):
		pos = self.errorstore.append(None, self.get_error_row_data(sev, filename,line,msg))
		path = self.errorstore.get_path(pos)
		col = self.errorview.get_column(3)
		self.errorview.scroll_to_cell(path,col)
		
		return 0;

#   --------------------------------
#   BUTTON METHODS

	def open_click(self,*args):
		dialog = gtk.FileChooserDialog("Open file...",
			None,
		    gtk.FILE_CHOOSER_ACTION_OPEN,
		    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
		    gtk.STOCK_OPEN, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)

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
		dialog.destroy()

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

	def solve_click(self,*args):
		#self.reporter.reportError("Solving simulation '" + self.sim.getName().toString() +"'...")
		self.do_solve()
	
	def check_click(self,*args):
		self.do_check()
		#self.reporter.reportError("CHECK clicked")

	def preferences_click(self,*args):
		if not self.sim:
			self.reporter.reportError("No simulation created yet!");
		
		_paramswin = SolverParametersWindow(self.sim, self.reporter)
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
		self.automenu.set_active(self.is_auto)

		if self.is_auto:
			self.reporter.reportSuccess("Auto mode is now ON")
		else:
			self.reporter.reportSuccess("Auto mode is now OFF")

	def on_file_quit_click(self,*args):
		self.do_quit()

	def on_tools_auto_toggle(self,checkmenuitem,*args):
		self.is_auto = checkmenuitem.get_active()
		self.autotoggle.set_active(self.is_auto)

	def on_help_about_click(self,*args):
		_xml = gtk.glade.XML(GLADE_FILE,"aboutdialog");
		_dialog = _xml.get_widget("aboutdialog")
		_dialog.run()
		_dialog.destroy()

	def on_help_contents_click(self,*args):
		_help = Help(HELP_ROOT)
		_help.run()

#   ------------------------------
#   CONTEXT MENU

	def tree_click(self,widget,event):
		# which button was clicked?
		if event.button == 3:
			_x = int(event.x)
			_y = int(event.y)
			_time = event.time
			_pthinfo = self.treeview.get_path_at_pos(_x, _y)
			if _pthinfo != None:
				_canpop = False;
				_path, _col, _cellx, _celly = _pthinfo
				# self.reporter.reportError("Right click on %s" % self.otank[_path][0])
				_instance = self.otank[_path][1]
				if _instance.getType().isRefinedSolverVar():
					_canpop = True;
					if _instance.isFixed():
						self.fixmenuitem.set_sensitive(False)
						self.freemenuitem.set_sensitive(True)
					else:
						self.fixmenuitem.set_sensitive(True)
						self.freemenuitem.set_sensitive(False)
				elif _instance.isRelation():
					_canpop = True;
					self.propsmenuitem.set_sensitive(True)					
				else:
					self.fixmenuitem.set_sensitive(False)
					self.freemenuitem.set_sensitive(False)

				if _instance.isPlottable():
					self.plotmenuitem.set_sensitive(True)
					_canpop = True;
				else:
					self.plotmenuitem.set_sensitive(False)

				if _canpop:
					self.treeview.grab_focus()
					self.treeview.set_cursor( _path, _col, 0)
					self.treecontext.popup( None, None, None, event.button, _time)
					return 1

	def fix_activate(self,widget):
		_path,_col = self.treeview.get_cursor()
		_name, _instance = self.otank[_path]
		_instance.setFixed(True)
		self.reporter.reportNote("Fixed variable %s" % _instance.getName().toString())
		if self.is_auto:
			self.sim.check()
			self.do_solve()
		else:
			self.refreshtree()
		return 1

	def free_activate(self,widget):
		_path,_col = self.treeview.get_cursor()
		_instance = self.otank[_path][1]
		_instance.setFixed(False)
		self.reporter.reportNote("Freed variable %s" % _instance.getName().toString())
		if self.is_auto:
			self.sim.check()
			self.do_solve()
		else:
			self.refreshtree()
		return 1

	def plot_activate(self,widget):
		self.reporter.reportNote("plot_activate...");
		_path,_col = self.treeview.get_cursor()
		_instance = self.otank[_path][1]
		if not _instance.isPlottable():
			self.reporter.reportError("Can't plot instance %s" % _instance.getName().toString())
			return
		else:
			self.reporter.reportNote("Instance %s about to be plotted..." % _instance.getName().toString())

		print("Plotting instance '%s'..." % _instance.getName().toString())

		_plot = _instance.getPlot()

		print "Title: ", _plot.getTitle()
		_plot.show(True)

		return 1

	def props_activate(self,widget):
		_path,_col = self.treeview.get_cursor()
		_instance = self.otank[_path][1]
		if _instance.isRelation():
			print "Relation '"+_instance.getName().toString()+"':", \
				_instance.getRelationAsString(self.sim.getModel())
		else:
			self.reporter.reportWarning("props_activate not implemented")

		
#   ---------------------------------
#   WINDOW-LEVEL ACTIONS

	def delete_event(self, widget, event, data=None):
		self.do_quit()	
		return False

def test():
	import ascend
	b = Browser();
	b.run()

if __name__ == "__main__":
    test()
