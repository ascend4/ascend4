import gi
gi.require_version('GtkSource','3.0')
from gi.repository import Gtk, GtkSource, GObject
from gi.repository import Pango
import ascpy
#import gtksourceview2 as gtksourceview
import os

class ModuleView:
	def __init__(self,browser,builder, library):
		"""Set up the 'modules' tab, set up column types."""

		self.browser = browser
		self.builder = builder
		self.library = library
		self.moduleview = builder.get_object('moduleview')

		modulestorecoltypes = [str, str, int] # bool=can-be-instantiated
		self.modulestore = Gtk.TreeStore(*modulestorecoltypes)
		moduleviewtitles = ["Module name", "Filename"]
		self.moduleview.set_model(self.modulestore)
		self.modcols = [ Gtk.TreeViewColumn() for _type in modulestorecoltypes]
		i = 0
		for modcol in self.modcols[:len(moduleviewtitles)]:
			modcol.set_title(moduleviewtitles[i])
			self.moduleview.append_column(modcol)
			_renderer = Gtk.CellRendererText()
			modcol.pack_start(_renderer, True)
			modcol.add_attribute(_renderer, 'text', i)
			modcol.add_attribute(_renderer,'weight',2)
			i = i + 1
		self.moduleview.connect("row-activated", self.module_activated )		
		self.moduleview.connect("button-press-event", self.on_treeview_event)
		self.modtank = {}
		
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["modulemenu"])
		self.modulemenu = self.browser.builder.get_object("modulemenu")
		self.viewmenuitem = self.browser.builder.get_object("view")
		self.viewmenuitem.connect("activate",self.view_activate)
		self.modelname = None
		self.modulename = None
		self.moduleview.get_selection().set_mode(Gtk.SelectionMode.SINGLE)

	def refresh(self, library):
		"""Repopulate the 'modules' tab with the current (new) contents of the 'library'."""

		self.modtank = {}
		self.modulestore.clear()
		self.library = library
		modules = library.getModules()
		try:
			_lll=len(modules)
		except:
			_msg = "UNABLE TO ACCESS MODULES LIST. This is bad.\n"+\
			"Check your SWIG configuration (check for warnings during build)."
			
			self.browser.reporter.reportError(_msg)
			raise RuntimeError(_msg)

		firstpath = None
		for m in reversed(modules):
			_n = str( m.getName() )
			_f = str( m.getFilename() )
			#print "ADDING ROW name %s, file = %s" % (_n, _f)
			_r = self.modulestore.append(None,  [ _n, _f, Pango.Weight.NORMAL ])

			if firstpath is None:
					firstpath = self.modulestore.get_path(_r)

			for t in library.getModuleTypes(m):
				_n = str(t.getName())
				if t.isModel() and not t.hasParameters():
					_w = Pango.Weight.BOLD
				else:
					_w = Pango.Weight.NORMAL
				
				#print "ADDING TYPE %s" % _n
				_piter = self.modulestore.append(_r , [ _n, "", _w ])
				_path = self.modulestore.get_path(_piter)
				self.modtank[_path.to_string()]=t
	
		# open up the top-level module (ie the one we just openened)
		if firstpath is not None:
			#print "EXPANDING PATH",firstpath
			self.moduleview.expand_row(firstpath,False)
		
		#print "DONE ADDING MODULES"

	def module_activated(self, treeview, path, column, *args):
		"""You can't currently double-click a module to open it, but that is
		something we might allow in the future."""

		modules = self.library.getModules()
		if len(path.to_string())==1:
			if self.moduleview.row_expanded(path):
				self.moduleview.collapse_row(path)
			else:
				self.moduleview.expand_row(path,False)
			#self.browser.reporter.reportNote("Launching of external editor not yet implemented")
		elif len(path.to_string())>=3:
			if path.to_string() in self.modtank:
				_type = self.modtank[path.to_string()];
				if not _type.isModel():
					self.browser.reporter.reportError("Can't create simulation for type '%s': not a MODEL type" % str(_type.getName()))
					return
				if _type.hasParameters():
					self.browser.reporter.reportError("Can't create simulation for MODEL '%s': requires parameters" % str(_type.getName()))
					return				
				self.browser.reporter.reportNote("Creating simulation for type '%s'" % str(_type.getName()) )
				self.browser.do_sim(_type)
			else:
				self.browser.reporter.reportError("Didn't find type corresponding to row")
	
	def on_treeview_event(self,widget,event):			
		_x = int(event.x)
		_y = int(event.y)
		_button = event.button
		_pthinfo = self.moduleview.get_path_at_pos(_x, _y)
		if _pthinfo is not None:
			_path, _col, _cellx, _celly = _pthinfo
		else:	
		 	return
		#self.moduleview.grab_focus()
		self.moduleview.set_cursor(_path,_col,0)
		if event.button == 3:	
			x = widget.get_selection()
			y = x.get_selected()
			if len(y[0].get_path(y[1]).to_string())==1:
				self.modulename=y[0].get_value(y[1],0)
				self.modelname=None
			elif len(y[0].get_path(y[1]).to_string())==3:	
				self.modelname = y[0].get_value(y[1],0)
				self.modulename = None
			self.viewmenuitem.set_sensitive(True)
			self.modulemenu.popup(None,None,None,None,3,event.time)
		
	def view_activate(self,widget,*args):
		filename=''
		if self.modulename:
			x = ascpy.Library()
			# TODO is this the fastest way??
			for module in x.getModules():
				if module.getName()==self.modulename:
					filename=module.getFilename()
					break
			# FIXME what if module not found??
			ViewModel(filename=filename,title="Module '%s'" % (self.modulename))
		elif self.modelname:
			x = ascpy.Library() 
			for module in x.getModules():
				for model in  x.getModuleTypes(module):
					if str(model)==self.modelname:
						filename=module.getFilename()
			if not filename:
				return
			displaytext=[]
			typelist = ['MODEL','DEFINITION','ATOM']  
			proceed = False
			flagvariable = False  
			module = open(filename,"r")
			if module:
				lines = module.readlines()
				for line in lines:
					words = line.split()
					for i in range(len(words)):
						if words[i] in typelist:
							if i!= len(words)-1:
								if words[i+1].split(';')[0]==self.modelname or words[i+1].split('(')[0]==self.modelname:
									proceed = True
						elif words[i]=='END':
							if words[i+1].split(';')[0]==self.modelname or words[i+1].split('(')[0]==self.modelname:
								flagvariable = True
								if proceed == True:
									displaytext.append(line)
									proceed = False
								break
						if proceed == True:
							displaytext.append(line)
							break
					if flagvariable==True:
						break
				ViewModel(text=''.join(displaytext),title="Model '%s'" % (self.modelname))

	def clear(self):
		self.modulestore.clear()

class ViewModel:
	"""
	A window to display the content of an ASCEND model in syntax-highlighted text
	NOTE: syntax highlighting as implemented here requires the gtksourceview-3.0
	syntax file to be installed in /usr/share/gtksourceview-3.0/language-specs,
	which requires install-time configuration.
	"""
	#TODO Enable Model Editing (REALLY? That would be complicated -- JP)

	def __init__(self, filename=None, text=None, title="View"):
		# Defining the main window
		window = Gtk.Window(Gtk.WindowType.TOPLEVEL)
		window.set_resizable(True)  
		window.connect("destroy", self.close_window, window)
		window.set_title(title)
		window.set_border_width(0)
		window.set_position(Gtk.WindowPosition.CENTER)
		window.set_size_request(600,400)

		#Creating a vertical box
		box = Gtk.VBox(False, 10)
		box.set_border_width(10)
		window.add(box)
		box.show()
		
		#Get the ASCEND language
		GObject.type_register(GtkSource.View)
		mgr = GtkSource.LanguageManager.get_default()
		op = mgr.get_search_path()
		if os.path.join('..','tools','gtksourceview-3.0') not in op:
			op.append(os.path.join('..','tools','gtksourceview-3.0'))
			mgr.set_search_path(op)
		lang = mgr.get_language('ascend')

		# TODO add status bar where this message can be reported?
		if lang is None:
			print("UNABLE TO LOCATE ASCEND LANGUAGE DESCRIPTION for gtksourceview")

		#Creating a ScrolledWindow for the textview widget
		scroll = Gtk.ScrolledWindow()
		scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
		view = GtkSource.View()
		view.set_editable(False)
		buff = GtkSource.Buffer()
		buff.set_language(lang)
		buff.set_highlight_syntax(True)
		view.set_buffer(buff)
		scroll.add(view)

		scroll.show()
		view.show()
		box.pack_start(scroll, True, True, 0)

		if filename is not None:
			#Get the content of the file
			model = open(filename, "r")
			if model:
				string = model.read()
				model.close()
				buff.set_text(string)
			else:
				self.reporter.reportError( "Error opening the file" )
		elif text is not None:
			buff.set_text(text)
		else:
			buff.set_text("Nothing was selected")

		window.show()

	def close_window(self, widget, *data):
		data[0].hide()
		
# vim: ts=4:sw=4:noet
