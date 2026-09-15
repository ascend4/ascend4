import os

from gi.repository import Gdk, GdkPixbuf, Gtk, Pango

from properties import *
from unitsdialog import *
from study import *

BROWSER_FIXED_COLOR = "#008800"
BROWSER_FREE_COLOR = "#000088"
BROWSER_SETTING_COLOR = "#4444AA"

BROWSER_INCLUDED_COLOR = "black"
BROWSER_UNINCLUDED_COLOR = "#888888"

BLOCK_INDEX = 7
ORIGINAL_PATH_INDEX = 8
MODEL_ORDER_INDEX = 9
BLOCK_KIND_INDEX = 10
BLOCK_LOW_INDEX = 11
BLOCK_HIGH_INDEX = 12
BLOCK_POSITION_LOW_INDEX = 13
BLOCK_POSITION_HIGH_INDEX = 14

BLOCK_UNKNOWN = 0
BLOCK_FIXED = 1
BLOCK_NUMBERED = 2

class ModelView:
	def __init__(self,browser,builder):
		self.browser = browser # the parent object: the entire ASCEND browser

		self.builder = builder
		self.notes = browser.library.getAnnotationDatabase()	

		self.modelview = builder.get_object("browserview")

		self.otank = {}
		self.solver_var_blocks = {}
		self.solver_rel_blocks = {}
		self.solver_var_order = {}
		self.solver_rel_order = {}
		self.solver_block_sizes = {}
		self.solver_fixed_vars = set()
		self.declaration_file_order = {}
		self.variables = {"shown": set(), "hidden": set()}

		# name, type, value, foreground, weight, editable, status-icon,
		# solver block, original tree-store path, calculated model order,
		# block kind, numeric block extent, and positions within the lowest and
		# highest contained blocks.
		columns = [
			str,str,str,str,int,bool,GdkPixbuf.Pixbuf,str,str,int,int,int,int,int,int
		]
		self.modelstore = Gtk.TreeStore(*columns)
		titles = ["Name","Type","Value"]
		self.modelview.set_model(self.modelstore)
		self.tvcolumns = [ Gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		
		self.modelview.connect("row-expanded", self.row_expanded )
		self.modelview.connect("button-press-event", self.on_treeview_event )
		self.modelview.connect("key-press-event",self.on_treeview_event )
		
		self.modelview.set_has_tooltip(True)
		self.last_tooltip_path = None
		self.modelview.connect("query-tooltip",self.on_query_tooltip)

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
				self.valuerenderer = renderer
				renderer.connect('edited',self.cell_edited_callback)
				renderer.connect('editing-started', self.cell_editing_started_callback)
			i = i + 1

		# Let the Value column absorb spare horizontal space. Otherwise GTK gives
		# it to the final column, separating that column's sort arrow from its
		# heading when the browser window is wide.
		self.tvcolumns[2].set_expand(True)

		# Block numbers use the same zero-based numbering as Diagnose Blocks.
		# Unsupported instance kinds and solvers without a block decomposition
		# are deliberately displayed as blank cells.
		self.blockcolumn = Gtk.TreeViewColumn("Block")
		_blockrenderer = Gtk.CellRendererText()
		self.blockcolumn.pack_start(_blockrenderer, True)
		self.blockcolumn.add_attribute(_blockrenderer, 'text', BLOCK_INDEX)
		self.blockcolumn.set_expand(False)
		self.modelview.append_column(self.blockcolumn)
		self.showblocksmenuitem = self.browser.builder.get_object("show_tree_blocks")
		_show_blocks = self.browser.prefs.getBoolPref("Browser", "show_tree_blocks", True)
		self.blockcolumn.set_visible(_show_blocks)
		if self.showblocksmenuitem is not None:
			self.showblocksmenuitem.set_active(_show_blocks)
			self.showblocksmenuitem.connect("toggled", self.on_show_tree_blocks_toggled)
		self.relationslastmenuitem = self.browser.builder.get_object("relations_appear_last")
		self.relations_appear_last = self.browser.prefs.getBoolPref(
			"Browser", "relations_appear_last", True
		)
		if self.relationslastmenuitem is not None:
			self.relationslastmenuitem.set_active(self.relations_appear_last)
			self.relationslastmenuitem.connect(
				"toggled", self.on_relations_appear_last_toggled
			)

		# Keep sorting outside the TreeStore. Its paths are used as stable keys
		# into otank, while the filter and sort models may freely rearrange the
		# paths presented by the TreeView.
		self.filtered_model = self.modelstore.filter_new()
		self.filtered_model.set_visible_func(self.filter_rows)
		self.sort_model = Gtk.TreeModelSort.new_with_model(self.filtered_model)
		self.sort_model.set_default_sort_func(self.compare_model_order, None)
		self.sort_model.set_sort_func(0, self.compare_names, None)
		self.sort_model.set_sort_func(BLOCK_LOW_INDEX, self.compare_blocks, None)
		self.modelview.set_model(self.sort_model)
		self.tvcolumns[0].set_sort_column_id(0)
		self.blockcolumn.set_sort_column_id(BLOCK_LOW_INDEX)
		# A third click on an active header returns to declaration/model order.
		# Restore the last-selected mode on startup, defaulting to Name ascending.
		_sort_mode = self.browser.prefs.getStringPref("Browser", "tree_sort", "name")
		_sort_order = (
			Gtk.SortType.DESCENDING
			if self.browser.prefs.getBoolPref("Browser", "tree_sort_descending", False)
			else Gtk.SortType.ASCENDING
		)
		if _sort_mode == "block":
			self.sort_model.set_sort_column_id(BLOCK_LOW_INDEX, _sort_order)
		elif _sort_mode == "model":
			self.sort_model.set_sort_column_id(
				Gtk.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk.SortType.ASCENDING
			)
		else:
			self.sort_model.set_sort_column_id(0, _sort_order)
		self.sort_model.connect(
			"sort-column-changed", self.on_tree_sort_column_changed
		)

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

		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")

	@staticmethod
	def compare_values(left, right):
		return (left > right) - (left < right)

	def refresh_declaration_file_order(self):
		self.declaration_file_order = {}
		try:
			modules = self.browser.library.getModules()
		except RuntimeError:
			return
		for module in modules:
			filename = str(module.getFilename())
			if not filename:
				continue
			filename = os.path.realpath(filename)
			if filename not in self.declaration_file_order:
				self.declaration_file_order[filename] = len(self.declaration_file_order)

	def model_order_key(self, model, piter):
		order = model.get_value(piter, MODEL_ORDER_INDEX)
		name = model.get_value(piter, 0)
		return (order, name.casefold(), name)

	def child_model_orders(self, parent, children):
		"""Calculate source/model order only for this materialised sibling set."""
		entries = []
		for fallback_order, child in enumerate(children):
			try:
				filename = str(child.getDeclarationFilename(parent))
				line = int(child.getDeclarationLine(parent))
			except (AttributeError, RuntimeError):
				filename = ""
				line = 0
			if filename and line > 0:
				normalized = os.path.realpath(filename)
				file_rank = self.declaration_file_order.get(
					normalized, len(self.declaration_file_order)
				)
				key = (
					0, file_rank,
					normalized if normalized not in self.declaration_file_order else "",
					line, fallback_order,
				)
			else:
				key = (1, fallback_order)
			entries.append((key, fallback_order))
		entries.sort()
		return {
			child_index: model_order
			for model_order, (_key, child_index) in enumerate(entries)
		}

	def compare_model_order(self, model, left, right, _data):
		return self.compare_values(
			self.model_order_key(model, left), self.model_order_key(model, right)
		)

	def compare_names(self, model, left, right, _data):
		left_name = model.get_value(left, 0)
		right_name = model.get_value(right, 0)
		left_group = 1 if self.row_is_relation(model, left) else 0
		right_group = 1 if self.row_is_relation(model, right) else 0
		_sort_column, order = self.sort_model.get_sort_column_id()
		if not self.relations_appear_last:
			left_group = right_group = 0
		elif order == Gtk.SortType.DESCENDING:
			# GTK reverses the comparator for descending sorts. Reverse only the
			# group here so relation rows remain last while names reverse normally.
			left_group = -left_group
			right_group = -right_group
		return self.compare_values(
			(left_group, left_name.casefold(), left_name),
			(right_group, right_name.casefold(), right_name),
		)

	def row_instance(self, model, piter):
		path = model.get_value(piter, ORIGINAL_PATH_INDEX)
		entry = self.otank.get(path)
		return entry[1] if entry is not None else None

	def row_is_relation(self, model, piter):
		instance = self.row_instance(model, piter)
		try:
			return instance is not None and instance.isRelation()
		except RuntimeError:
			return False

	def row_is_compound(self, model, piter):
		instance = self.row_instance(model, piter)
		try:
			return instance is not None and instance.isCompound()
		except RuntimeError:
			return False

	def block_sort_key(self, model, piter, descending):
		kind = model.get_value(piter, BLOCK_KIND_INDEX)
		low = model.get_value(piter, BLOCK_LOW_INDEX)
		high = model.get_value(piter, BLOCK_HIGH_INDEX)
		if kind == BLOCK_NUMBERED:
			group = 0 if descending else 1
			block = -high if descending else low
			if self.relations_appear_last:
				detail = (
					1 if self.row_is_relation(model, piter) else 0,
					self.model_order_key(model, piter),
				)
			else:
				position_column = (
					BLOCK_POSITION_HIGH_INDEX
					if descending else BLOCK_POSITION_LOW_INDEX
				)
				detail = (
					model.get_value(piter, position_column),
					0 if self.row_is_compound(model, piter) else 1,
					self.model_order_key(model, piter),
				)
		elif kind == BLOCK_FIXED:
			group = 1 if descending else 0
			block = 0
			detail = self.model_order_key(model, piter)
		else:
			group = 2
			block = 0
			detail = self.model_order_key(model, piter)
		return (group, block, detail)

	def compare_blocks(self, model, left, right, _data):
		_sort_column, order = self.sort_model.get_sort_column_id()
		descending = order == Gtk.SortType.DESCENDING
		result = self.compare_values(
			self.block_sort_key(model, left, descending),
			self.block_sort_key(model, right, descending),
		)
		# Gtk reverses the comparator result for descending sorts. The block
		# comparator has already selected highest extents and kept unknown rows
		# last, so compensate for that final reversal.
		return -result if descending else result

	def setSimulation(self,sim):
		# instance hierarchy
		self.sim = sim
		self.modelstore.clear()
		self.otank = {} # map path -> (name,value)
		self.solver_var_blocks = {}
		self.solver_rel_blocks = {}
		self.solver_var_order = {}
		self.solver_rel_order = {}
		self.solver_block_sizes = {}
		self.solver_fixed_vars = set()
		self.declaration_file_order = {}
		self.refresh_declaration_file_order()
		self.browser.disable_menu()
		try:
			self.make( self.sim.getName(),self.sim.getModel() )
			self.browser.enable_on_model_tree_build()
		except Exception as e:
			self.browser.reporter.reportError("Error building tree: %s" % e)

		self.fill_variables_menus()

		self.filtered_model.refilter()
		root = self.sort_model.get_iter_first()
		if root is not None:
			self.modelview.expand_row(self.sort_model.get_path(root), False)

		self.browser.maintabs.set_current_page(1)

	def fill_variables_menus(self):
		# show all variables
		vars = set()
		for instance in list(self.otank.values()):
			vars.add(str(instance[1].getType()))
		self.variables["shown"] = sorted(vars)
		for instype in self.variables["shown"]:
			menuitem = Gtk.MenuItem(instype)
			menuitem.connect("activate", self.show_variable)
			self.hidemenuitem.get_submenu().append(menuitem)
		self.hidemenuitem.show_all()
		self.hideallmenuitem.set_sensitive(True)

	def clear_variables_menus(self):
		self.variables = {"shown": set(), "hidden": set()}
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

		self.filtered_model.refilter()
		model = self.modelview.get_model()
		root = model.get_iter_first()
		if root is not None:
			self.modelview.expand_row(model.get_path(root), False)

	def hide_all_variables(self, *args):
		for instype in list(self.variables["shown"]):
			self.set_variable_visibility(instype, False)

		self.filtered_model.refilter()

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

		self.filtered_model.refilter()

	def set_variable_visibility(self, instype, show):
		if show:
			if instype in self.variables["hidden"]:
				self.variables["hidden"].remove(instype)
			self.variables["shown"].append(instype)
			menuitem = None
			for item in self.showmenuitem.get_submenu().get_children():
				if item.get_label() == instype:
					menuitem = item
					break
			if menuitem is not None:
				self.showmenuitem.get_submenu().remove(menuitem)
				self.hidemenuitem.get_submenu().insert(menuitem, self.get_menu_position(menuitem, self.hidemenuitem))

		else:
			if instype in self.variables["shown"]:
				self.variables["shown"].remove(instype)
			self.variables["hidden"].add(instype)
			menuitem = None
			for item in self.hidemenuitem.get_submenu().get_children():
				if item.get_label() == instype:
					menuitem = item
					break
			if menuitem is not None:
				self.hidemenuitem.get_submenu().remove(menuitem)
				self.showmenuitem.get_submenu().insert(menuitem, self.get_menu_position(menuitem, self.showmenuitem))

		self.hideallmenuitem.set_sensitive(len(self.variables["shown"]) > 0)
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
		insttype = str(value.getType())

		if value.isRelation() and 'relation' not in self.variables["shown"]:
			return False
		elif value.isAtom() and insttype not in self.variables["shown"]:
			return False
		elif value.isArray() and 'array' not in self.variables["shown"]:
			return False
		else:
			return True

	def clear(self):
		self.clear_variables_menus()
		self.modelstore.clear()
		self.otank = {}
		self.solver_var_blocks = {}
		self.solver_rel_blocks = {}
		self.solver_var_order = {}
		self.solver_rel_order = {}
		self.solver_block_sizes = {}
		self.solver_fixed_vars = set()
		self.declaration_file_order = {}

#   --------------------------------------------
#   INSTANCE TREE

	def get_tree_row_data(self,instance): # for instance browser
		_value = self.browser.get_instance_display_value(instance)
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
		elif instance.isSelector():
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = Pango.Weight.BOLD
		elif instance.isSymbol() and not instance.isConst():
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = Pango.Weight.BOLD

		#if(len(_value) > 80):
		#	_value = _value[:80] + "..."

		return [
			_name, _type, _value, _fgcolor, _fontweight, _editable,
			_statusicon, "", None, 0, BLOCK_UNKNOWN, 0, 0, 0, 0
		]

	def make_row(self, piter, value, name=None, parent=None, model_order=0): # for instance browser
		assert(value)
		_piter = self.modelstore.append(piter, self.get_tree_row_data(value))
		path = self.modelstore.get_path(_piter)
		self.modelstore.set_value(_piter, ORIGINAL_PATH_INDEX, str(path))
		self.modelstore.set_value(_piter, MODEL_ORDER_INDEX, model_order)
		self.set_direct_block_data(_piter, value)
		if name is not None:
			self.modelstore.set_value(_piter, 0, str(name))
		return _piter

	def refreshtree(self):
		# @TODO FIXME use a better system than colour literals!
		for _path in self.otank: # { path : (name,value) }
			_iter = self.modelstore.get_iter(_path)
			_name, _instance = self.otank[_path]
			_value = self.browser.get_instance_display_value(_instance)
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

		sort_column, _order = self.sort_model.get_sort_column_id()
		if self.blockcolumn.get_visible() or sort_column == BLOCK_LOW_INDEX:
			self.refresh_solver_blocks()

	def on_show_tree_blocks_toggled(self, widget):
		visible = widget.get_active()
		self.blockcolumn.set_visible(visible)
		self.browser.prefs.setBoolPref("Browser", "show_tree_blocks", visible)
		if visible:
			self.refresh_solver_blocks()

	def on_relations_appear_last_toggled(self, widget):
		self.relations_appear_last = widget.get_active()
		self.browser.prefs.setBoolPref(
			"Browser", "relations_appear_last", self.relations_appear_last
		)
		if self.solver_var_blocks or self.solver_rel_blocks:
			self.apply_cached_block_data()
		self.sort_model.sort_column_changed()

	def on_tree_sort_column_changed(self, sortable):
		sort_column, order = sortable.get_sort_column_id()
		if sort_column == BLOCK_LOW_INDEX:
			mode = "block"
		elif sort_column == 0:
			mode = "name"
		else:
			mode = "model"
		self.browser.prefs.setStringPref("Browser", "tree_sort", mode)
		self.browser.prefs.setBoolPref(
			"Browser", "tree_sort_descending", order == Gtk.SortType.DESCENDING
		)

	def refresh_solver_blocks(self):
		"""Refresh leaf block labels and compound-row block extents."""
		self.solver_var_blocks = {}
		self.solver_rel_blocks = {}
		self.solver_var_order = {}
		self.solver_rel_order = {}
		self.solver_block_sizes = {}
		self.solver_fixed_vars = set()
		for _path in self.otank:
			_iter = self.modelstore.get_iter(_path)
			self.clear_block_data(_iter)

		if not hasattr(self, 'sim') or self.sim is None:
			return

		try:
			im = self.sim.getIncidenceMatrix()
			nblocks = im.getNumBlocks()
		except (AttributeError, RuntimeError, IndexError):
			# A system that has not been presolved, or a solver that does not
			# supply decomposition information, simply has no labels to show.
			return

		var_blocks = {}
		rel_blocks = {}
		var_order = {}
		rel_order = {}
		block_sizes = {}
		try:
			fixed_vars = set(
				self.solver_instance_key(var) for var in self.sim.getFixedVariables()
			)
			for block in range(nblocks):
				block_vars = list(im.getBlockVars(block))
				block_rels = list(im.getBlockRels(block))
				block_sizes[block] = (len(block_vars), len(block_rels))
				for offset, var in enumerate(block_vars):
					key = self.solver_instance_key(var)
					var_blocks[key] = block
					var_order[key] = offset
				for offset, rel in enumerate(block_rels):
					key = self.solver_instance_key(rel)
					rel_blocks[key] = block
					rel_order[key] = offset
		except (AttributeError, RuntimeError, IndexError):
			# Treat incomplete decomposition information as unavailable instead
			# of leaving a partially-labelled tree.
			return

		self.solver_var_blocks = var_blocks
		self.solver_rel_blocks = rel_blocks
		self.solver_var_order = var_order
		self.solver_rel_order = rel_order
		self.solver_block_sizes = block_sizes
		self.solver_fixed_vars = fixed_vars

		self.apply_cached_block_data()

	def solver_instance_key(self, instance):
		"""Identify the underlying instance, independent of any alias path."""
		try:
			instance = instance.getInstance()
		except (AttributeError, RuntimeError):
			pass
		try:
			return ("instance", int(instance.getInstanceId()))
		except (AttributeError, RuntimeError):
			# Compatibility fallback for an older ASCXX module. It does not give
			# alias guarantees, but retains the former behaviour.
			return ("name", str(self.sim.getInstanceName(instance)))

	def clear_block_data(self, piter):
		self.modelstore.set_value(piter, BLOCK_INDEX, "")
		self.modelstore.set_value(piter, BLOCK_KIND_INDEX, BLOCK_UNKNOWN)
		self.modelstore.set_value(piter, BLOCK_LOW_INDEX, 0)
		self.modelstore.set_value(piter, BLOCK_HIGH_INDEX, 0)
		self.modelstore.set_value(piter, BLOCK_POSITION_LOW_INDEX, 0)
		self.modelstore.set_value(piter, BLOCK_POSITION_HIGH_INDEX, 0)

	def get_solver_block_data(self, instance):
		"""Return the display label, block extent, and within-block positions."""
		try:
			if instance.getType().isRefinedSolverVar():
				key = self.solver_instance_key(instance)
				if key in self.solver_fixed_vars:
					return ("–", BLOCK_FIXED, 0, 0, 0, 0)
				if key in self.solver_var_blocks:
					block = self.solver_var_blocks[key]
					_var_count, rel_count = self.solver_block_sizes[block]
					position = self.solver_var_order[key]
					if not self.relations_appear_last:
						position += rel_count
					return (
						str(block), BLOCK_NUMBERED, block, block,
						position, position,
					)
			elif instance.isRelation():
				key = self.solver_instance_key(instance)
				if key in self.solver_rel_blocks:
					block = self.solver_rel_blocks[key]
					var_count, _rel_count = self.solver_block_sizes[block]
					position = self.solver_rel_order[key]
					if self.relations_appear_last:
						position += var_count
					return (
						str(block), BLOCK_NUMBERED, block, block,
						position, position,
					)
		except RuntimeError:
			pass
		return ("", BLOCK_UNKNOWN, 0, 0, 0, 0)

	def set_direct_block_data(self, piter, instance):
		label, kind, low, high, low_position, high_position = (
			self.get_solver_block_data(instance)
		)
		self.modelstore.set_value(piter, BLOCK_INDEX, label)
		self.modelstore.set_value(piter, BLOCK_KIND_INDEX, kind)
		self.modelstore.set_value(piter, BLOCK_LOW_INDEX, low)
		self.modelstore.set_value(piter, BLOCK_HIGH_INDEX, high)
		self.modelstore.set_value(piter, BLOCK_POSITION_LOW_INDEX, low_position)
		self.modelstore.set_value(piter, BLOCK_POSITION_HIGH_INDEX, high_position)

	def update_compound_block_extent(self, piter):
		"""Aggregate descendant block data and return this row's extent."""
		child = self.modelstore.iter_children(piter)
		child_data = []
		while child is not None:
			child_data.append(self.update_compound_block_extent(child))
			child = self.modelstore.iter_next(child)

		path = self.modelstore.get_path(piter).to_string()
		instance = self.otank[path][1]
		if not instance.isCompound():
			return (
				self.modelstore.get_value(piter, BLOCK_KIND_INDEX),
				self.modelstore.get_value(piter, BLOCK_LOW_INDEX),
				self.modelstore.get_value(piter, BLOCK_HIGH_INDEX),
				self.modelstore.get_value(piter, BLOCK_POSITION_LOW_INDEX),
				self.modelstore.get_value(piter, BLOCK_POSITION_HIGH_INDEX),
			)

		numbered = [data for data in child_data if data[0] == BLOCK_NUMBERED]
		if numbered:
			low = min(data[1] for data in numbered)
			high = max(data[2] for data in numbered)
			low_position = min(data[3] for data in numbered if data[1] == low)
			high_position = min(data[4] for data in numbered if data[2] == high)
			label = str(low) if low == high else "%d–%d" % (low, high)
			kind = BLOCK_NUMBERED
		elif any(data[0] == BLOCK_FIXED for data in child_data):
			label = "–"
			kind = BLOCK_FIXED
			low = high = 0
			low_position = high_position = 0
		else:
			label = ""
			kind = BLOCK_UNKNOWN
			low = high = 0
			low_position = high_position = 0

		self.modelstore.set_value(piter, BLOCK_INDEX, label)
		self.modelstore.set_value(piter, BLOCK_KIND_INDEX, kind)
		self.modelstore.set_value(piter, BLOCK_LOW_INDEX, low)
		self.modelstore.set_value(piter, BLOCK_HIGH_INDEX, high)
		self.modelstore.set_value(piter, BLOCK_POSITION_LOW_INDEX, low_position)
		self.modelstore.set_value(piter, BLOCK_POSITION_HIGH_INDEX, high_position)
		return (kind, low, high, low_position, high_position)

	def apply_cached_block_data(self):
		for _path, (_name, instance) in self.otank.items():
			piter = self.modelstore.get_iter(_path)
			self.set_direct_block_data(piter, instance)
		root = self.modelstore.get_iter_first()
		while root is not None:
			self.update_compound_block_extent(root)
			root = self.modelstore.iter_next(root)

	def get_solver_block_label(self, instance):
		"""Return the cached direct block label for an instance."""
		return self.get_solver_block_data(instance)[0]

	def refresh_display_units(self, instance=None, instance_type=None):
		"""
		Refresh only displayed values affected by units policy changes.
		If instance is provided: refresh that instance only.
		If instance_type is provided: refresh matching type name only.
		"""
		target_type_name = None
		if instance_type is not None:
			target_type_name = str(instance_type.getName())
		for _path in self.otank:
			_iter = self.modelstore.get_iter(_path)
			_name, _instance = self.otank[_path]
			if instance is not None and _instance != instance:
				continue
			if target_type_name is not None:
				try:
					if str(_instance.getType().getName()) != target_type_name:
						continue
				except Exception:
					continue
			_value = self.browser.get_instance_display_value(_instance)
			self.modelstore.set_value(_iter, 2, _value)

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

			_default_units = self.browser.get_instance_display_units(_instance)
			_e = RealAtomEntry(_instance, newtext, _default_units)
			try:
				_e.checkEntry()
				_e.setValue()
				_e.applyUnitsOverride(self.browser)
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
			elif _instance.isSelector():
				_val = str(newtext)
				if _val == str(_instance.getSelectorValue()):
					self.browser.reporter.reportNote("Selector '%s' was not altered" % _instance.getName())
					return True
				_instance.setSelectorValue(ascpy.SymChar(_val))
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
		_iter = self.modelstore.get_iter(originalpath)
		self.modelstore.set_value(_iter,2, self.browser.get_instance_display_value(_instance))

		if _instance.getType().isRefinedSolverVar():
			self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR) # set the row green as fixed

		self.browser.do_solve_if_auto()
		return True

	def cell_editing_started_callback(self, renderer, editable, path):
		"""Replace rounded display text with full precision for inline editing."""
		if not isinstance(editable, Gtk.Entry):
			return
		try:
			piter = self.modelview.get_model().get_iter(path)
			originalpath = self.modelview.get_model().get_value(
				piter, ORIGINAL_PATH_INDEX
			)
			_instance = self.otank[originalpath][1]
		except (KeyError, TypeError, ValueError):
			return
		editable.set_text(
			self.browser.get_instance_display_value(_instance, full_precision=True)
		)
		editable.select_region(0, -1)

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
			model_orders = self.child_model_orders(value, children)
			##### EXTERNAL RELATION WORKAROUND
			index = 0
			relation_outputs = None
			##### EXTERNAL RELATION WORKAROUND
			for child_index, child in enumerate(children):
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
					_piter = self.make_row(
						piter, child, _name, parent=value,
						model_order=model_orders[child_index]
					)
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
		if self.solver_var_blocks or self.solver_rel_blocks or self.solver_fixed_vars:
			self.apply_cached_block_data()


#   ------------------------------
#   CONTEXT MENU

	def on_treeview_event(self,widget,event):
		_path = None
		_contextmenu = False
		_button = None
		
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

		if _path:
			_model = self.modelview.get_model()
			piter = _model.get_iter(_path)
			originalpath = _model.get_value(piter, ORIGINAL_PATH_INDEX)
			_name, _instance = self.otank[originalpath]
				
		if not _contextmenu:
			#print "NOT DOING ANYTHING ABOUT %s" % Gdk.keyval_name(event.keyval)
			return False


		if _path:		
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
			if event.type == Gdk.EventType.BUTTON_PRESS:
				self.treecontext.popup_at_pointer(event)
			else:
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
			if event.type == Gdk.EventType.BUTTON_PRESS:
				self.modelmenu.popup_at_pointer(event)
			else:
				self.modelmenu.popup(None, None, None, None, _button, event.time)
			return True

		self.hidevariable.set_label("Hide " + str(_instance.getType()))
		self.hidevariable.set_sensitive(True)

		self.modelview.grab_focus()
		self.modelview.set_cursor( _path, _col, 0)
		if event.type == Gdk.EventType.BUTTON_PRESS:
			self.treecontext.popup_at_pointer(event)
		else:
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
		_instance = self.get_selected_instance()
		if _instance is None:
			self.browser.reporter.reportError("Select a real variable first.")
			return
		if not _instance.isReal():
			self.browser.reporter.reportError("Units can only be edited for real-valued variables.")
			return
		T = _instance.getType()
		try:
			_un = UnitsDialog(self.browser,T,_instance)
			_un.run()
		except:
			self.browser.reporter.reportError("Unable to display units dialog.")

	def on_query_tooltip(self,widget,x,y,keymode,tooltip):
		_model = self.modelview.get_model()
		bin_x, bin_y = self.modelview.convert_widget_to_bin_window_coords(x, y)
		_pinfo = self.modelview.get_path_at_pos(bin_x,bin_y)
		if _pinfo:
			path, col, cx, cy = _pinfo
			if path != self.last_tooltip_path:
				self.last_tooltip_path = path
				piter = _model.get_iter(path) # first element is 'path'
				if piter:
					opath = _model.get_value(piter, ORIGINAL_PATH_INDEX)
					_name, _inst = self.otank[opath]
					if _inst.isSelector():
						tooltip.set_text("Selector state: %s" % self.browser.get_instance_display_value(_inst))
					else:
						tooltip.set_text(str(_name))
					if _inst.isAtom():
						# it would be useful to have a _inst.parent() method, but we don't have it, so use the treemodel
						parent = _model.iter_parent(piter)
						if parent:
							parentpath = _model.get_value(parent, ORIGINAL_PATH_INDEX)
							_pname,_pinst = self.otank[parentpath]
							n = self.notes.getNoteForVariable(_pinst.getType(),_name)
							if n:
								tooltip.set_text(n)
								self.last_tooltip = n
								return True
				self.last_tooltip = None
			else:
				if self.last_tooltip:
					tooltip.set_text(self.last_tooltip)
					return True

		return False
