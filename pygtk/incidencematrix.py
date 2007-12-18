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

		self.data = pylab.zeros((self.im.getNumRows(), self.im.getNumCols(), ))
		for i in _id:
			self.data[i.row, i.col] = int(i.type)

		del(_id)

		colors = {
			0: (1.,1.,1.)   # IM_NULL
			,1: (0.,1.,0.)  # IM_ACTIVE_FIXED
			,2: (0.,0.,0.3) # IM_ACTIVE_FREE
			,3: (1.,0.5,0.)  # IM_DORMANT_FIXED
			,4: (1.,0.,0.)  # IM_DORMANT_FREE
		}

		red = []
		green = []
		blue = []	
		n = 4
		for k,v in colors.iteritems():
			red.append((float(k)/n, v[0], v[0]))
			green.append((float(k)/n, v[1], v[1]))
			blue.append((float(k)/n, v[2], v[2]))

		cmapdata = {'red':tuple(red), 'green':tuple(green), 'blue':tuple(blue)}

		# prepare colour map
#		cmapdata = {
#			          # IM_NULL      IM_ACTIVE_FIXED IM_DORMANT_FREE IM_DORMANT_FIXED IM_ACTIVE_FREE
#			'red'  :  ((0., 1., 1.), (1., 0., 0.), (xx,                            (0.5, 0., 0.)),
#			'green':  ((0., 1., 1.), (1., 0., 0.)                                   , (0.5, 1., 1.)),
#			'blue' :  ((0., 1., 1.), (1., 0.3, 0.3)                                 , (0.5, 0., 0.))
#		}

		_im_cmap =  LinearSegmentedColormap('im_cmap',  cmapdata, n+1)

		pylab.ioff()
		pylab.figure()
		ax = pylab.subplot(111)
		ax.axis('equal') # aspect ratio = 1.0
		ax.imshow(self.data, cmap=_im_cmap, interpolation='nearest',vmin=0, vmax=n) 
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
		
			_col = int(x+0.5)
			_row = int(y+0.5)


			try:
				if self.data[_row, _col] == 0:
					#print "nothing here"
					return ""
	
				if self.lastrow != None and self.lastcol != None:
					if self.lastrow == _row and self.lastcol == _col:
						return self.lastmsg

				_var = self.im.getVariable(_col);
				_rel = self.im.getRelation(_row);
				_blk = self.im.getBlockRow(_row);
			except IndexError:
				return "[out of range]"

			print "row = %d, col = %d" % (_row,_col)

			self.lastrow = _row;
			self.lastcol = _col; 
			self.lastmsg = "rel '%s', var '%s': block %d" %(_rel,_var,_blk)
			print self.lastmsg
			return self.lastmsg

