import gtk
import gtk.glade
import pango
import ascpy

class ModuleView:
	def __init__(self,browser,glade, library):
		"""Set up the 'modules' tab, set up column types."""

		self.browser = browser
		self.library = library
		self.moduleview = glade.get_widget('moduleview')

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
		self.modtank = {}

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
		print "PATH",path
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

