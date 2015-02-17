import gtk
import pango
import ascpy
import gtksourceview2 as gtksourceview
import os

class ModuleView:
	def __init__(self,browser,builder, library):
		"""Set up the 'modules' tab, set up column types."""

		self.browser = browser
		self.builder = builder
		self.library = library
		self.moduleview = builder.get_object('moduleview')

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
		self.moduleview.connect("button-press-event", self.on_treeview_event)
		self.modtank = {}
		
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["modulemenu"])
		self.modulemenu = self.browser.builder.get_object("modulemenu")
		self.viewmenuitem = self.browser.builder.get_object("view")
		self.viewmenuitem.connect("activate",self.view_activate)
		self.modelname = None
		self.modulename = None
		self.moduleview.get_selection().set_mode(gtk.SELECTION_SINGLE)

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
			_r = self.modulestore.append(None,  [ _n, _f, pango.WEIGHT_NORMAL ])

			if firstpath is None:
					firstpath = self.modulestore.get_path(_r)

			for t in library.getModuleTypes(m):
				_n = t.getName()
				if t.isModel() and not t.hasParameters():
					_w = pango.WEIGHT_BOLD
				else:
					_w = pango.WEIGHT_NORMAL
				
				#print "ADDING TYPE %s" % _n
				_piter = self.modulestore.append(_r , [ _n, "", _w ])
				_path = self.modulestore.get_path(_piter)
				self.modtank[_path]=t
	
		# open up the top-level module (ie the one we just openened)
		if firstpath is not None:
			#print "EXPANDING PATH",firstpath
			self.moduleview.expand_row(firstpath,False)
		
		#print "DONE ADDING MODULES"

	def module_activated(self, treeview, path, column, *args):
		"""You can't currently double-click a module to open it, but that is
		something we might allow in the future."""

		modules = self.library.getModules()
		if len(path)==1:
			if self.moduleview.row_expanded(path):
				self.moduleview.collapse_row(path)
			else:
				self.moduleview.expand_row(path,False)
			#self.browser.reporter.reportNote("Launching of external editor not yet implemented")
		elif len(path)==2:
			if self.modtank.has_key(path):
				_type = self.modtank[path];
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
		_path, _col, _cellx, _celly = _pthinfo
		#self.moduleview.grab_focus()
		self.moduleview.set_cursor(_path,_col,0)
		if event.button == 3:	
			x = widget.get_selection()
			y = x.get_selected()
			if len(y[0].get_path(y[1]))==1:
				self.modulename=y[0].get_value(y[1],0)
				self.modelname=None
			elif len(y[0].get_path(y[1]))==2:	
				self.modelname = y[0].get_value(y[1],0)
				self.modulename = None
			self.viewmenuitem.set_sensitive(True)
			self.modulemenu.popup(None,None,None,3,event.time)
		
	def view_activate(self,widget,*args):
		filename=''
		if self.modulename:
			x = ascpy.Library()
			for module in x.getModules():
				if module.getName()==self.modulename:
					filename=module.getFilename()
					break
			ViewModel(filename,None)
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
				text = ''.join(displaytext)	
				ViewModel(None,text)

class ViewModel:

	# A window to display the content of an ASCEND model in syntax highlighted text
	#TODO Enable Model Editing 

	def __init__(self,_filename,_text):
		# Defining the main window
		window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		window.set_resizable(True)  
		window.connect("destroy", self.close_window, window)
		window.set_title("View Window")
		window.set_border_width(0)
		window.set_position(gtk.WIN_POS_CENTER)
		window.set_size_request(600,400)

		#Creating a vertical box
		box = gtk.VBox(False, 10)
		box.set_border_width(10)
		window.add(box)
		box.show()
		
		#Get the ASCEND language
		sourceviewLangman = gtksourceview.language_manager_get_default()
		op = sourceviewLangman.get_search_path()
		if os.path.join('..','tools','gtksourceview-2.0') not in op:
			op.append(os.path.join('..','tools','gtksourceview-2.0'))
			sourceviewLangman.set_search_path(op)
		sourceviewLang = sourceviewLangman.get_language('ascend')

		#Creating a ScrolledWindow for the textview widget
		scrollwindow = gtk.ScrolledWindow()
		scrollwindow.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		sourceviewView = gtksourceview.View()
		sourceviewView.set_editable(False)
		sourceviewBuff = gtksourceview.Buffer()
		sourceviewBuff.set_language(sourceviewLang)
		sourceviewBuff.set_highlight_syntax(True)
		sourceviewView.set_buffer(sourceviewBuff)
		scrollwindow.add(sourceviewView)

		scrollwindow.show()
		sourceviewView.show()
		box.pack_start(scrollwindow)

		if _filename:
			#Get the content of the file
			model = open(_filename, "r")
			if model:
				string = model.read()
				model.close()
				sourceviewBuff.set_text(string)
			else:
				self.reporter.reportError( "Error opening the file" )
		elif _text:
			sourceviewBuff.set_text(_text)

		window.show()

	def close_window(self, widget, *data):
		data[0].hide()

