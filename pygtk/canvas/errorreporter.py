import pango, ascpy, gtk
		
class ErrorReporter():
	
	def __init__(self, ascpyreporter,iconok,iconinfo,iconwarning,iconerror):		
		self.iconok = iconok
		self.iconinfo = iconinfo
		self.iconwarning = iconwarning
		self.iconerror = iconerror

		# set up the error view		
		self.errorview = gtk.TreeView()
		#print dir(self.errorview)
		#self.errorview.set_headers_visible(True)	
		errstorecolstypes = [gtk.gdk.Pixbuf,str,str,str,int]
		self.errorstore = gtk.TreeStore(*errstorecolstypes)
		errtitles = ["","Location","Message"];
		self.errorview.set_model(self.errorstore)
		self.errcols = [ gtk.TreeViewColumn() for _type in errstorecolstypes]

		i = 0
		for tvcolumn in self.errcols[:len(errtitles)]:
			tvcolumn.set_title(errtitles[i])
			self.errorview.append_column(tvcolumn)			

			if i>0:
				_renderer = gtk.CellRendererText()
				tvcolumn.pack_start(_renderer, True)				
				tvcolumn.add_attribute(_renderer, 'text', i)
				if(i==2):
					tvcolumn.add_attribute(_renderer, 'foreground', 3)
					tvcolumn.add_attribute(_renderer, 'weight', 4)
			else:
				_renderer1 = gtk.CellRendererPixbuf()
				tvcolumn.pack_start(_renderer1, False)				
				tvcolumn.add_attribute(_renderer1, 'pixbuf', int(0))

			i = i + 1


		#--------------------
		# set up the error reporter callback
		self.reporter = ascpyreporter
		self.reporter.setPythonErrorCallback(self.error_callback)		
		
		


#   ----------------------------------
#   ERROR PANEL

	def get_error_row_data(self,sev,filename,line,msg):
		try:
			_sevicon = {
				0   : self.iconok
				,1  : self.iconinfo
				,2  : self.iconwarning
				,4  : self.iconerror
				,8  : self.iconinfo
				,16 : self.iconwarning
				,32 : self.iconerror
				,64 : self.iconerror
			}[sev]
		except KeyError:
			_sevicon = self.iconerror

		_fontweight = pango.WEIGHT_NORMAL
		if sev==32 or sev==64:
			_fontweight = pango.WEIGHT_BOLD
		
		_fgcolor = "black"
		if sev==8:
			_fgcolor = "#888800"
		elif sev==16:
			_fgcolor = "#884400"
		elif sev==32 or sev==64:
			_fgcolor = "#880000"
		elif sev==0:
			_fgcolor = "#448844"
		
		if not filename and not line:
			_fileline = ""
		else:
			if(len(filename) > 25):
				filename = "..."+filename[-22:]
			_fileline = filename + ":" + str(line)

		_res = (_sevicon,_fileline,msg.rstrip(),_fgcolor,_fontweight)
		return _res  

	def error_callback(self,sev,filename,line,msg):
		pos = self.errorstore.append(None, self.get_error_row_data(sev, filename,line,msg))
		path = self.errorstore.get_path(pos)
		col = self.errorview.get_column(3)
		self.errorview.scroll_to_cell(path,col)		
		return 0;
