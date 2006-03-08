import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango

OBSERVER_INITIAL_COLS = 3 # how many cells are at the start of the table?
OBSERVER_ICON, OBSERVER_WEIGHT, OBSERVER_EDIT = range(0,OBSERVER_INITIAL_COLS) # column indices for the start of the TreeStore
OBSERVER_NULL = 0 # value that gets added to empty cells in a new column

# This is messy code since it doesn't observe the convention of keeping your model
# separate from your view. It's all mixed up together. Yuck. Part of the
# difficulty with that was the fact that TreeStores don't support the adding of
# columns.

# Update: there is a technique for doing this, in fact:
# http://www.daa.com.au/pipermail/pygtk/2006-February/011777.html

class ObserverTab:
	def __init__(self,xml,name,browser,tab):
		xml.signal_autoconnect(self);

		self.view = xml.get_widget('observerview')
		self.tab = tab

		#self.activeimg = gtk.Image()
		#self.activeimg.set_from_file("glade/active.png")
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
		self.name = name
		self.browser = browser

		# create the 'active' pixvuf columns	
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

