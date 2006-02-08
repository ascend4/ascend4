import gtk
import gtk.glade
import ascend
import pylab;
import matplotlib 
matplotlib.use('GTK') 
from matplotlib.figure import Figure 
from matplotlib.axes import Subplot 
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.backends.backend_gtk import FigureCanvasGTK, NavigationToolbar 
from itertools import groupby
from operator import itemgetter

class DiagnoseWindow:
	def __init__(self,GLADE_FILE,browser):
		self.browser=browser
		_xml = gtk.glade.XML(GLADE_FILE,"diagnosewin")
		_xml.signal_autoconnect(self)	

		self.window = _xml.get_widget("diagnosewin")
		self.view = _xml.get_widget("canvasvbox")
		self.blockentry = _xml.get_widget("blockentry")

		self.varview = _xml.get_widget("varview")
		self.varbuf = gtk.TextBuffer()
		self.varview.set_buffer(self.varbuf)
		self.varcollapsed = _xml.get_widget("varcollapsed")
		self.relview = _xml.get_widget("relview")	
		self.relcollapsed = _xml.get_widget("relcollapsed")
		self.relvalues = _xml.get_widget("relvalues")
		self.rellabels = _xml.get_widget("rellabels")
		self.relrels = _xml.get_widget("relrels")
		self.relresids = _xml.get_widget("relresids")
		self.relbuf = gtk.TextBuffer()
		self.relview.set_buffer(self.relbuf)

		self.prepare_data()
		self.fill_values(0) # block zero

	def run(self):
		self.window.run()
		self.window.hide()

	def prepare_data(self):
		# convert incidence map to pylab numarray type:
		print "PREPARING DATA"
		self.im = self.browser.sim.getIncidenceMatrix()
		self.data = self.im.getIncidenceData()
		print "DATA LOADED"

		self.canvas = None
	
	def fill_values(self, block):
		print "FILL VALUES %d" % block
		self.block = block
		self.blockentry.set_text(str(block))
		
		if self.canvas:
			self.view.remove(self.canvas)

		# This is not going to be very efficient at this stage:
		mtx = pylab.zeros((self.im.getNumRows(), self.im.getNumCols(), ))*0.
		for i in self.data:
			mtx[i.row, i.col] = int(i.type)

		# prepare colour map
		cmapdata = {
			          # type = 0     type = 1       type = 2
			          # norelation   active fixed   active free
			'red'  :  ((0., 1., 1.), (0.5, 0., 0.), (1., 0., 0.)),
			'green':  ((0., 1., 1.), (0.5, 1., 1.), (1., 0., 0.)),
			'blue' :  ((0., 1., 1.), (0.5, 0., 0.), (1., 0.3, 0.3))
		}

		_im_cmap =  LinearSegmentedColormap('im_cmap',  cmapdata, 4)

		self.figure = pylab.Figure(figsize=(6,4), dpi=72)
		_axes = self.figure.add_subplot(111)
		_axes.set_xlabel('Variables') 
		_axes.set_ylabel('Relations') 
		_axes.set_title('Block Incidence Matrix') 
		_axes.grid(True) 
		_axes.imshow(mtx, cmap=_im_cmap, interpolation='nearest')

		self.canvas = FigureCanvasGTK(self.figure) # a gtk.DrawingArea 
		self.canvas.show()

		self.view.pack_start(self.canvas, True, True)

		self.fill_var_names()
		self.fill_rel_names()

	def fill_var_names(self):
		names = [str(i) for i in self.im.getBlockVars(self.block)]
		if self.varcollapsed.get_active():
			res = collapse(names)
			rows = []
			for k in res:
				if k=="":
					for r in res[k]:
						rows.append(r)
				else:
					rows.append( '%s:\n\t%s' % (k, "\n\t".join(res[k])) )
			text = "\n".join(rows)
		else:
			text = "\n".join(names)
		self.varbuf.set_text(text)

	def fill_rel_names(self):
		names = [str(i) for i in self.im.getBlockRels(self.block)]
		if self.relcollapsed.get_active():
			text = "\n".join(collapse(names))
		else:
			text = "\n".join(names)
		self.relbuf.set_text(text)

	def set_block(self, block):
		self.block = block;
		self.fill_values(block)

	def on_varcollapsed_toggled(self,*args):
		print "COLLAPSED-TOGGLED"
		self.fill_var_names()

	def on_relcollapsed_toggled(self,*args):
		print "COLLAPSED-TOGGLED"
		self.fill_rel_names()

	def on_nextbutton_clicked(self,*args):
		self.set_block(self.block + 1)

	def on_prevbutton_clicked(self,*args):
		self.set_block(self.block - 1)		

	def on_blockentry_changed(self,*args):
		self.set_block( int(self.blockentry.get_text()) )

# The following is from 
# http://www.experts-exchange.com/Programming/Programming_Languages/Python/Q_21719649.html

def fold(data):
    """ fold sorted numeric sequence data into ranged representation:
    >>> fold([1,  4,5,6, 10, 15,16,17,18, 22, 25,26,27,28])
    '[1,4-6,10,15-18,22,25-28]'
    """
    folded = []
    for k, g in groupby(enumerate(data), lambda (i,x):i-x):
        seq = map(itemgetter(1), g)
        if len(seq) > 1:
            x = '%s-%s' % (seq[0], seq[-1])
        else:
            x = str(seq[0])
        folded.append(x)
    return folded and '[%s]' % ','.join(folded) or ''

def collapse(names):
    """reduce a list of items into something more readable:
    >>> data = 'C.x C.p C.T C.delta[1] C.delta[2] C.delta[3] C.sat.x C.sat.p C.h C.delta[5]'.split()
    >>> res = reduce(data)
    >>> for k in sorted(res):
    ...   print '%s: %s' % (k, res[k])
    C: T, delta[1-3,5], h, p, x
    C.sat: p, x
    """
    data = sorted([n.split('.') for n in names], key=len)
    res = {}
    for k, g in groupby(data, lambda x: len(x)):
        item = g.next()
        assert len(item) == k
        key = '.'.join(item[:-1])
        indexed = {}
        seq = set(get(indexed, item))
        for item in g:
            seq.add(get(indexed, item))
        res[key] = [i+fold(indexed.get(i, [])) for i in sorted(seq)]
    return res

def get(indexed, item):
    item = item[-1]
    if item.endswith(']'):
        item, idx = item[:-1].split('[')
        indexed.setdefault(item, []).append(int(idx))
    return item
