import gtk
import gtk.glade
import pango
import ascpy

from varentry import *
from properties import *

BROWSER_FIXED_COLOR = "#008800"
BROWSER_FREE_COLOR = "#000088"

BROWSER_ACTIVE_COLOR = "black"
BROWSER_INACTIVE_COLOR = "#888888"

class ModelView:
	def __init__(self,browser,glade):
		self.browser = browser # the parent object: the entire ASCEND browser		

		self.modelview = glade.get_widget("browserview")
		
		# name, type, value, foreground, weight, editable, status-icon
		columns = [str,str,str,str,int,bool,gtk.gdk.Pixbuf]

		self.otank = {}

		# name, type, value, foreground, weight, editable, status-icon
		columns = [str,str,str,str,int,bool,gtk.gdk.Pixbuf]
		self.modelstore = gtk.TreeStore(*columns)
		titles = ["Name","Type","Value"];
		self.modelview.set_model(self.modelstore)
		self.tvcolumns = [ gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		
		self.modelview.connect("row-expanded", self.row_expanded )
		self.modelview.connect("button-press-event", self.on_treeview_event )
		self.modelview.connect("key-press-event",self.on_treeview_event )

		# data columns are: name type value colour weight editable
		
		i = 0
		for tvcolumn in self.tvcolumns[:len(titles)]:
			tvcolumn.set_title(titles[i])
			self.modelview.append_column(tvcolumn)			

			if(i==2):
				# add status icon
				renderer1 = gtk.CellRendererPixbuf()
				tvcolumn.pack_start(renderer1, False)
				tvcolumn.add_attribute(renderer1, 'pixbuf', 6)

			renderer = gtk.CellRendererText()
			tvcolumn.pack_start(renderer, True)
			tvcolumn.add_attribute(renderer, 'text', i)
			tvcolumn.add_attribute(renderer, 'foreground', 3)
			tvcolumn.add_attribute(renderer, 'weight', 4)
			if(i==2):
				tvcolumn.add_attribute(renderer, 'editable', 5)
				renderer.connect('edited',self.cell_edited_callback)
			i = i + 1

		#--------------------
		# set up the context menu for fixing/freeing vars

		# TODO import this menu from Glade (this code is a PITA)

		self.treecontext = gtk.Menu();
		self.fixmenuitem = gtk.ImageMenuItem("_Fix",True);
		self.fixmenuitem.set_image(self.browser.fixedimg)

		self.freemenuitem = gtk.ImageMenuItem("F_ree",True);
		_img = gtk.Image()
		_img.set_from_file(self.browser.options.assets_dir+'unlocked.png')
		self.freemenuitem.set_image(_img)

		self.plotmenuitem = gtk.ImageMenuItem("P_lot",True);
		_img = gtk.Image()
		_img.set_from_file(self.browser.options.assets_dir+'plot.png')
		self.plotmenuitem.set_image(_img)

		self.propsmenuitem = gtk.ImageMenuItem("_Properties",True);
		_img = gtk.Image()
		_img.set_from_file(self.browser.options.assets_dir+'properties.png')
		self.propsmenuitem.set_image(_img)

		self.observemenuitem = gtk.ImageMenuItem("_Observe",True);
		_img = gtk.Image()
		_img.set_from_file(self.browser.options.assets_dir+'observe.png')
		self.observemenuitem.set_image(_img)

		self.fixmenuitem.show(); self.fixmenuitem.set_sensitive(False)
		self.freemenuitem.show(); self.freemenuitem.set_sensitive(False)
		self.plotmenuitem.show(); self.plotmenuitem.set_sensitive(False)
		self.observemenuitem.show(); self.observemenuitem.set_sensitive(False)
		self.propsmenuitem.show()
		self.treecontext.append(self.fixmenuitem)
		self.treecontext.append(self.freemenuitem)
		_sep = gtk.SeparatorMenuItem(); _sep.show()
		self.treecontext.append(_sep);
		self.treecontext.append(self.plotmenuitem)
		self.treecontext.append(self.observemenuitem)
		_sep = gtk.SeparatorMenuItem(); _sep.show()
		self.treecontext.append(_sep)
		self.treecontext.append(self.propsmenuitem)
		self.fixmenuitem.connect("activate",self.fix_activate)
		self.freemenuitem.connect("activate",self.free_activate)
		self.plotmenuitem.connect("activate",self.plot_activate)
		self.propsmenuitem.connect("activate",self.props_activate)
		self.observemenuitem.connect("activate",self.observe_activate)

		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")

	def setSimulation(self,sim):
		# instance hierarchy
		self.sim = sim
		self.modelstore.clear()
		self.otank = {} # map path -> (name,value)
		self.make( self.sim.getName(),self.sim.getModel() )
		self.browser.maintabs.set_current_page(1);

	def clear(self):
		self.modelstore.clear()
		self.otank = {}

#   --------------------------------------------
#   INSTANCE TREE

	def get_tree_row_data(self,instance): # for instance browser
		_value = str(instance.getValue())
		_type = str(instance.getType())
		_name = str(instance.getName())
		_fgcolor = BROWSER_ACTIVE_COLOR
		_fontweight = pango.WEIGHT_NORMAL
		_editable = False
		_statusicon = None
		if instance.getType().isRefinedSolverVar():
			_editable = True
			_fontweight = pango.WEIGHT_BOLD
			if instance.isFixed():
				_fgcolor = BROWSER_FIXED_COLOR
			else:
				_fgcolor = BROWSER_FREE_COLOR
				_fontweight = pango.WEIGHT_BOLD
			_status = instance.getVarStatus();
			_statusicon = self.browser.statusicons[_status]
		elif instance.isRelation():
			if not instance.isActive():
				_fgcolor = BROWSER_INACTIVE_COLOR
		elif instance.isBool() or instance.isReal() or instance.isInt():
			# TODO can't edit constants that have already been refined
			_editable = True

		#if(len(_value) > 80):
		#	_value = _value[:80] + "..."
		
		return [_name, _type, _value, _fgcolor, _fontweight, _editable, _statusicon]

	def make_row( self, piter, name, value ): # for instance browser

		_piter = self.modelstore.append( piter, self.get_tree_row_data(value) )
		return _piter

	def refreshtree(self):
		# @TODO FIXME use a better system than colour literals!
		for _path in self.otank: # { path : (name,value) }
			_iter = self.modelstore.get_iter(_path)
			_name, _instance = self.otank[_path]
			self.modelstore.set_value(_iter, 2, _instance.getValue())
			if _instance.getType().isRefinedSolverVar():
				if _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FREE_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR)
				elif not _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FIXED_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FREE_COLOR)
				self.modelstore.set_value(_iter, 6, self.browser.statusicons[_instance.getVarStatus()])
			elif _instance.isRelation():
				if _instance.isActive():
					self.modelstore.set_value(_iter,3,BROWSER_ACTIVE_COLOR)
				else:
					self.modelstore.set_value(_iter,3,BROWSER_INACTIVE_COLOR)

	def cell_edited_callback(self, renderer, path, newtext, **kwargs):
		# get back the Instance object we just edited (having to use this seems like a bug)
		path = tuple( map(int,path.split(":")) )

		if not self.otank.has_key(path):
			raise RuntimeError("cell_edited_callback: invalid path '%s'" % path)
			return

		_name, _instance = self.otank[path]

		if _instance.isReal():
			# only real-valued things can have units
			
			_e = RealAtomEntry(_instance,newtext);
			try:
				_e.checkEntry()
				_e.setValue()
				_e.exportPreferredUnits(self.browser.prefs)
			except InputError, e:
				self.browser.reporter.reportError(str(e))
				return;

		else:
			if _instance.isBool():
				_lower = newtext.lower();
				if _lower.startswith("t") or _lower.startswith("y") or _lower.strip()=="1":
					newtext = 1
				elif _lower.startswith("f") or _lower.startswith("n") or _lower.strip()=="0":
					newtext = 0
				else:
					self.browser.reporter.reportError("Invalid entry for a boolean variable: '%s'" % newtext)
					return
				_val = bool(newtext);
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Boolean atom '%s' was not altered" % _instance.getName())
					return
				_instance.setBoolValue(_val)

			elif _instance.isInt():
				_val = int(newtext)
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Integer atom '%s' was not altered" % _instance.getName())
					return
				_instance.setIntValue(_val)
			else:
				self.browser.reporter.reportError("Attempt to set a non-real, non-boolean, non-integer value!")
				return

		# now that the variable is set, update the GUI and re-solve if desired
		_iter = self.modelstore.get_iter(path)
		self.modelstore.set_value(_iter,2,_instance.getValue())

		if _instance.getType().isRefinedSolverVar():
			self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR) # set the row green as fixed
		
		self.browser.do_solve_if_auto()

	def make_children(self, value, piter ):
		if value.isCompound():
			children=value.getChildren();
			for child in children:
				_name = child.getName();
				_piter = self.make_row(piter,_name,child)
				_path = self.modelstore.get_path(_piter)
				self.otank[_path]=(_name,child)
				#self.browser.reporter.reportError("2 Added %s at path %s" % (_name,repr(_path)))

	def make(self, name=None, value=None, path=None, depth=1):
		if path is None:
			# make root node
			piter = self.make_row( None, name, value )
			path = self.modelstore.get_path( piter )
			self.otank[ path ] = (name, value)
			#self.browser.reporter.reportError("4 Added %s at path %s" % (name, path))
		else:
		    name, value = self.otank[ path ]

		piter = self.modelstore.get_iter( path )
		if not self.modelstore.iter_has_child( piter ):
		    self.make_children(value,piter)

		if depth:
		    for i in range( self.modelstore.iter_n_children( piter ) ):
		        self.make( path = path+(i,), depth = depth - 1 )
		else:
			self.modelview.expand_row("0",False)

	def row_expanded( self, modelview, piter, path ):
		self.make( path = path )


#   ------------------------------
#   CONTEXT MENU

	def on_treeview_event(self,widget,event):
		_contextmenu = False;
		if event.type==gtk.gdk.KEY_PRESS and gtk.gdk.keyval_name(event.keyval)=='Menu':
			_contextmenu = True
			_path, _col = self.modelview.get_cursor()
			_button = 3;
		elif event.type==gtk.gdk.BUTTON_PRESS:
			if event.button == 3:
				_contextmenu = True
				_x = int(event.x)
				_y = int(event.y)
				_button = event.button
				_pthinfo = self.modelview.get_path_at_pos(_x, _y)
				if _pthinfo == None:
					return
				_path, _col, _cellx, _celly = _pthinfo
		
		# which button was clicked?
		if not _contextmenu:
			return 

		_canpop = False;
		# self.browser.reporter.reportError("Right click on %s" % self.otank[_path][0])
		_instance = self.otank[_path][1]
		if _instance.getType().isRefinedSolverVar():
			_canpop = True
			self.observemenuitem.set_sensitive(True)
			if _instance.isFixed():
				self.fixmenuitem.set_sensitive(False)
				self.freemenuitem.set_sensitive(True)
			else:
				self.fixmenuitem.set_sensitive(True)
				self.freemenuitem.set_sensitive(False)
		elif _instance.isRelation():
			_canpop = True
			self.propsmenuitem.set_sensitive(True)					
		elif _instance.isModel():
			# MODEL instances have a special context menu:
			_menu = self.get_model_context_menu(_instance)
			self.modelview.grab_focus()
			self.modelview.set_cursor(_path,_col,0)
			print "RUNNING POPUP MENU"
			_menu.popup(None,None,None,_button,event.time)
			return

		if _instance.isPlottable():
			self.plotmenuitem.set_sensitive(True)
			_canpop = True;
		else:
			self.plotmenuitem.set_sensitive(False)

		if not _canpop:
			return 

		self.modelview.grab_focus()
		self.modelview.set_cursor( _path, _col, 0)
		self.treecontext.popup( None, None, None, _button, event.time)
		return 1

	def get_model_context_menu(self,instance):
		menu = gtk.Menu()

		mi = gtk.ImageMenuItem("Run method...",False)
		mi.set_sensitive(False)
		img = gtk.Image()
		img.set_from_stock(gtk.STOCK_EXECUTE,gtk.ICON_SIZE_MENU)
		mi.set_image(img)
		mi.show()
		menu.append(mi)

		sep = gtk.SeparatorMenuItem(); sep.show()
		menu.append(sep)

		t = instance.getType()
		ml = t.getMethods()
		if len(ml):
			for m in ml:
				mi = gtk.MenuItem(m.getName(),False)
				mi.show()
				mi.connect("activate",self.run_activate,instance,m)
				menu.append(mi)		
		
		return menu

	def run_activate(self,widget,instance,method):
		print "RUNNING %s" % method.getName()
		self.browser.sim.run(method,instance)
		self.refreshtree()		

	def fix_activate(self,widget):
		_path,_col = self.modelview.get_cursor()
		_name, _instance = self.otank[_path]
		self.set_fixed(_instance,True);
		_instance.setFixed(True)
		return 1

	def free_activate(self,widget):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,False)
		return 1

	def plot_activate(self,widget):
	
		self.browser.reporter.reportNote("plot_activate...");
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if not _instance.isPlottable():
			self.browser.reporter.reportError("Can't plot instance %s" % _instance.getName().toString())
			return
		else:
			self.browser.reporter.reportNote("Instance %s about to be plotted..." % _instance.getName().toString())

		print("Plotting instance '%s'..." % _instance.getName().toString())

		_plot = _instance.getPlot()

		print "Title: ", _plot.getTitle()
		_plot.show(True)

		return 1

	def props_activate(self,widget,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if _instance.isRelation():
			print "Relation '"+_instance.getName().toString()+"':", \
				_instance.getRelationAsString(self.sim.getModel())
			_dia = RelPropsWin(self.browser,_instance);
			_dia.run();
		elif _instance.getType().isRefinedSolverVar():
			_dia = VarPropsWin(self.browser,_instance);
			_dia.run();
		else:
			self.browser.reporter.reportWarning("Select a variable first...")

	def observe_activate(self,widget,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if _instance.getType().isRefinedSolverVar():
			print "OBSERVING",_instance.getName().toString()		
			self.browser.observe(_instance)

	def on_fix_variable_activate(self,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,True)

	def on_free_variable_activate(self,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,False)

	def set_fixed(self,instance,val):
		if instance.getType().isRefinedSolverVar():
			f = instance.isFixed();
			if (f and not val) or (not f and val):
				instance.setFixed(val)
				self.browser.do_solve_if_auto()
