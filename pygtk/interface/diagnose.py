import gtk
import gtk.glade
import ascend
from itertools import groupby
from operator import itemgetter
import math
import re

ZOOM_RE = re.compile(r"([0-9]+)\s*%?")
MAX_ZOOM_SIZE = 2000
MAX_ZOOM_RATIO = 16

class DiagnoseWindow:
	def __init__(self,GLADE_FILE,browser):
		self.browser=browser
		_xml = gtk.glade.XML(GLADE_FILE,"diagnosewin")
		_xml.signal_autoconnect(self)	

		self.window = _xml.get_widget("diagnosewin")
		self.imagescroll = _xml.get_widget("imagescroll")
		self.image = _xml.get_widget("image")
		self.blockentry = _xml.get_widget("blockentry")
		self.zoomentry = _xml.get_widget("zoomentry")

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

		self.zoom=1;
	
	def fill_values(self, block):
		print "FILL VALUES for block %d" % block
		try:
			rl,cl,rh,ch = self.im.getBlockLocation(block)
		except IndexError:
			print "invalid block"
			return
		self.block = block
		self.blockentry.set_text(str(block))

		nr = int(rh-rl+1);
		nc = int(ch-cl+1);

		#print "STARTING IMAGE CREATION"
		# refer http://pygtk.org/pygtk2tutorial/sec-DrawingMethods.html
		c = chr(255)
		b = nr*nc*3*[c]
		rowstride = 3 * nc
		
		blackdot = [chr(0)]*3;
		reddot = [chr(255), chr(0), chr(0)]
		pinkdot = [chr(255), chr(127), chr(127)]
		skydot = [chr(127), chr(127), chr(255)]
		bluedot = [chr(0), chr(0), chr(255)]
		
		for i in self.data:
			if i.row < rl or i.row > rh or i.col < cl or i.col > ch:
				continue
			r = i.row - rl;
			c = i.col - cl;
			pos = rowstride*r + 3*c
			dot = blackdot;
			var = self.im.getVariable(i.col);
			rat = var.getValue() / var.getNominal()
			if rat!=0:
				try:
					#print "SCALE i.col =",rat
					val = math.log(abs(rat));
					#print "LOG i.col =",val
					if val > 1:
						dot = reddot;
					elif var < -1:
						dot = bluedot;
					elif var > 0:
						dot = pinkdot;
					elif var < 0:
						dot = skydot;
				except ValueError, e:
					pass
			#print "DOT: ",dot
			b[pos], b[pos+1], b[pos+2] = dot

		d = ''.join(b)

		#print "DONE IMAGE CREATION"
	
		self.pixbuf = gtk.gdk.pixbuf_new_from_data(d, gtk.gdk.COLORSPACE_RGB, False, 8 \
				, nc, nr, rowstride);

		self.nr = nr
		self.nc = nc
		self.zoom = -1 # to fit, up to max 16x
		self.do_zoom()

		#print "DONE IMAGE TRANSFER TO SERVER"

		self.fill_var_names()
		self.fill_rel_names()

	def do_zoom(self):
		if self.zoom == -1:
			w, h = self.imagescroll.size_request()
			#print "SCALE TO FIX, w=%d, h=%d" % (w,h)
			if self.nc/self.nr > w/h:
				# a 'wide' image	
				self.zoom = w / self.nc
			else:
				self.zoom = h / self.nr

		if self.zoom > MAX_ZOOM_RATIO:
			self.zoom = MAX_ZOOM_RATIO

		if self.zoom * self.nc > MAX_ZOOM_SIZE or self.zoom * self.nr > MAX_ZOOM_SIZE:
			self.zoom = MAX_ZOOM_SIZE / max(self.nc,self.nr)

		w = int(self.zoom * self.nc);
		h = int(self.zoom * self.nr);
			
		self.zoomentry.set_text("%d %%" % (int(self.zoom*100)) )

		if self.zoom < 2:
			pb1 = self.pixbuf.scale_simple(w,h,gtk.gdk.INTERP_BILINEAR)
		else:
			pb1 = self.pixbuf.scale_simple(w,h,gtk.gdk.INTERP_NEAREST)
		
		self.image.set_from_pixbuf(pb1)

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

	def set_zoom(self,zoom):
		self.zoom = zoom
		self.do_zoom()

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

	def on_blockentry_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		print "KEY ",keyname
		if keyname=="Return":
			self.set_block( int(self.blockentry.get_text()) )

	def on_zoominbutton_clicked(self,*args):
		z = int( math.log(self.zoom)/math.log(2) )
		z = pow(2,z + 1);
		self.set_zoom(z)

	def on_zoomoutbutton_clicked(self,*args):
		z = int( math.log(self.zoom)/math.log(2) + 0.999)
		z = pow(2,z - 1);
		self.set_zoom(z)		

	def on_zoomentry_key_press_event(self,widget,event):
		keyname = gtk.gdk.keyval_name(event.keyval)
		print "KEY ",keyname
		if keyname=="Return":
			t = self.zoomentry.get_text()
			m = ZOOM_RE.match(t)
			if not m:
				self.zoomentry.set_text("%d %%" % int(self.zoom*100))
			for mm in m:
				print m
			self.set_zoom( int(self.zoomentry.get_text()) )

# The following is from 
# http://www.experts-exchange.com/Programming/Programming_Languages/Python/Q_21719649.html
# it's still buggy.

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
