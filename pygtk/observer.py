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
		except KeyError:
			_dataval = ""

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
