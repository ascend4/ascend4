import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango

class ObserverTab:
	def __init__(self,xml,name,browser):
		xml.signal_autoconnect(self);

		self.view = xml.get_widget('observerview')

		#self.activeimg = gtk.Image()
		#self.activeimg.set_from_file("icons/active.png")
		self.activeimg = None
		self.keptimg = None
		
		# no instances yet in the observer:
		self.columninstances = []

		self.columns = [gtk.gdk.Pixbuf]
		self.titles = ["T"]
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
		_col.set_title(self.titles[0])
		_col.pack_start(_renderer,False)
		_col.add_attribute(_renderer, 'pixbuf', 0)
		self.view.append_column(_col);

		# create the first row
		print "Adding row",self.rows[0],"to store"
		_store.append(None, self.make_row(self.activeimg, self.rows[0]) )

		self.view.set_model(_store)

		self.browser.reporter.reportNote("Created OBSERVER")

	def on_add_clicked(self,*args):
		_rownum = len(self.rows)

		# add a copy of the last row
		self.rows.append(self.rows[_rownum-1])
		self.activerow = _rownum

		self.view.get_model().set(self.activeiter,0,self.keptimg)
		self.activeiter = self.view.get_model().append(None,self.make_row(self.activeimg,self.rows[_rownum]))

	def on_clear_clicked(self,*args):
		print "CLEAR"
		pass

	def sync(self):
		print "SYNCHING ROW",self.activerow
		
		#new row data
		_r = [_i.getRealValue() for _i in self.columninstances]

		# stick the row data into the TreeStore
		_m = self.view.get_model()
		_i = 1
		for _c in _r:
			_m.set(self.activeiter, _i, _c)
			_i = _i + 1
		print "REPLACEMENT DATA IS",_r

		# keep the data in self.rows as well
		self.rows[self.activerow] = _r;

	def make_row(self,img,row):
		_r = [img]
		for _c in row:
			_r.append(_c)
		print "MADE KEPT ROW:",_r
		return _r		

	def add_instance(self, inst):
		if not inst.getType().isRefinedSolverVar():
			self.browser.reporter.reportError("Instance is not a refined solver variable: can't 'observe'.");
			return

		_colnum = len(self.columns)
		_colname = inst.getName().toString()
		_value = float(inst.getRealValue())
		_rownum = len(self.rows)-1

		# store the instances in self.columninstances for sync purposes
		self.columninstances.append(inst)
	
		print "ROWS IN self.rows",len(self.rows)
		print "LAST ROW: ",repr(self.rows[_rownum])

		print "COLNUM =",_colnum

		print "ADDING TO OBSERVER '"+self.name+"' COLUMN '"+_colname+"', CURRENT VALUE = ",_value

		# create new TreeStore, copy of old, plus one columm
		self.columns.append(float)
		self.titles.append(_colname);
		print "NEW COLUMNS WILL BE",str(self.columns)

		_store = gtk.TreeStore(*(self.columns))

		_iter = None
		_i = 0
		for _r in self.rows:
			print "ROW",_i
			_r.append(0)
			_iter = _store.append(None, self.make_row(self.keptimg,_r) )
			_i = _i + 1
	
		self.activeiter = _iter

		# add newest data point in bottom-right
		self.rows[_rownum][_colnum-1] = _value
		_store.set_value(self.activeiter, _colnum, _value)

		# re-assign store to TreeView
		self.view.set_model(_store)		#self.store.set(_currentrow, _colnum, _value)

		# add value to current column
		#self.store.set(_currentrow, _colnum, _value)

		_renderer = gtk.CellRendererText()
		_col = gtk.TreeViewColumn(_colname, _renderer)
		_col.add_attribute(_renderer, 'text', _colnum)
		_col.set_alignment(0.0)
		#_col.set_reorderable(True)
		#_col.set_sort_column_id(_colnum)

		self.view.append_column(_col);

