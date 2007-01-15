import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango
import os.path

CHANGED_COLOR = "#FFFF88"
SOLVERPARAM_BOOL_TRUE = "Yes"
SOLVERPARAM_BOOL_FALSE = "No"

#======================================================
# SOLVER PARAMETERS WINDOW

class SolverParametersWindow:
	def __init__(self,browser,params,name,parent=None):
		if parent==None:
			self.parent = browser
		self.reporter = browser.reporter
		self.params = params
		self.assets_dir = browser.options.assets_dir

		_xml = gtk.glade.XML(browser.glade_file,"paramswin")
		self.window = _xml.get_widget("paramswin")
		self.window.set_transient_for(self.parent.window)

		self.paramdescription = _xml.get_widget("paramdescription")
		self.paramname = _xml.get_widget("paramname")
		self.solvername = _xml.get_widget("solvername")

		_xml.signal_autoconnect(self)

		self.solvername.set_text(name)
		
		self.paramsview = _xml.get_widget("paramsview")	
		self.otank = {}
		self.paramstore = gtk.TreeStore(str,str,str,bool,str,int)
		self.paramsview.set_model(self.paramstore)

		# name column
		_renderer0 = gtk.CellRendererText()
		_col0 = gtk.TreeViewColumn("Name", _renderer0, text=0, background=4, weight=5)
		self.paramsview.append_column(_col0)

		# value column: 'editable' set by column 3 of the model data.
		_renderer1 = gtk.CellRendererText()	
		_renderer1.connect('edited',self.on_paramsview_edited)
		_col1 = gtk.TreeViewColumn("Value", _renderer1, text=1, editable=3, background=4)
		self.paramsview.append_column(_col1)

		# range column
		_renderer2 = gtk.CellRendererText()
		_col2 = gtk.TreeViewColumn("Range", _renderer2, text=2, background=4)
		self.paramsview.append_column(_col2)

		self.populate()

		self.paramsview.expand_all()	

	def on_paramsview_row_activated(self,treeview,path,view_column,*args,**kwargs):
		# get back the object we just clicked

		if not self.otank.has_key(path):
			return
		
		_iter,_param = self.otank[path]

		if _param.isBool():
			newvalue = not _param.getBoolValue()
			_param.setBoolValue(newvalue)
			if newvalue:
				self.paramstore.set_value(_iter,1,SOLVERPARAM_BOOL_TRUE)
			else:
				self.paramstore.set_value(_iter,1,SOLVERPARAM_BOOL_FALSE)
			self.paramstore.set_value(_iter,4, CHANGED_COLOR)

	def on_paramsview_button_press_event(self,widget,event):
		if event.button == 1:
			_x = int(event.x)
			_y = int(event.y)
			_time = event.time
			_pathinfo = self.paramsview.get_path_at_pos(_x, _y)
			if _pathinfo != None:
				_path, _col, _cellx, _celly = _pathinfo
				if not self.otank.has_key(_path):
					return
				_iter, _param = self.otank[_path]

				# update the description field
				self.paramdescription.set_text(_param.getDescription())
				self.paramname.set_text(_param.getName())

				if _param.isStr():
					_menu = gtk.Menu();
					_head = gtk.ImageMenuItem("Options",True)
					_head.show()
					_head.set_sensitive(False)
					_img = gtk.Image()
					_img.set_from_file(os.path.join(self.assets_dir,'folder-open.png'))

					_head.set_image(_img)
					_menu.append(_head)
					_sep = gtk.SeparatorMenuItem(); _sep.show()
					_menu.append(_sep);

					_item = None;
					for i in _param.getStrOptions():
						_item = gtk.RadioMenuItem(group=_item, label=i);
						if i == _param.getStrValue():
							_item.set_active(True)
						else:
							_item.set_active(False)
						_item.show()
						_item.connect('activate', self.on_menu_activate, _param, _iter, i);
						_menu.append(_item)
									
					_menu.show()
					_menu.popup(None, None, None, event.button, _time)

	def on_menu_activate(self, menuitem, param, iter, newvalue):
		if param.getStrValue() != newvalue:
			param.setStrValue(newvalue)
			self.paramstore.set_value(iter, 1, newvalue)
			self.paramstore.set_value(iter, 4, CHANGED_COLOR)
		else:
			print "NOT CHANGED"
	
	def on_paramsview_cursor_changed(self, *args, **kwargs):
		_path, _col = self.paramsview.get_cursor()
		if not self.otank.has_key(_path):
			self.paramdescription.set_text("")
			self.paramname.set_text("")
			return
		_iter, _param = self.otank[_path]
		self.paramdescription.set_text(_param.getDescription())	
		self.paramname.set_text(_param.getName())
		#self.paramsview.set_cursor(_path,self.paramsview.get_column(1));		

	def on_paramsview_edited(self, renderer, path, newtext, **kwargs):
		# get back the Instance object we just edited (having to use this seems like a bug)
		path = tuple( map(int,path.split(":")) )

		if not self.otank.has_key(path):
			raise RuntimeError("cell_edited_callback: invalid path '%s'" % path)
			return
		
		_iter,_param = self.otank[path]
		# you can only edit real, int, str:

		_changed = False
		if _param.isInt():
			newvalue = int(newtext)
			if _param.isBounded():
				if newvalue > _param.getIntUpperBound():
					self.doErrorDialog()
					return False
				if newvalue < _param.getIntLowerBound():
					self.doErrorDialog()
					return False
			if _param.getIntValue() != newvalue:
				_param.setIntValue(newvalue)
				_changed = True
		elif _param.isReal():
			newvalue = float(newtext)
			if _param.isBounded():
				if newvalue > _param.getRealUpperBound():
					self.doErrorDialog()
					return False
				if newvalue < _param.getRealLowerBound():
					self.doErrorDialog()
					return False
			if _param.getRealValue() != newvalue:
				_param.setRealValue(newvalue)
				_changed = True
		elif _param.isStr():
			newvalue = str(newtext)
			if _param.getStrValue() != newvalue:
				_param.setStrValue(newvalue)
				_changed = True

		if _changed:
			self.paramstore.set_value(_iter, 1, newvalue)
			self.paramstore.set_value(_iter, 4, CHANGED_COLOR)			
		else:
			print "NO CHANGE"

	def create_row_data(self,p):
		_row = [p.getLabel()];
		if p.isStr():
			_row.extend([p.getStrValue(), str(len(p.getStrOptions()))+" options", False]);
		elif p.isBool():
			if p.getBoolValue():
				_val = SOLVERPARAM_BOOL_TRUE
			else:
				_val = SOLVERPARAM_BOOL_FALSE
			_row.extend([_val,"",False])
		elif p.isReal():
			if not p.isBounded():
				_row.extend([str(p.getRealValue()), "",True])
			else:
				_row.extend([str(p.getRealValue()), "[ "+str(p.getRealLowerBound())+", "+str(p.getRealUpperBound())+" ]",True])
		elif p.isInt():
			if not p.isBounded():
				_row.extend([str(p.getIntValue()), "", True])
			else:
				_row.extend([str(p.getIntValue()), "[ "+str(p.getIntLowerBound())+", "+str(p.getIntUpperBound())+" ]", True])

		else:
			raise RuntimeError("invalid type")

		_row.extend(["white",pango.WEIGHT_NORMAL])
		return _row;
		
	def populate(self):
		# Fill the paramstore with data
		
		data = {}
		for i in self.params:
			if not data.has_key(i.getPage()):
				data[i.getPage()] = {}
			data[i.getPage()][i.getNumber()] = i;

		_pagenum = 1;
		for _page in sorted(data.keys()):
			if len(data[_page].keys()):
				_pageiter = self.paramstore.append( None, ["Page "+str(_pagenum), "", "", False, "white", pango.WEIGHT_BOLD])
				for _number in sorted(data[_page].keys()):
					_param = data[_page][_number]
					_piter = self.paramstore.append( _pageiter, self.create_row_data(_param) )
					_path = self.paramstore.get_path(_piter)
					self.otank[ _path ] = (_piter, _param)
				_pagenum = _pagenum + 1

	def doErrorDialog(self,msg=None):
		_dialog = gtk.Dialog("Out of bounds", parent=self.window, flags=gtk.DIALOG_MODAL, buttons=(gtk.STOCK_OK, gtk.RESPONSE_OK) )	
		if msg:
			_label = gtk.Label(msg)
		else:
			_label = gtk.Label("Please enter a value that is within the\ndisplayed upper and lower bounds")

		_dialog.vbox.pack_start(_label, True, True, 0)
		_label.show()
		_dialog.run()
		_dialog.destroy()

	def run(self):
		_res = self.window.run()
		self.window.destroy()
		return _res
