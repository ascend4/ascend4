from gi.repository import GdkPixbuf

from properties import *
from unitsdialog import *
from study import *

BROWSER_FIXED_COLOR = "#008800"
BROWSER_FREE_COLOR = "#000088"
BROWSER_SETTING_COLOR = "#4444AA"

BROWSER_INCLUDED_COLOR = "black"
BROWSER_UNINCLUDED_COLOR = "#888888"

ORIGINAL_PATH_INDEX = 7

class ModelView:
	def __init__(self,browser,builder):
		self.browser = browser # the parent object: the entire ASCEND browser

		self.builder = builder
		self.notes = browser.library.getAnnotationDatabase()	

		self.modelview = builder.get_object("browserview")

		self.otank = {}

		# name, type, value, foreground, weight, editable, status-icon
		columns = [str,str,str,str,int,bool,GdkPixbuf.Pixbuf,str]
		self.modelstore = Gtk.TreeStore(*columns)
		titles = ["Name","Type","Value"]
		self.modelview.set_model(self.modelstore)
		self.tvcolumns = [ Gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		
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
				renderer1 = Gtk.CellRendererPixbuf()
				tvcolumn.pack_start(renderer1, False)
				tvcolumn.add_attribute(renderer1, 'pixbuf', 6)

			renderer = Gtk.CellRendererText()
			tvcolumn.pack_start(renderer, True)
			tvcolumn.add_attribute(renderer, 'text', i)
			tvcolumn.add_attribute(renderer, 'foreground', 3)
			tvcolumn.add_attribute(renderer, 'weight', 4)
			if(i==2):
				tvcolumn.add_attribute(renderer, 'editable', 5)
				renderer.connect('edited',self.cell_edited_callback)
			i = i + 1

		#--------------------
		# get all menu icons and set up the context menu for fixing/freeing vars
		_imagelist = []
		for i in range(6):
			_imagelist.append("image%s" % (i+1))
		self.browser.builder.add_objects_from_file(self.browser.glade_file, _imagelist)
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["treecontext"])
		
		self.treecontext = self.browser.builder.get_object("treecontext")
		self.fixmenuitem = self.browser.builder.get_object("fix1")
		self.freemenuitem = self.browser.builder.get_object("free1")
		self.propsmenuitem = self.browser.builder.get_object("properties1")
		self.observemenuitem = self.browser.builder.get_object("observe1")
		self.studymenuitem = self.browser.builder.get_object("study1")
		self.unitsmenuitem = self.browser.builder.get_object("units1")
		self.hidevariable = self.browser.builder.get_object("hide_var")
		self.showallmenuitem = self.browser.builder.get_object("show_variables_all")
		self.hideallmenuitem = self.browser.builder.get_object("hide_variables_all")
		self.showmenuitem = self.browser.builder.get_object("show_variables")
		self.hidemenuitem = self.browser.builder.get_object("hide_variables")

		self.fixmenuitem.connect("activate",self.fix_activate)
		self.freemenuitem.connect("activate",self.free_activate)
		self.propsmenuitem.connect("activate",self.props_activate)
		self.observemenuitem.connect("activate",self.observe_activate)
		self.studymenuitem.connect("activate", self.study_activate)
		self.unitsmenuitem.connect("activate",self.units_activate)
		self.showallmenuitem.connect("activate", self.show_all_variables)
		self.hideallmenuitem.connect("activate", self.hide_all_variables)
		self.hidevariable.connect("activate", self.show_variable)

		self.variables = {"showed": [], "hidden": []}

		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")

	def setSimulation(self,sim):
		# instance hierarchy
		self.sim = sim
		self.modelstore.clear()
		self.otank = {} # map path -> (name,value)
		self.browser.disable_menu()
		try:
			self.make( self.sim.getName(),self.sim.getModel() )
			self.browser.enable_on_model_tree_build()
		except Exception as e:
			self.browser.reporter.reportError("Error building tree: %s" % e)

		self.fill_variables_menus()

		filtered_model = self.modelstore.filter_new()
		filtered_model.set_visible_func(self.filter_rows)
		self.modelview.set_model(filtered_model)
		self.modelview.expand_row(filtered_model.get_path(filtered_model.get_iter_first()), False)

		self.browser.maintabs.set_current_page(1)

	def fill_variables_menus(self):
		# show all variables
		vars = []
		for instance in list(self.otank.values()):
			if not str(instance[1].getType()) in vars:
				vars.append(str(instance[1].getType()))
		self.variables["showed"] = sorted(vars)
		for instype in self.variables["showed"]:
			menuitem = Gtk.MenuItem(instype)
			menuitem.connect("activate", self.show_variable)
			self.hidemenuitem.get_submenu().append(menuitem)
		self.hidemenuitem.show_all()
		self.hideallmenuitem.set_sensitive(True)

	def clear_variables_menus(self):
		self.variables = {"showed": [], "hidden": []}
		allitem = self.hidemenuitem.get_submenu().get_children()[0]
		for item in list(self.hidemenuitem.get_submenu().get_children()):
			self.hidemenuitem.get_submenu().remove(item)
		self.hidemenuitem.get_submenu().append(allitem)
		allitem.set_sensitive(False)
		allitem = self.showmenuitem.get_submenu().get_children()[0]
		for item in list(self.showmenuitem.get_submenu().get_children()):
			self.showmenuitem.get_submenu().remove(item)
		self.showmenuitem.get_submenu().append(allitem)
		allitem.set_sensitive(False)

	def show_all_variables(self, *args):
		for instype in list(self.variables["hidden"]):
			self.set_variable_visibility(instype, True)

		model = self.modelview.get_model()
		model.refilter()
		self.modelview.expand_row(model.get_path(model.get_iter_first()), False)

	def hide_all_variables(self, *args):
		for instype in list(self.variables["showed"]):
			self.set_variable_visibility(instype, False)

		self.modelview.get_model().refilter()

	def show_variable(self, widget):
		# if context menu
		if widget.get_label().startswith("Hide "):
			_model, _pathlist = self.modelview.get_selection().get_selected_rows()
			for _path in _pathlist:
				piter = _model.get_iter(_path)
				originalpath = _model.get_value(piter, ORIGINAL_PATH_INDEX)
				_, ins = self.otank[originalpath]
				self.set_variable_visibility(str(ins.getType()), False)
		# if main menu
		else:
			instype = widget.get_label()
			if widget in self.hidemenuitem.get_submenu().get_children():
				self.set_variable_visibility(instype, False)
			else:
				self.set_variable_visibility(instype, True)

		self.modelview.get_model().refilter()

	def set_variable_visibility(self, instype, show):
		if show:
			if instype in self.variables["hidden"]:
				self.variables["hidden"].remove(instype)
			self.variables["showed"].append(instype)
			menuitem = None
			for item in self.showmenuitem.get_submenu().get_children():
				if item.get_label() == instype:
					menuitem = item
					break
			if menuitem is not None:
				self.showmenuitem.get_submenu().remove(menuitem)
				self.hidemenuitem.get_submenu().insert(menuitem, self.get_menu_position(menuitem, self.hidemenuitem))

		else:
			if instype in self.variables["showed"]:
				self.variables["showed"].remove(instype)
			self.variables["hidden"].append(instype)
			menuitem = None
			for item in self.hidemenuitem.get_submenu().get_children():
				if item.get_label() == instype:
					menuitem = item
					break
			if menuitem is not None:
				self.hidemenuitem.get_submenu().remove(menuitem)
				self.showmenuitem.get_submenu().insert(menuitem, self.get_menu_position(menuitem, self.showmenuitem))

		self.hideallmenuitem.set_sensitive(len(self.variables["showed"]) > 0)
		self.showallmenuitem.set_sensitive(len(self.variables["hidden"]) > 0)

	def get_menu_position(self, menuitem, menu):
		children = menu.get_submenu().get_children()
		for i in range(1, len(children)):
			if children[i].get_label() > menuitem.get_label():
				return i

		return len(children)

	def filter_rows(self, model, piter, data):
		path = model.get_path(piter)
		if str(path) not in self.otank:
			return False

		name, value = self.otank[path.to_string()]
		instype = str(value.getType())
		return instype in self.variables["showed"]

	def clear(self):
		self.clear_variables_menus()
		self.modelstore.clear()
		self.otank = {}

#   --------------------------------------------
#   INSTANCE TREE

	def get_tree_row_data(self,instance): # for instance browser
		_value = str(instance.getValue())
		_type = str(instance.getType())
		_name = str(instance.getName())
		_fgcolor = BROWSER_INCLUDED_COLOR
		_fontweight = Pango.Weight.NORMAL
		_editable = False
		_statusicon = None
		if instance.getType().isRefinedSolverVar():
			_editable = True
			_fontweight = Pango.Weight.BOLD
			if instance.isFixed():
				_fgcolor = BROWSER_FIXED_COLOR
			else:
				_fgcolor = BROWSER_FREE_COLOR
				_fontweight = Pango.Weight.BOLD
			_status = instance.getStatus();
			_statusicon = self.browser.statusicons[_status]
		elif instance.isRelation():
			_status = instance.getStatus();
			_statusicon = self.browser.statusicons[_status]
			if not instance.isIncluded():
				_fgcolor = BROWSER_UNINCLUDED_COLOR
		elif instance.isBool() or instance.isReal() or instance.isInt():
			# TODO can't edit constants that have already been refined
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = Pango.Weight.BOLD
		elif instance.isSymbol() and not instance.isConst():
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = Pango.Weight.BOLD

		#if(len(_value) > 80):
		#	_value = _value[:80] + "..."

		return [_name, _type, _value, _fgcolor, _fontweight, _editable, _statusicon, None]

	def make_row(self, piter, value, name=None): # for instance browser
		assert(value)
		_piter = self.modelstore.append(piter, self.get_tree_row_data(value))
		path = self.modelstore.get_path(_piter)
		self.modelstore.set_value(_piter, ORIGINAL_PATH_INDEX, str(path))
		if name is not None:
			self.modelstore.set_value(_piter, 0, str(name))
		return _piter

	def refreshtree(self):
		# @TODO FIXME use a better system than colour literals!
		for _path in self.otank: # { path : (name,value) }
			_iter = self.modelstore.get_iter(_path)
			_name, _instance = self.otank[_path]
			_value = str(_instance.getValue())
			##### CELSIUS TEMPERATURE WORKAROUND
			_value = CelsiusUnits.convert_show(_instance, _value, True)
			##### CELSIUS TEMPERATURE WORKAROUND
			self.modelstore.set_value(_iter, 2, _value)
			if _instance.getType().isRefinedSolverVar():
				if _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FREE_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR)
				elif not _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FIXED_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FREE_COLOR)
				if self.browser.statusicons[_instance.getStatus()] != None:
					self.modelstore.set_value(_iter, 6, self.browser.statusicons[_instance.getStatus()])
			elif _instance.isRelation():
				if self.browser.statusicons[_instance.getStatus()] != None:
					self.modelstore.set_value(_iter, 6, self.browser.statusicons[_instance.getStatus()])
				if _instance.isIncluded():
					self.modelstore.set_value(_iter,3,BROWSER_INCLUDED_COLOR)
				else:
					self.modelstore.set_value(_iter,3,BROWSER_UNINCLUDED_COLOR)

	def get_selected_type(self):
		return self.get_selected_instance().getType()

	def get_selected_instance(self):
		model, pathlist = self.modelview.get_selection().get_selected_rows()
		if len(pathlist) == 0:
			return None

		piter = self.modelview.get_model().get_iter(pathlist[0])
		originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
		name, instance = self.otank[originalpath]
		return instance

	def cell_edited_callback(self, renderer, path, newtext, **kwargs):
		# get back the Instance object we just edited (having to use this seems like a bug)
		#path = tuple( map(int,path.split(":")) )
		piter = self.modelview.get_model().get_iter(path)
		originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
		if originalpath not in self.otank:
			raise RuntimeError("cell_edited_callback: invalid path '%s'" % path)

		_name, _instance = self.otank[originalpath]

		if _instance.isReal():
			if _instance.getValue() == newtext:
				return True
			# only real-valued things can have units

			##### CELSIUS TEMPERATURE WORKAROUND
			newtext = CelsiusUnits.convert_edit(_instance, newtext, True)
			##### CELSIUS TEMPERATURE WORKAROUND

			_e = RealAtomEntry(_instance, newtext)
			try:
				_e.checkEntry()
				_e.setValue()
				_e.exportPreferredUnits(self.browser.prefs)
			except InputError as e:
				self.browser.reporter.reportError(str(e))
				return True

		else:
			if _instance.isBool():
				_lower = newtext.lower();
				if _lower.startswith("t") or _lower.startswith("y") or _lower.strip()=="1":
					newtext = 1
				elif _lower.startswith("f") or _lower.startswith("n") or _lower.strip()=="0":
					newtext = 0
				else:
					self.browser.reporter.reportError("Invalid entry for a boolean variable: '%s'" % newtext)
					return True
				_val = bool(newtext);
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Boolean atom '%s' was not altered" % _instance.getName())
					return True
				_instance.setBoolValue(_val)

			elif _instance.isInt():
				_val = int(newtext)
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Integer atom '%s' was not altered" % _instance.getName())
					return True
				_instance.setIntValue(_val)
			elif _instance.isSymbol():
				_val = str(newtext)
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Symbol atom '%s' was not altered" % _instance.getName())
					return True
				_instance.setSymbolValue(ascpy.SymChar(_val))
						
			else:
				self.browser.reporter.reportError("Attempt to set a non-real, non-boolean, non-integer value!")
				return True

		# now that the variable is set, update the GUI and re-solve if desired
		_iter = self.modelstore.get_iter(path)
		self.modelstore.set_value(_iter,2, str(_instance.getValue()))

		if _instance.getType().isRefinedSolverVar():
			self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR) # set the row green as fixed

		self.browser.do_solve_if_auto()
		for _obs in self.browser.observers:
			if _obs.alive:
				_obs.units_refresh(self.get_selected_instance().getType())
		return True

	##### EXTERNAL RELATION WORKAROUND
	def get_external_relation_outputs(self, value):
		relation = str(value.getRelationAsString(self.browser.sim.getModel()))
		relation = relation[relation.find('(') + 1:relation.find(')')]
		relation = relation.replace(',', '').replace(';', '')
		params = relation.split('\n')
		result = []
		for r in params:
			if "OUTPUT" in r:
				result.append(r.split("OUTPUT")[0].strip())
		return result
	##### EXTERNAL RELATION WORKAROUND

	def make_children(self, value, piter, depth=5):
		assert(value)
		if value.isCompound():
			children=value.getChildren();
			##### EXTERNAL RELATION WORKAROUND
			index = 0
			relation_outputs = None
			##### EXTERNAL RELATION WORKAROUND
			for child in children:
				try:
					_name = child.getName()
					##### EXTERNAL RELATION WORKAROUND
					if str(value.getType().getName()) == "array" and str(child.getType().getName()) == "relation":
						if relation_outputs is None:
							relation_outputs = self.get_external_relation_outputs(child)

						if index < len(relation_outputs):
							_name = relation_outputs[index]
							index += 1
					##### EXTERNAL RELATION WORKAROUND
					_piter = self.make_row(piter, child, _name)
					if child.isCompound() and len(child.getChildren()) > 0 and depth > 0:
						self.make_children(child, _piter, depth - 1)
					_path = self.modelstore.get_path(_piter)
					self.otank[_path.to_string()] = (child.getName(), child)
					#self.browser.reporter.reportError("2 Added %s at path %s" % (_name,repr(_path)))
				except Exception as e:
					self.browser.reporter.reportError("%s: %s" % (_name,e))
	

	def make(self, name=None, value=None, path=None, depth=1):
		if path is None:
			# make root node
			piter = self.make_row(None, value)
			path = self.modelstore.get_path( piter )
			self.otank[ path.to_string() ] = (name, value)
			#self.browser.reporter.reportError("4 Added %s at path %s" % (name, path))
		else:
			name, value = self.otank[ path.to_string() ]

		assert(value)

		piter = self.modelstore.get_iter( path )
		if not self.modelstore.iter_has_child( piter ):
			#self.browser.reporter.reportNote( "name=%s has CHILDREN..." % name )
			self.make_children(value,piter)

		if depth:
			for i in range( self.modelstore.iter_n_children( piter ) ):
				tmp_path = path.copy()
				tmp_path.append_index(i)
				if tmp_path.to_string() not in list(self.otank.keys()):
					continue

				self.make(path=tmp_path, depth=depth - 1)
		else:
			self.modelview.expand_row(self.modelstore.get_path(self.modelstore.get_iter_first()),False) # Edit here only.

	def row_expanded(self, modelview, piter, path):
		originalpath = Gtk.TreePath.new_from_string(modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX))
		self.make(path=originalpath)


#   ------------------------------
#   CONTEXT MENU

	def on_treeview_event(self,widget,event):
		_path = None
		_contextmenu = False
		if event.type == Gdk.EventType.KEY_PRESS:
			_keyval = Gdk.keyval_name(event.keyval)
			_path, _col = self.modelview.get_cursor()
			if _keyval == 'Menu':
				_contextmenu = True
				_button = 3
			elif _keyval == 'F2' or _keyval == 'Return':
				print("F2 pressed")
				self.modelview.set_cursor(_path, self.tvcolumns[2], 1)
				return True
			elif event.keyval == Gdk.KEY_f and (event.state & Gdk.ModifierType.CONTROL_MASK):
				#print("Ctrl+F pressed - perform your action here")
				self.fix_activate(widget)
				return True  # Prevents further handling (i.e., blocks the default search behavior)
			elif event.keyval == Gdk.KEY_r and (event.state & Gdk.ModifierType.CONTROL_MASK):
				#print("Ctrl+R pressed - perform your action here")
				self.free_activate(widget)
				return True
		
		elif event.type == Gdk.EventType.BUTTON_PRESS:
			_x = int(event.x)
			_y = int(event.y)
			_button = event.button
			_pthinfo = self.modelview.get_path_at_pos(_x, _y)
			if _pthinfo is not None:
				_path, _col, _cellx, _celly = _pthinfo
				if event.button == 3:
					_contextmenu = True

		if not _contextmenu:
			#print "NOT DOING ANYTHING ABOUT %s" % Gdk.keyval_name(event.keyval)
			return False

		if _path:
			piter = self.modelview.get_model().get_iter(_path)
			originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
			_name, _instance = self.otank[originalpath]
			# set the statusbar
			nn = self.notes.getNotes(self.sim.getModel().getType(),ascpy.SymChar("inline"),_name)
			for n in nn:
				print("%s: (%s) %s" % (n.getId(),str(n.getLanguage()),n.getText()))
		
			self.builder.get_object("free_variable").set_sensitive(False)
			self.builder.get_object("fix_variable").set_sensitive(False)
			self.builder.get_object("propsmenuitem").set_sensitive(False)
			if _instance.isReal():
				self.builder.get_object("units").set_sensitive(True)
			if _instance.getType().isRefinedSolverVar():
				self.builder.get_object("propsmenuitem").set_sensitive(True)
				if _instance.isFixed():
					self.builder.get_object("free_variable").set_sensitive(True)
				else:
					self.builder.get_object("fix_variable").set_sensitive(True)
			elif _instance.isRelation():
				self.builder.get_object("propsmenuitem").set_sensitive(True)

		self.unitsmenuitem.set_sensitive(False)
		self.fixmenuitem.set_sensitive(False)
		self.freemenuitem.set_sensitive(False)
		self.observemenuitem.set_sensitive(False)
		self.studymenuitem.set_sensitive(False)
		self.propsmenuitem.set_sensitive(False)					
		self.hidevariable.set_sensitive(False)

		# if selected more than one row
		model, pathlist = self.modelview.get_selection().get_selected_rows()
		if len(pathlist) > 1 and _path in pathlist:
			_fixed = False
			_free = False
			_observe = False
			for p in pathlist:
				piter = self.modelview.get_model().get_iter(p)
				originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
				_name, _instance = self.otank[originalpath]
				if _instance.getType().isRefinedSolverVar():
					_fixed |= _instance.isFixed()
					_free |= not _instance.isFixed()
					_observe = True
			if _fixed:
				self.freemenuitem.set_sensitive(True)
			if _free:
				self.fixmenuitem.set_sensitive(True)
			if _observe:
				self.observemenuitem.set_sensitive(True)

			self.hidevariable.set_sensitive(True)
			self.hidevariable.set_label("Hide selected types")

			self.modelview.grab_focus()
			self.treecontext.popup(None, None, None, None, _button, event.time)
			return True

		if _instance.isReal():
			print("CAN POP: real atom")
			self.unitsmenuitem.set_sensitive(True)

		if _instance.getType().isRefinedSolverVar():
			self.propsmenuitem.set_sensitive(True)
			self.observemenuitem.set_sensitive(True)
			if _instance.isFixed():
				self.freemenuitem.set_sensitive(True)
				if len(self.browser.observers) > 0:
					self.studymenuitem.set_sensitive(True)
			else:
				self.fixmenuitem.set_sensitive(True)
		elif _instance.isRelation() or _instance.isLogicalRelation() or _instance.isWhen():
			self.propsmenuitem.set_sensitive(True)
		elif _instance.isModel():
			# MODEL instances have a special context menu:
			self.modelmenu = self.get_model_context_menu(_instance)
			self.modelview.grab_focus()
			self.modelview.set_cursor(_path,_col,0)
			print("RUNNING POPUP MENU")
			self.modelmenu.popup(None, None, None, None, _button, event.time)
			return True

		self.hidevariable.set_label("Hide " + str(_instance.getType()))
		self.hidevariable.set_sensitive(True)

		self.modelview.grab_focus()
		self.modelview.set_cursor( _path, _col, 0)
		self.treecontext.popup( None, None, None,None, _button, event.time)
		return True

	def get_model_context_menu(self,instance):
		menu = Gtk.Menu()
		
		if instance.isPlottable():
			print("PLOTTABLE")
			mi = Gtk.ImageMenuItem("P_lot",True);
			img = Gtk.Image()
			img.set_from_file(self.browser.options.assets_dir+'/plot.png')
			mi.set_image(img)
			mi.show()
			mi.connect("activate",self.plot_activate)
			menu.append(mi);
			sep = Gtk.SeparatorMenuItem(); sep.show()
			menu.append(sep)
		
		mi = Gtk.ImageMenuItem("Run method...")
		mi.set_sensitive(False)
		img = Gtk.Image()
		img.set_from_stock(Gtk.STOCK_EXECUTE,Gtk.IconSize.MENU)
		mi.set_image(img)
		mi.show()
		menu.append(mi)

		sep = Gtk.SeparatorMenuItem(); sep.show()
		menu.append(sep)

		t = instance.getType()
		ml = t.getMethods()
		if len(ml):
			for m in ml:
				mi = Gtk.MenuItem(m.getName())
				mi.show()
				mi.connect("activate",self.run_activate,instance,m)
				menu.append(mi)		
		
		return menu

	def run_activate(self,widget,instance,method):
		print("RUNNING %s" % method.getName())
		try:
			self.browser.sim.run(method,instance)
		except Exception as e:
			self.browser.reporter.reportError(str(e))
		self.refreshtree()		

	def fix_activate(self,widget):
		self.browser.reporter.reportNote("Fixing variable")
		_model, _pathlist = self.modelview.get_selection().get_selected_rows()
		for _path in _pathlist:
			piter = self.modelview.get_model().get_iter(_path)
			originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
			_name, _instance = self.otank[originalpath]
			self.set_fixed(_instance, True)
		self.browser.do_solve_if_auto()
		return 1

	def free_activate(self,widget):
		self.browser.reporter.reportNote("Freeing variable")
		_model, _pathlist = self.modelview.get_selection().get_selected_rows()
		for _path in _pathlist:
			piter = self.modelview.get_model().get_iter(_path)
			originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
			_name, _instance = self.otank[originalpath]
			self.set_fixed(_instance, False)
		self.browser.do_solve_if_auto()
		return 1

	def plot_activate(self,widget):

		self.browser.reporter.reportNote("plot_activate...");
		_path,_col = self.modelview.get_cursor()
		piter = self.modelview.get_model().get_iter(_path)
		originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
		_instance = self.otank[originalpath][1]
		if not _instance.isPlottable():
			self.browser.reporter.reportError("Can't plot instance %s" % _instance.getName().toString())
			return
		else:
			self.browser.reporter.reportNote("Instance %s about to be plotted..." % _instance.getName().toString())

		print(("Plotting instance '%s'..." % _instance.getName().toString()))

		_plot = _instance.getPlot()

		print("Title: ", _plot.getTitle())
		_plot.show(True)

		return 1

	def props_activate(self,widget,*args):
		if not hasattr(self,'sim'):
			self.browser.reporter.reportError("Can't show properties until a simulation has been created.");
			return
		_path,_col = self.modelview.get_cursor()
		piter = self.modelview.get_model().get_iter(_path)
		originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
		_instance = self.otank[originalpath][1]
		if _instance.isRelation() or _instance.isLogicalRelation() or _instance.isWhen():
			# print "Relation '"+_instance.getName().toString()+"':", \
			# 	_instance.getRelationAsString(self.sim.getModel())
			_dia = RelPropsWin(self.browser, _instance)
			_dia.run()
		elif _instance.getType().isRefinedSolverVar():
			_dia = VarPropsWin(self.browser, _instance)
			_dia.run()
		else:
			self.browser.reporter.reportWarning("Select a variable or relation first...")

	def observe_activate(self,widget,*args):
		_model, _pathlist = self.modelview.get_selection().get_selected_rows()
		for _path in _pathlist:
			piter = self.modelview.get_model().get_iter(_path)
			originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
			_name, _instance = self.otank[originalpath]
			if _instance.getType().isRefinedSolverVar():
				print("OBSERVING",_instance.getName().toString())
				self.browser.observe(_instance)

	def on_fix_variable_activate(self, widget):
		self.fix_activate(widget)

	def on_free_variable_activate(self, widget):
		self.free_activate(widget)

	def set_fixed(self,instance,val):
		if instance.getType().isRefinedSolverVar():
			f = instance.isFixed()
			if (f and not val) or (not f and val):
				instance.setFixed(val)


	def study_activate(self, *args):
		_path,_col = self.modelview.get_cursor()
		piter = self.modelview.get_model().get_iter(_path)
		originalpath = self.modelview.get_model().get_value(piter, ORIGINAL_PATH_INDEX)
		_instance = self.otank[originalpath][1]
		self.browser.observe(_instance)
		_dia = StudyWin(self.browser,_instance)
		_dia.run()

	def units_activate(self,*args):
		T = self.get_selected_type()
		try:
			_un = UnitsDialog(self.browser,T)
			_un.run()
		except:
			self.browser.reporter.reportError("Unable to display units dialog.")

