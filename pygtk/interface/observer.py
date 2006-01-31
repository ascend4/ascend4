import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango

class ObserverTab:
	def __init__(self,xml,name,browser):
		xml.signal_autoconnect(self);

		self.view = xml.get_widget('observerview')

		self.activeimg = self.view.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)

		self.columns = [gtk.gdk.Pixbuf]
		self.titles = ["T"]
		self.store = gtk.TreeStore(*self.columns)
		self.rows = []
		self.rows.append([self.activeimg])

		self.name = name
		self.browser = browser


		# create the 'active' pixvuf columns	
		_col = gtk.TreeViewColumn()
		_col.set_title(self.titles[0])
		self.view.append_column(_col);
		_renderer = gtk.CellRendererPixbuf()
		_col.pack_start(_renderer,False)
		_col.add_attribute(_renderer, 'pixbuf', int(0))

		# create the first row
		print "Adding row",self.rows[0],"to store"
		self.store.append(None, self.rows[0])

		self.browser.reporter.reportNote("Created OBSERVER")

	def on_add_click(self,*args):
		print "ADD"
		pass

	def on_clear_click(self,*args):
		print "CLEAR"
		pass

	def add_instance(self, inst):
		if not inst.getType().isRefinedSolverVar():
			self.browser.reporter.reportError("Instance is not a refined solver variable: can't 'observe'.");
			return

		_colnum = len(self.columns)
		_colname = inst.getName().toString()
		_value = float(inst.getRealValue())
		_rownum = len(self.rows)-1

		print "COLNUM =",_colnum

		print "ADDING TO OBSERVER '"+self.name+"' COLUMN '"+_colname+"', CURRENT VALUE = ",_value

		# create new TreeStore, copy of old, plus one columm
		self.columns.append(float)
		self.titles.append(_colname);
		print "NEW COLUMNS WILL BE",str(self.columns)
		_store = gtk.TreeStore(*(self.columns))
		_currentrow = None
		for _r in self.rows:
			print "OLD ROW DATA:",repr(_r)
			_newrowdata = _r.append(None)
			print "NEW ROW DATA:",repr(_r)
			_currentrow = _store.append(None,_newrowdata)

		# add newest data point in bottom-right
		self.rows[_rownum][_colnum] = _value
		_store.set_value(_currentrow, _colnum, _value)

		# kill old self.store
		del(self.store)
		self.store = _store

		# re-assign store to TreeView
		self.view.set_model(self.store)		#self.store.set(_currentrow, _colnum, _value)

		# add value to current column
		#self.store.set(_currentrow, _colnum, _value)

		_renderer = gtk.CellRendererText()
		_col = gtk.TreeViewColumn(_colname, _renderer)
		_col.set_alignment(1.0)
		_col.set_reorderable(True)
		_col.set_sort_column_id(_colnum)

		self.view.append_column(_col);

		print "SELF.STORE: COLUMNS =",self.store.get_n_columns()," ROWS =",self.store.iter_n_children(None)


		print "LAST ROW DATA:",repr(self.rows[_rownum])	

		_rn = 0
		_r = self.store.iter_children(None)
		while _r != None:
			for _i in range(0, self.store.get_n_columns()):
				print "ROW",_rn,"COL",_i,":",self.store.get(_r,_i)
			_r = self.store.iter_next(_r)
			_rn = _rn + 1

