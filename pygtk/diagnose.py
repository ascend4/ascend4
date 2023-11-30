import numpy as np
from itertools import groupby
from operator import itemgetter
import math

from gi.repository import GdkPixbuf, Gdk

import config
from infodialog import *
from preferences import *

ZOOM_RE = re.compile(r"([0-9]+)\s*%?")
MAX_ZOOM_SIZE = float(2000) # float
MAX_ZOOM_RATIO = float(32) # float
AT_BOUND_TOL = 0.0001

class DiagnoseWindow:
	def __init__(self,browser,block=0):
		self.browser=browser
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["diagnosewin"])
		self.browser.builder.connect_signals(self)
		self.window = self.browser.builder.get_object("diagnosewin")
		self.window.grab_focus()
		self.window.set_transient_for(self.browser.window)

		self.prefs = Preferences()

		try:
			_icon = Gtk.Image()
			_iconpath = os.path.join(browser.assets_dir, 'diagnose' + config.ICON_EXTENSION)
			#print(("ICON PATH =",_iconpath))
			_icon.set_from_file(_iconpath)
			#print(("ICON = ",_icon))
			self.window.set_icon(_icon.get_pixbuf())
		except:
			pass
		
		self.blockstatus = self.browser.builder.get_object("blockstatustext")

		self.imagescroll = self.browser.builder.get_object("imagescroll")
		self.image = self.browser.builder.get_object("image")
		self.blockentry = self.browser.builder.get_object("blockentry")
		self.zoomentry = self.browser.builder.get_object("zoomentry")

		self.var = None; self.rel = None
		self.varname = self.browser.builder.get_object("varname1")
		self.varval = self.browser.builder.get_object("varval")
		self.varinfobutton = self.browser.builder.get_object("varinfobutton")
		self.relname = self.browser.builder.get_object("relname1")
		self.relresid = self.browser.builder.get_object("relresid")
		self.relinfobutton = self.browser.builder.get_object("relinfobutton")
		self.preferred_units_check = self.browser.builder.get_object("preferred_units_check")
		if self.prefs.getBoolPref("Diagnose","show_preferred_units")==True:
			self.preferred_units_check.set_active(True)
		else:
			self.preferred_units_check.set_active(False)

		self.varview = self.browser.builder.get_object("varview")
		self.varbuf = Gtk.TextBuffer()
		self.varview.set_buffer(self.varbuf)
		self.varcollapsed = self.browser.builder.get_object("varcollapsed")
		self.relview = self.browser.builder.get_object("relview")	
		self.relcollapsed = self.browser.builder.get_object("relcollapsed")
		self.relvalues = self.browser.builder.get_object("relvalues")
		self.rellabels = self.browser.builder.get_object("rellabels")
		self.relrels = self.browser.builder.get_object("relrels")
		self.relresids = self.browser.builder.get_object("relresids")
		self.relbuf = Gtk.TextBuffer()
		self.relview.set_buffer(self.relbuf)
		self.im = None
		self.block = 0
		self.apply_prefs()

		self.prepare_data()
		self.fill_values(block) # block zero

	def run(self):
		self.window.run()
		self.window.hide()

	def apply_prefs(self):
		vc = self.browser.prefs.getBoolPref("Diagnose","varcollapsed",True)

		#print(("VARCOLLAPSED =",vc))
		self.varcollapsed.set_active(vc)
		self.relcollapsed.set_active(self.browser.prefs.getBoolPref("Diagnose","relcollapsed",True))

	def prepare_data(self):
		# convert incidence map to pylab numarray type:
		#print("PREPARING DATA to be loaded")
		self.im = self.browser.sim.getIncidenceMatrix()
		self.data = self.im.getIncidenceData()
		#print("DATA LOADED")

		self.zoom=1;
	
	def fill_values(self, block):
		
		try:
			if self.im.getNumBlocks()==0:
				#print("NO BLOCKS!")
				self.image.set_from_stock(Gtk.STOCK_DIALOG_ERROR
					,Gtk.IconSize.DIALOG
				)
				self.browser.reporter.reportError(
					"Can't 'Diagnose blocks' until solver has been used."
				)
				return;
			rl,cl,rh,ch = self.im.getBlockLocation(block)
		except IndexError:
			if block >= self.im.getNumBlocks():
				block = self.im.getNumBlocks() - 1
				rl,cl,rh,ch = self.im.getBlockLocation(block)
			else:				
				print(("BLOCK INDEX ERROR: block =",block))
				self.blockentry.set_text(str(self.block))
				return
		except RuntimeError as e:
			print(("ERROR GETTING BLOCK LOCATION:",str(e)))
			self.blockentry.set_text(str(self.block))
			return

		self.block = block
		self.blockentry.set_text(str(block))

		self.rl = rl
		self.cl = cl
		self.rh = rh
		self.ch = ch

		nr = int(rh-rl+1);
		nc = int(ch-cl+1);

		nr = int(rh - rl + 1)
		nc = int(ch - cl + 1)

		#print("STARTING IMAGE CREATION")

		# Create an empty numpy array for RGB image
		image_array = 255 * np.ones((nr, nc, 3), dtype=np.uint8)

		# Define colors as NumPy arrays
		blackdot = np.array([0, 0, 0], dtype=np.uint8)
		reddot = np.array([255, 0, 0], dtype=np.uint8)
		pinkdot = np.array([255,127,127], dtype=np.uint8)
		skydot = np.array([127,127,255], dtype=np.uint8)
		bluedot = np.array([0,0,255], dtype=np.uint8)
		hotpinkdot = np.array([255,47,179], dtype=np.uint8) # very big (+/-)
		brightbluedot = np.array([71,157,255], dtype=np.uint8) # very small (+/-)
		greendot = np.array([87,193,70], dtype=np.uint8) # close to 1
		orangedot = np.array([255,207,61], dtype=np.uint8) # 10-1000
		bluegreendot = np.array([70,221,181], dtype=np.uint8) # 0.001 - 0.1		

		for i in self.data:
			if rl <= i.row <= rh and cl <= i.col <= ch:
				r = i.row - rl
				c = i.col - cl

				dot = blackdot
				var = self.im.getVariable(i.col)

				if abs( (var.getValue()-var.getUpperBound())/ var.getNominal() )  < AT_BOUND_TOL:
					dot = reddot
				elif abs( var.getValue() - var.getLowerBound() ) / var.getNominal() < AT_BOUND_TOL:
					dot = reddot
				else:
					rat = var.getValue() / var.getNominal()
					if rat!=0:
						try:
							val = abs(rat)
							if abs(rat) > 1000:
								dot = hotpinkdot
							elif abs(rat) > 10:
								dot = orangedot
							elif abs(rat) < 0.001:
								dot = brightbluedot
							elif abs(rat) < 10 and abs(rat) > 0.1:
								dot = greendot
							elif abs(rat) > 0.001 and abs(rat) < 0.1:
								dot = bluegreendot
							else:
								dot = blackdot
						except ValueError as e:
							pass

				# Set the pixel color in the NumPy array
				image_array[r, c, :] = dot[:]

		#print("DONE IMAGE CREATION")

		self.pixbuf = GdkPixbuf.Pixbuf.new_from_data(
			image_array.tobytes()
			,GdkPixbuf.Colorspace.RGB
			,False # no alpha
			,8 # bits per channel
			,nc # width
			,nr # height
			,nc*3 # rowstride
		)

		self.nr = nr
		self.nc = nc
		self.zoom = -1 # to fit, up to max 16x
		self.do_zoom()

		#print("DONE IMAGE TRANSFER TO SERVER")

		self.fill_var_names()
		self.fill_rel_names()
		self.fill_block_status()

		self.fill_selection_info()

		#print("DONE FILL VALUES")

	def fill_selection_info(self):
		if self.var:
			self.varname.set_text(self.var.getName())
			default_units = self.var.getInstance().getType().getDimensions().getDefaultUnits().getName().toString()
			pref_units = self.var.getInstance().getType().getPreferredUnits()
			if pref_units and self.prefs.getBoolPref("Diagnose","show_preferred_units",True):
				varval = str(self.var.getValue())+" "+pref_units.getName().toString()
			else:
				if default_units=="?":
					varval = str(self.var.getValue())
				else:
					varval = str(self.var.getValue())+" "+default_units
			self.varval.set_text(varval)
			self.varinfobutton.set_sensitive(True)
		else:
			self.varname.set_text("")
			self.varval.set_text("")
			self.varinfobutton.set_sensitive(False)

		if self.rel:
			self.relname.set_text(self.rel.getName())
			self.relresid.set_text(str(self.rel.getResidual()))
			self.relinfobutton.set_sensitive(True)
		else:
			self.relname.set_text("")
			self.relresid.set_text("")
			self.relinfobutton.set_sensitive(False)

	def do_zoom(self):
		if self.zoom == -1:
			w, h = self.imagescroll.get_size_request()
			#print "SCALE TO FIX, w=%d, h=%d" % (w,h)
			if self.nc/self.nr > w/h:
				# a 'wide' image	
				self.zoom = float(w) / self.nc
			else:
				self.zoom = float(h) / self.nr

		#self.browser.reporter.reportNote("Diagnose window: preliminary calculated zoom = %f (nr = %d, nc = %d)" % (self.zoom, self.nr, self.nc))
		

		if self.zoom > MAX_ZOOM_RATIO:
			self.zoom = MAX_ZOOM_RATIO

		if self.zoom * self.nc > MAX_ZOOM_SIZE or self.zoom * self.nr > MAX_ZOOM_SIZE:
			self.browser.reporter.reportNote("image is too big, reducing to MAX_ZOOM_SIZE = %f" % MAX_ZOOM_SIZE);
			self.zoom = MAX_ZOOM_SIZE / max(self.nc,self.nr)

		#self.browser.reporter.reportNote("Diagnose window: matrix zoom = %f" % self.zoom)
		w = int(self.zoom * self.nc)
		h = int(self.zoom * self.nr)
			
		self.zoomentry.set_text("%d %%" % (int(self.zoom*100)) )

		if self.zoom < 2:
			pb1 = self.pixbuf.scale_simple(w,h,GdkPixbuf.InterpType.BILINEAR)
		else:
			pb1 = self.pixbuf.scale_simple(w,h,GdkPixbuf.InterpType.NEAREST)

		self.image.set_from_pixbuf(pb1)

	def fill_block_status(self):
		#print("FILL BLOCK STATUS")
		s = self.im.getBlockStatus(self.block)
		ss = "Failed"
		if s == ascpy.IM_CONVERGED:
			ss = "Converged"
		elif s == ascpy.IM_NOT_YET_ATTEMPTED:
			ss = "Not attempted yet"
		elif s == ascpy.IM_OVER_TIME:
			ss += " (time limit)"
		elif s == ascpy.IM_OVER_ITER:
			ss += " (iter limit)"
		self.blockstatus.set_text(ss);
		

	def fill_var_names(self):
		#print("FILL VAR NAMES")

		names = [str(i) for i in self.im.getBlockVars(self.block)]

		#print "NAMES:",names

		if self.varcollapsed.get_active():
			res = reduce(names)
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

		#print("DONE VAR NAMES")

	def fill_rel_names(self):
		#print("REL NAMES")

		rels = self.im.getBlockRels(self.block)

		#print("GOT RELS, NOW GETTING NAMES")

		names = [str(i) for i in rels]

		#print "NAMES =",names

		if self.relcollapsed.get_active():
			res = reduce(names)
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
		self.relbuf.set_text(text)

		#print("DONE REL NAMES")

	def set_block(self, block):
		self.fill_values(block)

	def set_zoom(self,zoom):
		self.zoom = zoom
		self.do_zoom()

	def show_cursor(self,x,y):
		c = self.cl + int(x/self.zoom)
		r = self.rl + int(y / self.zoom)
		if c > self.ch or r > self.rh:
			#print "OUT OF RANGE"
			return
		self.var = self.im.getVariable(c)
		self.rel = self.im.getRelation(r)
		self.fill_selection_info()

	# GUI EVENT HOOKS-----------------------------------------------------------

	def on_diagnosewin_close(self,*args):
		self.window.response(Gtk.ResponseType.CLOSE);

	def on_preferred_units_toggle(self,widget):
		_v = widget.get_active()
		self.prefs.setBoolPref("Diagnose","show_preferred_units",_v)
		self.fill_selection_info()

	# incidence data view

	def on_varcollapsed_toggled(self,*args):
		vc = self.varcollapsed.get_active()
		self.browser.prefs.setBoolPref("Diagnose","varcollapsed",vc)	
		if self.im:
			self.fill_var_names()

	def on_relcollapsed_toggled(self,*args):
		rc = self.varcollapsed.get_active()
		self.browser.prefs.setBoolPref("Diagnose","relcollapsed",rc)	
		if self.im:
			self.fill_rel_names()

	# detailed information about vars and rels (solver-side information!)

	def on_varinfobutton_clicked(self,*args):
		title = "Variable '%s'" % self.var
		text = "%s\n%s\n" % (title,"(from the solver's view)")
		units = " "
		default_units = self.var.getInstance().getType().getDimensions().getDefaultUnits().getName().toString()
		pref_units = self.var.getInstance().getType().getPreferredUnits()
		if pref_units and self.prefs.getBoolPref("Diagnose","show_preferred_units",True):
			units += pref_units.getName().toString()
		else:
			if default_units!="?":
				units += default_units
		_rows = {
			"Value": self.var.getValue()
			,"Nominal": self.var.getNominal()
			,"Lower bound": self.var.getLowerBound()
			,"Upper bound": self.var.getUpperBound()
		}
		for k,v in list(_rows.items()):
			text += "\n  %s\t%s" % (k,value_human(v)+units)
		
		text += "\n\nIncident with %d relations:" % self.var.getNumIncidentRelations()
		for r in self.var.getIncidentRelations():
			text += "\n  %s" % r.getName()

		_dialog = InfoDialog(self.browser,self.window,text,title,tabs=(150,300))
		_dialog.run()

	def on_relinfobutton_clicked(self,*args):
		title = "Relation '%s'" % self.rel
		text = "%s\n%s\n" % (title,"(from the solver's view)")
		text += "\n  %s\t%15f" % ("Residual", self.rel.getResidual())

		text += "\n\nRelation expression:\n"
		text += self.rel.getRelationAsString()

		text += "\n\nIncident with %d variables:" % self.rel.getNumIncidentVariables()
		for v in self.rel.getIncidentVariables():
			units = " "
			default_units = v.getInstance().getType().getDimensions().getDefaultUnits().getName().toString()
			pref_units = v.getInstance().getType().getPreferredUnits()
			if pref_units and self.prefs.getBoolPref("Diagnose","show_preferred_units",True):
				units += pref_units.getName().toString()
			else:
				if default_units != "?" :
					units += default_units
			text += "\n  %s\t= %s" % ( v.getName(),value_human(v.getValue())+units )

		_dialog = InfoDialog(self.browser,self.window,text,title,tabs=(150,300))
		_dialog.run()
		

	# block navigation

	def on_nextbutton_clicked(self,*args):
		self.set_block(self.block + 1)

	def on_prevbutton_clicked(self,*args):
		self.set_block(self.block - 1)	

	def on_prevbigbutton_clicked(self,*args):
		b = self.block - 1
		while b >= 0:
			rl,cl,rh,ch = self.im.getBlockLocation(b)
			if rh-rl > 0 or ch-cl>0:
				self.set_block(b)
				return
			b = b - 1
		#print("NO PRECEDING 'BIG' BLOCKS")
		
	def on_nextbigbutton_clicked(self,*args):
		b = self.block + 1
		n = self.im.getNumBlocks()
		while b < n:
			rl,cl,rh,ch = self.im.getBlockLocation(b)
			if rh-rl > 0 or ch-cl>0:
				self.set_block(b)
				return
			b = b + 1
		#print("NO FOLLOWING 'BIG' BLOCKS")
	
	def on_blockentry_key_press_event(self,widget,event):
		keyname = Gdk.keyval_name(event.keyval)
		#print(("KEY ",keyname))
		if keyname=="Return":
			self.set_block( int(self.blockentry.get_text()) )

	# zoom in and out

	def on_zoominbutton_clicked(self,*args):
		z = int( math.log(self.zoom)/math.log(2) )
		z = pow(2,z + 1);
		self.set_zoom(z)

	def on_zoomoutbutton_clicked(self,*args):
		z = int( math.log(self.zoom)/math.log(2) + 0.999)
		z = pow(2,z - 1);
		self.set_zoom(z)		

	def on_zoomentry_key_press_event(self,widget,event):
		keyname = Gdk.keyval_name(event.keyval)
		#print(("KEY ",keyname))
		if keyname=="Return":
			t = self.zoomentry.get_text()
			m = ZOOM_RE.match(t)
			if not m:
				self.zoomentry.set_text("%d %%" % int(self.zoom*100))
			for mm in m:
				print(m)
			self.set_zoom( int(self.zoomentry.get_text()) )

	# clicking in incidence matrix to get updated information at RHS

	def on_imageevent_motion_notify_event(self,widget,event):
		self.show_cursor(event.x, event.y)

	def on_imageevent_button_press_event(self,widget,event):
		self.show_cursor(event.x, event.y)


def value_human(v):
	if v==0 or abs( math.log10(abs(v)) )<8:
		return "%f" % v	
	return "%e" % v

#---------------------------------------
# Procedures to 'fold' a list of items from a hierarchy
# http://www.experts-exchange.com/Programming/Programming_Languages/Python/Q_21719649.html
# It's still buggy, I think

def fold(data):
    """ fold sorted numeric sequence data into ranged representation:
    >>> fold([1,  4,5,6, 10, 15,16,17,18, 22, 25,26,27,28])
    '[1,4-6,10,15-18,22,25-28]'
    """
    folded = []
    for k, g in groupby(enumerate(sorted(data)), lambda i_x:i_x[0]-i_x[1]):
        seq = list(map(itemgetter(1), g))
        if len(seq) > 1:
            x = '%s-%s' % (seq[0], seq[-1])
        else:
            x = str(seq[0])
        folded.append(x)
    return folded and '[%s]' % ','.join(folded) or ''

def reduce(names):
    """reduce a list of items nto something more readable:
    >>> data = 'C.x C.p C.T C.delta[1] C.delta[2] C.delta[3] C.sat.x C.sat.p C.h C.delta[5]'.split()
    >>> res = reduce(data)
    >>> for k in sorted(res):
    ...   print '%s: %s' % (k, res[k])
    C: T, delta[1-3,5], h, p, x
    C.sat: p, x
    """
    data = sorted([n.split('.') for n in sorted(names)], key=len)
    res = {}
    for k, g in groupby(data, lambda x: len(x)):
        if k == 1:
            indexed = {}
            seq = set([get(indexed, item) for item in g])
            res['[global]'] = [ i+fold(indexed.get(i, [])) for i in sorted(seq) ]
        else:
            for key, g1 in groupby(g, lambda x: '.'.join(x[:-1])):
                indexed = {}
                seq = set(get(indexed, item) for item in g1)
                res[key] = [ i+fold(indexed.get(i, [])) for i in sorted(seq) ]
    return res

def get(indexed, item):
    item = item[-1]
    if item.endswith(']'):
        item, idx = item[:-1].split('[')
        indexed.setdefault(item, []).append(int(idx))
    return item
