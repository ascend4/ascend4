#!/usr/bin/env python

import gtk, gobject, os, pango, re
import blockinstance
import preferences
import gtksourceview2 as gtksourceview
import modeltree
from unitsdialog import UnitsDialog
from varentry_canvas import *
SAVED_TAB = 0

#Not a good place to do this, but makes implementation very easy.
#TODO: Set up a central mechanism to handle icons, images, colors.
_iconfixed = gtk.Image()
_iconfree = gtk.Image()
_iconfixed.set_from_file(os.path.join('..','glade','locked.png'))
_iconfree.set_from_file(os.path.join('..','glade','unlocked.png'))
_iconfixed = _iconfixed.get_pixbuf()
_iconfree = _iconfree.get_pixbuf()
_colorfixed = '#33CC00'
_colorfree = '#000000'
_weightfixed = 700
_weightfree = 400


class BlockProperties(object):
	'''
	Pop up window for viewing and editing ASCEND block properties.
	Hint: The Parameter data is stored in the ListStore i.e. M of MVC of TreeView
	'''
	def __init__(self, parent, item, tab= None):
		#Get the XML
		glade_file_path = os.path.join('..','glade','bp.glade')
		xml = gtk.glade.XML(glade_file_path,root = None,domain="",typedict={})
		self.dialog = xml.get_widget('dialog')
		self.parent = parent
		self.units = self.parent.ascwrap.library.getUnits()

		self.block = item.blockinstance

		self.dialog.set_title('Properties of '+ str(self.block.name))

		##General Tab##
		#Set the 'Name:'
		self.block_name = xml.get_widget('block_name')
		self.block_name.set_text(self.block.name)
		self.block_name.set_editable(True)

		#Set the 'Type:'
		self.type_name = xml.get_widget('type_name')
		text = self.block.blocktype.type
		self.type_name.set_text(str(text))
		self.type_name.set_editable(False)
		#Set the 'Ports:'
		self.type_name = xml.get_widget('port_name')
		ports = self.block.ports
		sorted_ports = [[],[],[]]

		#Sort the ports
		for i,j in ports.iteritems():
			if j.type == blockinstance.PORT_IN:
				sorted_ports[0].append(j.name)
			elif j.type == blockinstance.PORT_OUT:
				sorted_ports[1].append(j.name)
			elif j.type == blockinstance.PORT_INOUT:
				sorted_ports[2].append(j.name)

		self.general_entry = [xml.get_widget('port_name_input'), xml.get_widget('port_name_output'), xml.get_widget('port_name_inputoutput')]

		#Display the ports, set them not editable
		for i in range(len(self.general_entry)):
			self.general_entry[i].set_editable(False)
			for port in sorted_ports[i]:
				self.general_entry[i].set_text(str(sorted_ports[i]))

		#Stream
		#self.stream = xml.get_widget('comboboxentry1')
		#self.stream_store = gtk.ListStore(gobject.TYPE_STRING)

		'''
		for stream in self.parent.ascwrap.streams:
			try:
				self.block.blocktype.type.findMember(stream[0])
				for sm_type in stream[1]:
					self.stream_store.append([str(sm_type)])
			except RuntimeError:
				print "DEBUG: Stream not found"

		self.stream.set_model(self.stream_store)
		self.stream.set_entry_text_column(0)
		iter = self.stream_store.get_iter_first()


		#FIXME Does not properly handle all cases, i.e. if there are multiple
		#stream in one block
		for stream in self.parent.ascwrap.streams:
			if self.block.stream in stream[1]:
				while True:
					if self.block.stream == self.stream_store.get_value(iter,0):
						break;
					else:
						iter = self.stream_store.iter_next(iter)
						self.stream.set_active_iter(iter)

		self.stream.connect('changed',self.stream_changed)
		'''
		##End of General Tab##

		##Parameters Tab##
		#Hint: This uses the gtk.TreeView and gtk.ListStore

		self.param_list_store = paramListStore(self.block.params)
		self.param_tree_model = self.param_list_store.get_model()
		self.param_tree_view = displayModel(self.parent)
		self.view_param = self.param_tree_view.draw_view(self.param_tree_model,xml,self.units)
		self.view_param.columns_autosize()
		self.param_tree_view.view.connect("button-press-event", self.on_treeview_event )
		self.param_tree_view.view.connect("key-press-event",self.on_treeview_event )
		##End of Parameters Tab##

		###Custom Method Tab##
		#custom_alignment = xml.get_widget('custom_alignment')
		#self.sourceviewView = gtksourceview.View()
		#self.sourceviewView.set_show_line_numbers(True)
		#self.sourceviewLangman = gtksourceview.language_manager_get_default()
		#op = self.sourceviewLangman.get_search_path()

		#if os.path.join('..','..','tools','gtksourceview-2.0') not in op:
			#op.append(os.path.join('..','..','tools','gtksourceview-2.0'))
			#self.sourceviewLangman.set_search_path(op)

		#self.sourceviewLang = self.sourceviewLangman.get_language('ascend')
		#self.sourceviewBuff = gtksourceview.Buffer()
		#self.sourceviewBuff.set_language(self.sourceviewLang)
		#self.sourceviewBuff.set_highlight_syntax(True)
		#self.sourceviewBuff.set_text(self.block.usercode)
		#self.sourceviewView.set_buffer(self.sourceviewBuff)
		#custom_alignment.add(self.sourceviewView)
		#self.sourceviewView.show()
		###End of Custom Method Tab##

		##Instance Tab##
		try:
			self.instance_box = xml.get_widget('instance')
			self.instance_model = modeltree.TreeView(self.block.instance)
			self.instance_box.add(self.instance_model.treeview)
			self.instance_model.treeview.show()
		except Exception as e:
			self.instance_box = xml.get_widget('instance')
			self.instance_label = gtk.Label()
			self.instance_box.add_with_viewport(self.instance_label)
			self.instance_label.set_text('Instance not Built, Solve the Canvas Model first!')
			self.instance_label.show()
		##End of Instance Tab##

		self.notebook = xml.get_widget('notebook1')

		global SAVED_TAB
		if tab is not None:
			self.notebook.set_current_page(tab)
		else:
			self.notebook.set_current_page(SAVED_TAB)

		##Attach callback to OK##
		OK_button = xml.get_widget('ok')
		OK_button.connect('clicked',self.save_changes,self.parent,self.block)
		OK_button.grab_default()

		##Context Menu##
		self.treecontext = gtk.Menu();
		self.fixmenuitem = gtk.ImageMenuItem("_Fix/ _Free",True);
		self.unitsmenuitem = gtk.ImageMenuItem("Select _Units",True);
		self.fixmenuitem.show(); self.fixmenuitem.set_sensitive(False)
		self.unitsmenuitem.show(); self.unitsmenuitem.set_sensitive(False)

		self.treecontext.append(self.fixmenuitem)
		_sep = gtk.SeparatorMenuItem(); _sep.show()
		self.treecontext.append(_sep);
		self.treecontext.append(self.unitsmenuitem)
		self.fixmenuitem.connect("activate",self.fixfree_toggle)
		self.unitsmenuitem.connect("activate",self.units_activate)

		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")
		##End of Context Menu##

	def save_changes(self,button,parent,block):

		parent.view.canvas.canvasmodelstate = 'Modified'
		parent.status.push(0,"Modified Block Properties")
		parent.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))

	def on_treeview_event(self,widget,event):

		self.unitsmenuitem.set_sensitive(False)
		self.fixmenuitem.set_sensitive(False)
		_contextmenu = False

		if event.type==gtk.gdk.BUTTON_PRESS:
			_x = int(event.x)
			_y = int(event.y)
			_button = event.button
			_pthinfo = self.param_tree_view.view.get_path_at_pos(_x, _y)
			if _pthinfo is not None:
				_path, _col, _cellx, _celly = _pthinfo
				if event.button == 3:
					_contextmenu = True
					self.param_tree_view.view.get_selection().select_path(_path)
					self.unitsmenuitem.set_sensitive(True)
					self.fixmenuitem.set_sensitive(True)

		if _contextmenu:
			self.treecontext.popup( None, None, None, _button, event.time)
			return 1

	def units_activate(self,widget):
		model,iter = self.param_tree_view.view.get_selection().get_selected()
		param = model.get_value(iter,3)
		_udia = UnitsDialog(self.parent,self.param_tree_view,param.type)
		_udia.run()

	def fixfree_toggle(self,widget):
		model,iter = self.param_tree_view.view.get_selection().get_selected()
		path = self.param_tree_view.model.get_path(iter)
		self.param_tree_view.toggle_callback(path=path,model=model)
	'''
	def stream_changed(self,widget):
		stream = widget.get_active_text()
		self.parent.view.canvas.set_stream(stream)
	'''
	def run(self):
		global SAVED_TAB
		self.dialog.run()
		SAVED_TAB = self.notebook.get_current_page()
		self.dialog.hide()

class displayModel(object):
	'''
	Draws the gtk.TreeView, used for the setting of parameters
	'''
	def __init__(self,parent):
		self.parent = parent

	def draw_view(self,model,xml,units):
		self.view = xml.get_widget('param_tree')
		self.view.set_model(model)
		self.view.set_tooltip_column(6)
		self.model = model
		#Set the row renderers
		self.name_render = gtk.CellRendererText()
		self.name_render.set_property('foreground-set',True)
		self.name_render.set_property('weight-set',True)

		#self.param_render = gtk.CellRendererText()
		#self.param_render.set_property('editable',True)
		#self.param_render.connect('edited', self.set_param_callback,model)
		#self.param_render.set_property('foreground-set',True)
		#self.param_render.set_property('weight-set',True)

		self.icon_render = gtk.CellRendererPixbuf()

		self.toggle_render = gtk.CellRendererToggle()
		self.toggle_render.connect('toggled',self.toggle_callback,model)
		self.toggle_render.set_property('indicator-size',0)

		self.units_render = gtk.CellRendererText()
		self.units_render.set_property('editable',True)
		#self.units_render.set_property('model',self.umodel)
		#self.units_render.set_property('text-column',0)
		self.units_render.set_property('foreground-set',True)
		self.units_render.set_property('weight-set',True)
		#self.units_render.set_property('has-entry',False)
		self.units=units
		self._units = []
		#self.units_render.connect('editing-started',self.populate_units,model,self.units)
		self.units_render.connect('edited',self.set_units_callback,model)

		self.description_render = gtk.CellRendererText()
		self.description_render.set_property('foreground-set',True)
		self.description_render.set_property('weight-set',True)

		#Set the column views
		self.name_column = gtk.TreeViewColumn('Parameter',self.name_render,text=0,foreground =4, weight=5)
		#self.name_column.pack_start(self.toggle_render_name,False)
		#self.name_column.set_attributes(self.param_render,text=1)

		self.description_column = gtk.TreeViewColumn('Description',self.description_render,text=6,foreground =4, weight=5)

		#self.param_column = gtk.TreeViewColumn('Value')
		#self.param_column.set_expand(False)
		#self.param_column.pack_start(self.param_render,False)
		#self.param_column.set_attributes(self.param_render,text=1, foreground =5, weight=6)
		#self.param_column.set_expand(False)

		self.toggle_column = gtk.TreeViewColumn('Status')
		self.toggle_column.pack_start(self.icon_render,False)
		self.toggle_column.pack_start(self.toggle_render,False)
		self.toggle_column.set_attributes(self.icon_render,pixbuf=2)

		self.units_column = gtk.TreeViewColumn('Value',self.units_render,text=1, foreground =4, weight=5)

		self.view.append_column(self.name_column)
		self.view.append_column(self.toggle_column)
		#self.view.append_column(self.param_column)
		self.view.append_column(self.units_column)
		self.view.append_column(self.description_column)

		return self.view

	def toggle_callback(self,cellrendertoggle=None,path=None,model=None,fix=False):
		iter = model.get_iter(path)
		param = model.get_value(iter,3)

		if fix == True:
			param.fix = True
			if not param.value:
				param.value = 0
				model.set_value(iter,1,param.getValue())
			model.set_value(iter,2,_iconfixed)
			model.set_value(iter,4,_colorfixed)
			model.set_value(iter,5,_weightfixed)
			return

		if param.fix == 0:
			param.fix = True
			if not param.value:
				param.value = None
				model.set_value(iter,1,param.getValue())
			model.set_value(iter,2,_iconfixed)
			model.set_value(iter,4,_colorfixed)
			model.set_value(iter,5,_weightfixed)
		else:
			param.fix = False
			model.set_value(iter,2,_iconfree)
			model.set_value(iter,4,_colorfree)
			model.set_value(iter,5,_weightfree)

	#def set_param_callback(self,cellrendertext,path,new_text,model):
		#iter = model.get_iter(path)
		#param = model.get_value(iter,4)
		#param.value = new_text
		#model.set_value(iter,1,new_text)
		#param.fix = True
		#model.set_value(iter,3,_iconfixed)
		#if len(new_text) == 0:
			#model.set_value(iter,3,_iconfree)

	def set_units_callback(self,combo,path,new_text,model):
		iter = model.get_iter(path)
		param = model.get_value(iter,3)
		valid = False

		if new_text == '':
			param.units = str(param.type.getDimensions().getDefaultUnits().getName())
			param.value = None
			model.set_value(iter,1,new_text)
			self.toggle_callback(path=path,model=model,fix=False)
			return

		_entry = RealAtomEntry(param,new_text)

		try:
			_entry.checkEntry()
			_entry.setValue()
			_entry.exportPreferredUnits(self.parent.prefs)
			if param.type.getPreferredUnits():
				new_text = _entry.getValue() +' ' + _entry.getUnits()
				param.units = str(_entry.getUnits())
			else:
				new_text = str(_entry.getValue()) +' ' + str(param.type.getDimensions().getDefaultUnits().getName())
				param.units = str(param.type.getDimensions().getDefaultUnits().getName())

			self.toggle_callback(path=path,model=model,fix=True)
			combo.set_property('text',new_text)
			model.set_value(iter,1,new_text)
			#self.update_all_units(_entry._conv,_entry.getUnits(),param)

		except InputError,e:
			self.parent.reporter.reportError(str(e))
			return

	#def populate_units(self,combo,editable,path,model,units):
		#iter = model.get_iter(path)
		#param = model.get_value(iter,3)
		#dim=param.type.getDimensions()
		#self._units = []
		##print 'Populating for Dimension... ' + str(dim)
		#for u in units:
			#if u.getDimensions() == dim:
				#self._units.append(str(u.getName()))

	#def update_all_units(self,conv,units,param):
		#def update(model,path,iter):
			#_param = model.get_value(iter,3)
			#if str(_param.type) == str(param.type) and not _param==param:
				#model.set_value(iter,1,param.setValue(conv,units))
		#self.model.foreach(update)


class paramListStore(object):
	'''
	Stores the Parameters in a gtk.ListStore for gtk.TreeView
	First Column:	Name
	Second Column:   Value
	Fourth Column:   Lock/Unlock Icon Status
	Fifth Column:	Text Foreground
	Sixth Column:	Text Weight
	Seventh Column:  Tootip Description
	'''
	def __init__(self,params):
		self.list_store = gtk.ListStore(gobject.TYPE_STRING,gobject.TYPE_STRING,gtk.gdk.Pixbuf,gobject.TYPE_PYOBJECT,gobject.TYPE_STRING, gobject.TYPE_INT,gobject.TYPE_STRING)
		self.params = params
		for name in self.params.keys():
			pi=self.params[name]
			if self.params[name].fix == True:
				self.list_store.append([pi.name,pi.getValue(), _iconfixed, pi, _colorfixed, _weightfixed,pi.get_description()])
			else:
				self.list_store.append([pi.name,pi.getValue(), _iconfree, pi, _colorfree, _weightfree,pi.get_description()])
	def get_model(self):
		if self.list_store:
			return self.list_store
		return None

# vim: set ts=4 noet:
