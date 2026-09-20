import config
import loading
import platform
from gi.repository import Gtk

try:
	import matplotlib		
	from matplotlib.colors import LinearSegmentedColormap
	from matplotlib.patches import Patch
	import pylab

except:
	pass

class IncidenceMatrixWindow:

	def __init__(self,im):
		self.im = im # IncidenceMatrix object
		self.lastcol = None;
		self.lastrow = None;
		self.mode = "real"

		loading.load_matplotlib(throw=True)

	def run(self):
		_id = None
		try:
			_id = self.im.getDecompIncidenceData()
			self.mode = "decomp"
			rows = self.im.getDecompNumRows()
			cols = self.im.getDecompNumCols()
			title = "Mixed Incidence / Decomposition Matrix"
			xlabel = "Vars / dvars"
			ylabel = "Rels / logrels"
			legend = self._decomp_legend()
			n = 3
			colors = self._decomp_colors()
		except RuntimeError:
			_id = self.im.getIncidenceData()
			self.mode = "real"
			rows = self.im.getNumRows()
			cols = self.im.getNumCols()
			title = "Incidence Matrix"
			xlabel = "Variables"
			ylabel = "Relations"
			legend = self._real_legend()
			n = 4
			colors = self._real_colors()

		self.data = pylab.zeros((rows, cols, ))
		if self.mode == "decomp":
			self.blockdata = self._decomp_block_data(rows, cols)
			for i in _id:
				self.data[i.row, i.col] = self._decomp_plot_type(int(i.type))
		else:
			self.blockdata = None
			for i in _id:
				self.data[i.row, i.col] = int(i.type)

		red = []
		green = []
		blue = []	
		for k,v in sorted(colors.items()):
			red.append((float(k)/n, v[0], v[0]))
			green.append((float(k)/n, v[1], v[1]))
			blue.append((float(k)/n, v[2], v[2]))

		cmapdata = {'red':tuple(red), 'green':tuple(green), 'blue':tuple(blue)}

		_im_cmap =  LinearSegmentedColormap('im_cmap',  cmapdata, n+1)

		pylab.ioff()
		f = pylab.figure()
		ax = f.add_subplot(111)
		if self.blockdata is not None:
			ax.imshow(
				self.blockdata, cmap=_im_cmap, interpolation='nearest',
				vmin=0, vmax=n
			)
			_im_cmap.set_bad((1.,1.,1.,0.))
			plotdata = pylab.ma.masked_where(self.data == 0, self.data)
			ax.imshow(
				plotdata, cmap=_im_cmap, interpolation='nearest',
				vmin=0, vmax=n
			)
		else:
			ax.imshow(
				self.data, cmap=_im_cmap, interpolation='nearest',
				vmin=0, vmax=n
			)
		pylab.title(title)
		pylab.xlabel(xlabel)
		pylab.ylabel(ylabel)
		if legend:
			ax.legend(
				handles=legend,
				loc='upper left',
				bbox_to_anchor=(1.02, 1.0),
				borderaxespad=0.,
				fontsize='small'
			)
			f.subplots_adjust(right=0.74)
		ax.format_coord = self.incidence_get_coord_str
		pylab.ion()
		pylab.show()

	def _real_colors(self):
		return {
			0: (1.,1.,1.)   # IM_NULL
			,1: (0.,1.,0.)  # IM_ACTIVE_FIXED
			,2: (0.,0.,0.3) # IM_ACTIVE_FREE
			,3: (1.,0.5,0.)  # IM_DORMANT_FIXED
			,4: (1.,0.,0.)  # IM_DORMANT_FREE
		}

	def _decomp_colors(self):
		return {
			0: (1.,1.,1.)   # IM_NULL
			,1: (0.92,0.92,0.92) # block background
			,2: (0.,0.,0.)       # rel/var incidence
			,3: (0.,0.34,1.)     # logrel/dvar incidence
		}

	def _real_legend(self):
		return [
			Patch(facecolor=(0.,1.,0.), label="active fixed"),
			Patch(facecolor=(0.,0.,0.3), label="active free"),
			Patch(facecolor=(1.,0.5,0.), label="dormant fixed"),
			Patch(facecolor=(1.,0.,0.), label="dormant free"),
		]

	def _decomp_legend(self):
		colors = self._decomp_colors()
		return [
			Patch(facecolor=colors[1], label="block"),
			Patch(facecolor=colors[2], label="rel/var"),
			Patch(facecolor=colors[3], label="logrel/dvar"),
		]

	def _decomp_plot_type(self,pointtype):
		if pointtype in (5, 6):
			return 2
		if pointtype in (7, 8, 9, 10):
			return 3
		return 0

	def _decomp_block_data(self,rows,cols):
		data = pylab.zeros((rows, cols, ))
		for b in self.im.getDecompBlockSummaries():
			data[
				b.row_low:b.row_high + 1,
				b.col_low:b.col_high + 1
			] = 1
		return data

	def incidence_get_coord_str(self,x,y):
		
			_col = int(x+0.5)
			_row = int(y+0.5)

			try:
				if (_row < 0 or _col < 0
						or _row >= self.data.shape[0]
						or _col >= self.data.shape[1]):
					return "[out of range]"
				if self.data[_row, _col] == 0:
					#print "nothing here"
					return ""
	
				if self.lastrow != None and self.lastcol != None:
					if self.lastrow == _row and self.lastcol == _col:
						return self.lastmsg

				if self.mode == "decomp":
					_var = self.im.getDecompColLabel(_col)
					_rel = self.im.getDecompRowLabel(_row)
					_vkind = self.im.getDecompColKind(_col)
					_rkind = self.im.getDecompRowKind(_row)
					_blk = self._decomp_block_for_row(_row)
				else:
					_var = self.im.getVariable(_col);
					_rel = self.im.getRelation(_row);
					_vkind = "variable"
					_rkind = "relation"
					_blk = self.im.getBlockRow(_row);
			except IndexError:
				return "[out of range]"
			except RuntimeError:
				return "[out of range]"

			#print("row = %d, col = %d" % (_row,_col))

			self.lastrow = _row;
			self.lastcol = _col; 
			self.lastmsg = "%s '%s', %s '%s': block %d" %(
				_rkind,_rel,_vkind,_var,_blk
			)
			#print(self.lastmsg)
			return self.lastmsg

	def _decomp_block_for_row(self,row):
		for b in self.im.getDecompBlockSummaries():
			if b.row_low <= row <= b.row_high:
				return b.block
		return -1
