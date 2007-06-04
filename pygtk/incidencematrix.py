import config
import loading
import platform

try:
	import matplotlib		
	import pylab
	from matplotlib.colors import LinearSegmentedColormap
except:
	pass

class IncidenceMatrixWindow:

	def __init__(self,im):
		self.im = im # IncidenceMatrix object
		self.lastcol = None;
		self.lastrow = None;

		loading.load_matplotlib(throw=True)

	def run(self):
		# convert incidence map to pylab numarray type:
		_id = self.im.getIncidenceData();

		self.data = pylab.zeros((self.im.getNumRows(), self.im.getNumCols(), ))*0.
		for i in _id:
			self.data[i.row, i.col] = int(i.type)

		del(_id)

		# prepare colour map
		cmapdata = {
			          # type = 0     type = 1       type = 2
			          # norelation   active fixed   active free
			'red'  :  ((0., 1., 1.), (0.5, 0., 0.), (1., 0., 0.)),
			'green':  ((0., 1., 1.), (0.5, 1., 1.), (1., 0., 0.)),
			'blue' :  ((0., 1., 1.), (0.5, 0., 0.), (1., 0.3, 0.3))
		}

		_im_cmap =  LinearSegmentedColormap('im_cmap',  cmapdata, 4)

		pylab.ioff()
		pylab.figure()
		ax = pylab.subplot(111)
		ax.axis('equal') # aspect ratio = 1.0
		ax.imshow(self.data, cmap=_im_cmap, interpolation='nearest') 
			# integer 'type' values become reals 0..1, which are then coloured
			# according to cmapdata
		pylab.title("Incidence Matrix")
		pylab.xlabel("Variables")
		pylab.ylabel("Relations")
		#pylab.connect('motion_notify_event',self.on_sparsity_motion_notify)
		ax.format_coord = self.incidence_get_coord_str
		pylab.ion()
		if platform.system()=="Windows":
			pylab.show()
		else:
			pylab.show(False)

	def incidence_get_coord_str(self,x,y):
		
			_col = int(x)
			_row = (self.im.getNumRows()-1) - int(y)

			try:
				if self.data[_row, _col] == 0:
					return ""
	
				if self.lastrow != None and self.lastcol != None:
					if self.lastrow == _row and self.lastcol == _col:
						return self.lastmsg

				_var = self.im.getVariable(_col);
				_rel = self.im.getRelation(_row);
				_blk = self.im.getBlockRow(_row);
			except IndexError:
				return

			self.lastrow = _row;
			self.lastcol = _col; 
			self.lastmsg = "rel '%s', var '%s': block %d" %(_rel,_var,_blk)
			return self.lastmsg

