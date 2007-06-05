import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango
import os.path

OBSERVER_EDIT_COLOR = "#008800"
OBSERVER_NOEDIT_COLOR = "#000088"
OBSERVER_NORMAL_COLOR = "black"

# This code uses the techniques described in
# http://www.daa.com.au/pipermail/pygtk/2006-February/011777.html
# http://piman.livejournal.com/361173.html

OBSERVER_NUM=0

class ClickableTreeColumn(gtk.TreeViewColumn):
	def __init__(self, title="", *args, **kwargs):
		super(ClickableTreeColumn, self).__init__(None, *args, **kwargs)
		self.label = gtk.Label("%s" % title)
		self.label.show()
		self.set_widget(self.label)

	def do_connect(self):
		""" Connect the defined 'on_click' method. Note: must be called after
		this object (ClickableTreeColumn) has been added to the TreeView,
		eg mytreeview.append_column(col). """
		button = self.label.get_ancestor(gtk.Button)
		h = button.connect("clicked",self.on_click)
		#button.clicked()
		
	def on_click(self,widget,*args):
		print "RECEIVED EVENT"

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
		if len(uname) or uname.find("/")!=-1:
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
			values=[]

		self.values = values
		self.active = active

	def make_static(self,table):
		self.active = False
		print "TABLE COLS:",table.cols
		print "ROW VALUES:",self.values
		_v = []
		for col in table.cols.values():
			_v.append( col.instance.getRealValue() )
		self.values = _v
		print "Made static, values:",self.values

	def get_values(self,table):
		if not self.active:
			vv = []
			for k,v in table.cols.iteritems():
				if k<len(self.values):
					vv.append(self.values[k]/v.units.getConversion())
			return vv
		else:
			return [col.instance.getRealValue()/col.units.getConversion() \
				for index,col in table.cols.iteritems() \
			]

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
		self.activeimg.set_from_file(os.path.join(browser.options.assets_dir,"active.png"))
		# create PixBuf objects from these?
		self.rows = []
		_store = gtk.TreeStore(object)
		self.cols = {}

		# create the 'active' pixbuf column
		_renderer = gtk.CellRendererPixbuf()
		_col = ClickableTreeColumn("")
		_col.pack_start(_renderer,False)
		_col.set_cell_data_func(_renderer, self.activepixbufvalue)
		self.view.append_column(_col)
		_col.do_connect()
		
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
		self.rows = []
		self.activeiter = _store.append(None, [ObserverRow()] )

	def plot(self,x=None,y=None,y2=None):
		"""create a plot from two columns in the ObserverTable"""
		import platform
		import matplotlib
		matplotlib.use('GTKAgg')
		import pylab
		pylab.ioff()
		if x is None or y is None:
			if len(self.cols)<2:
				raise Exception("Not enough columns to plot (need 2+)")
			if x is None:
				x=self.cols[0]
			if y is None:
				y=self.cols[1]

		if x.__class__ is int and x>=0 and x<len(self.cols):
			x=self.cols[x]
		if y.__class__ is int and y>=0 and y<len(self.cols):
			y=self.cols[y]
		if y2.__class__ is int and y2>=0 and y2<len(self.cols):
			y2=self.cols[y2]			

		ncols = 2
		if y2 is not None:
			ncols+=1

		A = pylab.zeros((len(self.rows),ncols),'f')
		for i in range(len(self.rows)):
			r = self.rows[i].get_values(self)
			A[i,0]=r[x.index]
			A[i,1]=r[y.index]
			if y2 is not None:
				A[i,2]=r[y2.index]
		pylab.figure()
		p1 = pylab.plot(A[:,0],A[:,1],'b-')
		pylab.xlabel(x.title)
		pylab.ylabel(y.title)

		if y2 is not None:
			ax2 = pylab.twinx()
			p2 = pylab.plot(A[:,0],A[:,2],'r-')
			pylab.ylabel(y2.title)
			ax2.yaxis.tick_right()
			pylab.legend([y.name,y2.name])

		pylab.ion()
		if platform.system()=="Windows":
			pylab.show()
		else:
			pylab.show(False)				
		
	def on_plot_clicked(self,*args):
		try:
			self.plot()
		except Exception,e:
			self.browser.reporter.reportError(str(e))

	def do_add_row(self,values=None):
		_store = self.view.get_model()
		if self.alive:
			_row = ObserverRow()
			self.rows.append(_row)
			_oldrow = _store.get_value(self.activeiter,0)
			_oldrow.make_static(self)
			self.activeiter = _store.append(None,[_row])
			_path = _store.get_path(self.activeiter)
			_oldpath,_oldcol = self.view.get_cursor()
			self.view.set_cursor(_path, _oldcol)
		else:
			_row = ObserverRow(values=values,active=False)
			self.rows.append(_row)
			_store.append(None,[_row])			
			#self.browser.reporter.reportNote("Added data row")

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
		#self.browser.reporter.reportNote("SYNC performed")

	def add_instance(self,instance):
		_col = ObserverColumn(instance,self.colindex,browser=self.browser)
		self.cols[self.colindex] = _col
		self.colindex = self.colindex + 1

		# create a new column
		_renderer = gtk.CellRendererText()
		_renderer.connect('edited',self.on_view_cell_edited, _col)
		_tvcol = ClickableTreeColumn(_col.title)
		_tvcol.pack_start(_renderer,False)
		_tvcol.set_cell_data_func(_renderer, _col.cellvalue)
		self.view.append_column(_tvcol);
		_tvcol.do_connect()
		#self.browser.reporter.reportError("cols = "+str(self.cols))

	def copy_to_clipboard(self,clip):
		_s = []
		_s.append('\t'.join([_v.title for _k,_v in self.cols.iteritems()]))
		#_cf = [_v.units.getConversion() for _k,_v in self.cols.iteritems()]
		print "COPYING %d ROWS" % len(self.rows)
		#print "CONVERSIONS:",_cf
		for _r in self.rows:
			_s.append("\t".join([`_v` for _v in _r.get_values(self)]))

		clip.set_text('\n'.join(_s),-1) 

		self.browser.reporter.reportNote("Observer '%s' data copied to clipboard" % self.name)
