
import platform, os
import gtk

class WinFileFilter:
	def __init__(self,name,patternarray):
		self.name = name + " (" + ", ".join(patternarray) + ")"
		self.pattern = ";".join(patternarray)
	def __repr__(self):
		return '%s\0%s' % (self.name, self.pattern)

class FileChooser:
	def __init__(self):
		self.ext = {}
		self.filters = []
		if platform.system()=="Windows":
			try:
				import win32gui
				self.iswin = True
				return
			except ImportError:
				pass
		self.iswin = False
		self.chooser = gtk.FileChooserDialog()
		self.chooser.add_buttons(gtk.STOCK_OPEN,42)

	
	def add_filter(self,name,patternarray):
		if self.iswin:
			self.filters.append(WinFileFilter(name,patternarray))
		else:
			_f = gtk.FileFilter()
			_f.set_name(name)
			for _p in patternarray:
				_f.add_pattern(_p)
			self.chooser.add_filter(_f)
	
	def do(self):     
		if self.iswin:
			return self.do_windows()
		else:
			return self.do_gtk()

	def do_windows(self):
		import win32gui, win32con
		_fa = []
		for _f in self.filters:
			_fa.append(repr(_f))
		filter='\0'.join(_fa)+'\0'
		customfilter='Other files (*.*)\0*.*\0'
		print "FILTER:",repr(filter)
		
		fname = "ERROR"
		
		try:
			fname,customfilter,flags = win32gui.GetOpenFileNameW(
				InitialDir=os.getcwd(),
				Flags=win32con.OFN_EXPLORER,
				File='', DefExt='py',
				Title='Open File...',
				Filter=filter,
				CustomFilter=customfilter,
				FilterIndex=1
			)
		except Exception, e:
			if hasattr(e,'what'):
				print e.what()
			raise RuntimeError("File select error!")
                
		return fname

	def do_gtk(self):
		print "LAUNCHING..."
		self.add_filter("Other files",["*.*"])
		self.chooser.run()
		print "DONE..."
		return self.chooser.get_filename()
		

f = FileChooser()
f.add_filter("ASCEND files",["*.a4c","*.a4l"])
print "SELECTED FILE",f.do()
