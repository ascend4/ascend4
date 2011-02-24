import pygtk
pygtk.require('2.0')
import gtk, pango
import blockinstance

BROWSER_FIXED_COLOR = "#008800"
BROWSER_FREE_COLOR = "#000088"
BROWSER_SETTING_COLOR = "#4444AA"

BROWSER_INCLUDED_COLOR = "black"
BROWSER_UNINCLUDED_COLOR = "#888888"

class TreeView:
	def __init__(self, instance):
		self.data = {}
		columns = [str,str,str,str,int,bool,gtk.gdk.Pixbuf]
		self.treestore = gtk.TreeStore(*columns)
		self.treeview = gtk.TreeView(self.treestore)
		self.otank = {}	
		titles = [" Name"," Type"," Value"];
		self.tvcolumns = [ gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		i = 0
		for tvcolumn in self.tvcolumns[:len(titles)]:
			tvcolumn.set_title(titles[i])
			self.treeview.append_column(tvcolumn)			
			renderer = gtk.CellRendererText()
			tvcolumn.pack_start(renderer, True)
			tvcolumn.add_attribute(renderer, 'text', i)
			tvcolumn.add_attribute(renderer, 'foreground', 3)
			tvcolumn.add_attribute(renderer, 'weight', 4)
			i += 1
		self.make(instance.getName(), instance)
				
				
	def get_tree_row_data(self,instance):
		_value = str(instance.getValue())
		_type = str(instance.getType())
		_name = str(instance.getName())
		_fgcolor = BROWSER_INCLUDED_COLOR
		_fontweight = pango.WEIGHT_NORMAL
		_editable = False
		_statusicon = None
		if instance.getType().isRefinedSolverVar():
			_editable = False
			_fontweight = pango.WEIGHT_BOLD
			if instance.isFixed():
				_fgcolor = BROWSER_FIXED_COLOR
			else:
				_fgcolor = BROWSER_FREE_COLOR
				_fontweight = pango.WEIGHT_BOLD
				_status = instance.getStatus();
				#_statusicon = self.browser.statusicons[_status]
		elif instance.isRelation():
			_status = instance.getStatus();
			#_statusicon = self.browser.statusicons[_status]
			if not instance.isIncluded():
				_fgcolor = BROWSER_UNINCLUDED_COLOR
		elif instance.isBool() or instance.isReal() or instance.isInt():
			# TODO can't edit constants that have already been refined
			_editable = False
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = pango.WEIGHT_BOLD
		elif instance.isSymbol() and not instance.isConst():
			_editable = False
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = pango.WEIGHT_BOLD
		self.data[str(instance.getName())] = [_name, _type, _value, _fgcolor, _fontweight, _editable, _statusicon]
		return [_name, _type, _value, _fgcolor, _fontweight, _editable, _statusicon]
		
	def make_row( self, piter, name, value ):
		assert(value)
		_piter = self.treestore.append( piter, self.get_tree_row_data(value) )
		return _piter
			
	def make_row_from_presaved(self, piter, name, value):
		assert(value)
		_piter = self.treestore.append( piter,self.data[str(value.getName())])
		return _piter
	
	def make_children(self, value, piter ):
		assert(value)
		if value.isCompound():
			children=value.getChildren();
			for child in children:
				try:
					_name = child.getName();
					_piter = self.make_row(piter,_name,child)
					_path = self.treestore.get_path(_piter)
					self.otank[_path]=(_name,child)
				except Exception,e:
					pass
				
	def make_children_from_presaved(self, value, piter ):
		assert(value)
		if value.isCompound():
			children=value.getChildren();
			for child in children:
				try:
					_name = child.getName();
					_piter = self.make_row_from_presaved(piter,_name,child)
					_path = self.treestore.get_path(_piter)
					self.otank[_path]=(_name,child)
				except Exception,e:
					pass
				
	def make(self, name=None, value=None, path=None, depth=2):
		if path is None:
		# make root node
			piter = self.make_row( None, name, value )
			path = self.treestore.get_path( piter )
			self.otank[ path ] = (name, value)
		else:
			name, value = self.otank[ path ]
		
		assert(value)
		
		piter = self.treestore.get_iter( path )
		if not self.treestore.iter_has_child( piter ):
			self.make_children(value,piter)
		
		if depth:
			for i in range( self.treestore.iter_n_children( piter ) ):
				self.make( path = path+(i,), depth = depth - 1 )
		else:
			self.treeview.expand_row("0",False)			
	
	#def make_from_presaved(self, name=None, value=None, path=None, depth=1):
		#if path is None:
		## make root node
			#piter = self.make_row_from_presaved( None, name, value )
			#path = self.treestore.get_path( piter )
			#self.otank[ path ] = (name, value)
		#else:
			#name, value = self.otank[ path ]
		
		#assert(value)
		
		#piter = self.treestore.get_iter( path )
		#if not self.treestore.iter_has_child( piter ):
			#self.make_children_from_presaved(value,piter)
		
		#if depth:
			#for i in range( self.treestore.iter_n_children( piter ) ):
				#self.make_from_presaved( path = path+(i,), depth = depth - 1 )
		#else:
			#self.treeview.expand_row("0",False)	
