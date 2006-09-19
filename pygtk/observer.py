import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango

OBSERVER_EDIT_COLOR = "#008800"
OBSERVER_NOEDIT_COLOR = "#000088"
OBSERVER_NORMAL_COLOR = "black"

OBSERVER_INITIAL_COLS = 3 # how many cells are at the start of the table?
OBSERVER_ICON, OBSERVER_WEIGHT, OBSERVER_EDIT = range(0,OBSERVER_INITIAL_COLS) # column indices for the start of the TreeStore
OBSERVER_NULL = 0 # value that gets added to empty cells in a new column

# This is messy code since it doesn't observe the convention of keeping your model
# separate from your view. It's all mixed up together. Yuck. Part of the
# difficulty with that was the fact that TreeStores don't support the adding of
# columns.

# Update: there is a technique for doing this, in fact:
# http://www.daa.com.au/pipermail/pygtk/2006-February/011777.html

OBSERVER_NUM=0

class ObserverColumn:
	"""
		A class to identify the instance that relates to a specify column
		and the units of measurement and column title, etc.
	"""
	def __init__(self,instance,index,name=None,units=None,browser=None):
		self.instance = instance
		self.name = name
		self.index = index

		if name==None:
			if browser == None:
				name = "UNNAMED"
			else:
				name = browser.sim.getInstanceName(instance)

		if units == None:
			units = instance.getType().getPreferredUnits()
		if units == None:
			units = instance.getType().getDimensions().getDefaultUnits()
		
		uname = str(units.getName())
		if uname.find("/")!=-1:
			uname = "["+uname+"]"

		if uname == "":
			_title = "%s" % (name)
		else:
			_title = "%s / %s" % (name, uname) 

		self.title = _title
		self.units = units
		self.uname = uname
		self.name = name
	
	def __repr__(self):
		return "ObserverColumn(name="+self.name+")"

	def cellvalue(self, column, cell, model, iter):
		#print "RENDERING COLUMN",self.index
		_rowobject = model.get_value(iter,0)

		cell.set_property('editable',False)
		cell.set_property('weight',400)
		try:
			if _rowobject.active:
				_rawval = self.instance.getRealValue()
				if self.instance.getType().isRefinedSolverVar():
					if self.instance.isFixed():
						cell.set_property('editable',True)
						cell.set_property('weight',700)
						cell.set_property('foreground',OBSERVER_EDIT_COLOR)
					else:
						cell.set_property('foreground',OBSERVER_NOEDIT_COLOR)
			else:
				cell.set_property('foreground',OBSERVER_NORMAL_COLOR)
				_rawval = _rowobject.values[self.index]
			_dataval = _rawval / self.units.getConversion()
		except IndexError:
			_dataval = "N/A"

		cell.set_property('text', _dataval)

class ObserverRow:
	"""
		Just a container for a vector of values, but with columns that
		should correspond to those in the Observer object's vector of
		ObserverColumn objects.
	"""
	def __init__(self,values=None,active=True):
		if values==None:	
			values={}

		self.values = values
		self.active = active

	def make_static(self,table):
		self.active = False
		print "TABLE COLS:",table.cols
		print "ROW VALUES:",self.values
		r=0;
		for index,col in table.cols.iteritems():
			print "ROW",r,"; INDEX: ",index,"; COL: ",col
			try:
				self.values[index] = col.instance.getRealValue()
			except KeyError,e:
				print "Key error: e=",str(e)
				self.values[index] = None
			r=r+1
		print "Made static, values:",self.values

	def get_values(self,table):
		if not self.active:
			return self.values.values()
		else:
			_v = []
			for index,col in table.cols.iteritems():
				_v.append( col.instance.getRealValue() / col.units.getConversion() )
			return _v

class ObserverTab:

	def __init__(self,xml,browser,tab,name=None,alive=True):
		global OBSERVER_NUM
		self.colindex = 0
		if name==None:
			OBSERVER_NUM=OBSERVER_NUM+1
			name = "Observer %d" % OBSERVER_NUM
		self.name = name
		self.browser=browser
		xml.signal_autoconnect(self)
		self.view = xml.get_widget('observerview')
		self.tab = tab
		self.alive=alive
		if self.alive:
			self.browser.reporter.reportNote("New observer is 'alive'")

		self.keptimg =  gtk.Image()
		self.activeimg = gtk.Image()
		self.activeimg.set_from_file("glade/active.png")
		# create PixBuf objects from these?
		self.rows = []
		_store = gtk.TreeStore(object)
		self.cols = {}

		# create the 'active' pixbuf column
		_renderer = gtk.CellRendererPixbuf()
		_col = gtk.TreeViewColumn()
		_col.set_title("")
		_col.pack_start(_renderer,False)
		_col.set_cell_data_func(_renderer, self.activepixbufvalue)
		self.view.append_column(_col);
		
		# initially there will not be any other columns

		if self.alive:
			# for a 'live' Observer, create the 'active' bottom row
			self.browser.reporter.reportNote("Adding empty row to store")
			_row = ObserverRow()
			self.activeiter = _store.append(None, [_row] )
			self.rows.append(_row)

		self.view.set_model(_store)
		self.browser.reporter.reportNote("Created observer '%s'" % self.name)

	def activepixbufvalue(self,column,cell,model,iter):
		_rowobject = model.get_value(iter,0)
		if _rowobject.active:
			cell.set_property('pixbuf',self.activeimg.get_pixbuf())
		else:
			cell.set_property('pixbuf',self.keptimg.get_pixbuf())

	def on_add_clicked(self,*args):
		self.do_add_row()

	def on_clear_clicked(self,*args):
		_store = self.view.get_model()
		_store.clear();
		self.rows = {}
		self.activeiter = _store.append(None, [ObserverRow()] )

	def do_add_row(self):
		if self.alive:
			_row = ObserverRow()
			self.rows.append(_row)
			_store = self.view.get_model()
			_oldrow = _store.get_value(self.activeiter,0)
			_oldrow.make_static(self)
			self.activeiter = _store.append(None,[_row])
			_path = _store.get_path(self.activeiter)
			_oldpath,_oldcol = self.view.get_cursor()
			self.view.set_cursor(_path, _oldcol)
		else:
			self.browser.reporter.reportError("Can't add row: incorrect observer type")

	def on_view_cell_edited(self, renderer, path, newtext, col):
		# we can assume it's always the self.activeiter that is edited...
		if col.instance.isFixed():
			val = float(newtext) * col.units.getConversion()
			col.instance.setRealValue( val )
			self.browser.reporter.reportNote("Updated value to %f" % float(newtext))
		else:
			self.browser.reporter.reportError("Can't set a FREE variable from the Observer")
			return
		self.browser.do_solve_if_auto()

	def sync(self):
		self.view.queue_draw()
		self.browser.reporter.reportNote("SYNC performed")

	def add_instance(self,instance):
		_col = ObserverColumn(instance,self.colindex,browser=self.browser)
		self.cols[self.colindex] = _col
		self.colindex = self.colindex + 1

		# create a new column
		_renderer = gtk.CellRendererText()
		_renderer.connect('edited',self.on_view_cell_edited, _col)
		_tvcol = gtk.TreeViewColumn()
		_tvcol.set_title(_col.title)
		_tvcol.pack_start(_renderer,False)
		_tvcol.set_cell_data_func(_renderer, _col.cellvalue)
		self.view.append_column(_tvcol);
		self.browser.reporter.reportError("cols = "+str(self.cols))

	def copy_to_clipboard(self,clip):
		_s = []
		_s.append('\t'.join([_v.title for _k,_v in self.cols.iteritems()]))
		print "COPYING %d ROWS" % len(self.rows)
		for _r in self.rows:
			_s.append("\t".join([`_v` for _v in _r.get_values(self)]))

		clip.set_text('\n'.join(_s),-1) 

		self.browser.reporter.reportNote("Observer '%s' data copied to clipboard" % self.name)

#-------------------------------------------------------------------------------
# OLD STUFF

class ObserverTab1:
	"""	
		An 'Observer' tab in the Browser interface. Multiple tabs should be
		possible.
	"""
	def __init__(self,xml,name,browser,tab):
		xml.signal_autoconnect(self);

		self.view = xml.get_widget('observerview')
		self.tab = tab

		self.activeimg = None
		self.keptimg = None
		
		# no instances yet in the observer:
		self.columninstances = []

		self.columns = [gtk.gdk.Pixbuf,int,bool]

		# units for each data column
		self.units = []
		self.titles = []

		_store = gtk.TreeStore(*self.columns)
		self.rows = []

		# add an empty first row
		self.rows.append([])

		# work towards having multiple observers for multiple simulations
		self.name = nameself.view.set_model(_store)
		self.browser = browser

		# create the 'active' pixbuf columns	
		_renderer = gtk.CellRendererPixbuf()
		_col = gtk.TreeViewColumn()
		_col.set_title("")
		_col.pack_start(_renderer,False)
		_col.add_attribute(_renderer, 'pixbuf', OBSERVER_ICON)
		self.view.append_column(_col);

		# create the first row
		print "Adding row",self.rows[0],"to store"
		_store.append(None, self.make_row(True, self.rows[0]) )

		self.activerow = 0

		self.view.set_model(_store)

		self.browser.reporter.reportNote("Created observer '%s'" % self.name)

	def on_add_clicked(self,*args):
		self.do_add_row()

	def do_add_row(self):
		_rownum = len(self.rows)

		# add a copy of the last row
		self.rows.append(self.rows[_rownum-1])
		self.activerow = _rownum

		_m = self.view.get_model()
		_m.set(self.activeiter,OBSERVER_ICON,self.keptimg,OBSERVER_WEIGHT,pango.WEIGHT_NORMAL,OBSERVER_EDIT,False)
		self.activeiter = _m.append(None,self.make_row(True,self.rows[_rownum]))
		self.browser.reporter.reportNote("Kept current values");		

		# if the observer is the active tab, move the cursor to the new row.
		if self.browser.maintabs.get_current_page() == self.tab:
			self.view.set_cursor(_m.get_path(self.activeiter))

	def on_clear_clicked(self,*args):
		self.rows = []
		_r = [_i.getRealValue() for _i in self.columninstances]
		self.rows.append(_r)
		self.view.get_model().clear();
		self.view.get_model().append(None,self.make_row(True,_r))
		self.browser.reporter.reportNote("Observer '%s' cleared" % self.name)

	def on_view_cell_edited(self, renderer, path, newtext, datacolumn):
		# we can assume it's always the self.activeiter that is edited...
		if self.columninstances[datacolumn].isFixed():
			self.columninstances[datacolumn].setRealValue( float(newtext) * self.units[datacolumn].getConversion() )
		else:
			self.browser.reporter.reportError("Can't set a FIXED variable from the Observer")
			return
		self.browser.do_solve_if_auto()

	def sync(self):
		#new row data
		_r = [self.columninstances[_i].getRealValue() / self.units[_i].getConversion() for _i in range(0,len(self.columninstances)) ]

		_r1 = self.make_row(True,_r)

		# stick the row data into the TreeStore
		_m = self.view.get_model()
		_i = 0
		for _c in _r1:
			_m.set(self.activeiter, _i, _c)
			_i = _i + 1

		# keep the data in self.rows as well
		self.rows[self.activerow] = _r;

	def copy_to_clipboard(self,clip):
		_s = []
		_s.append('\t'.join(self.titles))
		for _r in self.rows:
			_s.append( '\t'.join([`_c` for _c in _r]) )

		clip.set_text('\n'.join(_s),-1) 

		self.browser.reporter.reportNote("Observer '%s' data copied to clipboard" % self.name)

	def make_row(self,isactive,row):
		# add the initial OBSERVER_INITIAL_COLS fields:
		if isactive:
			_r = [self.activeimg, pango.WEIGHT_BOLD, True]
		else:
			_r = [self.keptimg, pango.WEIGHT_NORMAL, False]
		
		for _c in row:
			_r.append(_c)

		return _r		

	def add_instance(self, inst):
		# TODO big changes here....
		if not inst.getType().isRefinedSolverVar():
			self.browser.reporter.reportError("Instance is not a refined solver variable: can't 'observe'.");
			return

		_colnum = len(self.columns)
		_colname = self.browser.sim.getInstanceName(inst)
		_rownum = len(self.rows)-1
		
		# store the instances in self.columninstances for sync purposes
		self.columninstances.append(inst)
	
		# create new TreeStore, copy of old, plus one columm
		self.columns.append(float)
		_units = inst.getType().getPreferredUnits()
		if _units == None:
			_units = inst.getType().getDimensions().getDefaultUnits()

		_uname = str(_units.getName())
		if _uname.find("/")!=-1:
			_uname = "["+_uname+"]"

		if _uname == "":
			_title = "%s" % (_colname)
		else:
			_title = "%s / %s" % (_colname, _uname) 

		self.titles.append(_title);
		self.units.append(_units) # we keep a track of the preferred units for the column at the time of the column creation

		_store = gtk.TreeStore(*(self.columns))

		_iter = None
		_i = 0
		_active = False
		for _r in self.rows:
			_r.append(OBSERVER_NULL)
			if _i == _rownum:
				_active = True
			_iter = _store.append(None, self.make_row(_active,_r) )
			_i = _i + 1
	
		self.activeiter = _iter

		# add newest data point in bottom-right
		_datacol = _colnum - OBSERVER_INITIAL_COLS
		_dataval = inst.getRealValue() / self.units[_datacol].getConversion() # convert value to units specified when col created
		self.rows[_rownum][_datacol] = _dataval
		_store.set_value(self.activeiter, _colnum, _dataval)

		# re-assign store to TreeView
		self.view.set_model(_store)

		_renderer = gtk.CellRendererText()
		_renderer.connect('edited',self.on_view_cell_edited, _datacol)
		_col = gtk.TreeViewColumn(_title, _renderer)
		_col.add_attribute(_renderer, 'text', _colnum)
		_col.add_attribute(_renderer, 'weight', OBSERVER_WEIGHT)
		_col.add_attribute(_renderer, 'editable', OBSERVER_EDIT)
		_col.set_alignment(0.0)
		_col.set_reorderable(True)
		_col.set_sort_column_id(_colnum)

		self.view.append_column(_col);

		self.browser.reporter.reportNote("Added variable '%s' to observer '%s'" % (_colname,self.name))


