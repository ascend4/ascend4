#!/usr/bin/env python

from __future__ import with_statement
import os, sys

os.chdir(os.path.abspath(os.path.dirname(sys.argv[0])))

os.environ['ASCENDLIBRARY'] = "../../models"
os.environ['ASCENDSOLVERS'] = "../../solvers/qrslv"

if sys.platform.startswith("win"):
	os.environ['PATH'] += ";..\.."
else:
	os.environ['LD_LIBRARY_PATH'] = "../.."
	
sys.path.append("..")
sys.path.append("../../ascxx")

#import gtkexcepthook

if sys.platform.startswith("win"):
    # Fetchs gtk2 path from registry
	import _winreg
	import msvcrt
	try:
		k = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "Software\\GTK\\2.0")
	except EnvironmentError:
		# use TkInter to report the error :-)
		from TkInter import *
		root = Tk()
		w = Label(root,"You must install the Gtk+ 2.2 Runtime Environment to run this program")
		w.pack()
		root.mainloop()
		sys.exit(1)
	else:
		gtkdir = _winreg.QueryValueEx(k, "Path")
        import os
        # we must make sure the gtk2 path is the first thing in the path
        # otherwise, we can get errors if the system finds other libs with
        # the same name in the path...
        os.environ['PATH'] = "%s/lib;%s/bin;" % (gtkdir[0], gtkdir[0]) + os.environ['PATH']

import ascpy

L = ascpy.Library()

# FIXME need to add more a concrete way to add/remove modules from the Library?

L.load('test/canvas/blocktypes.a4c')

D = L.getAnnotationDatabase()

M = L.getModules()

blocktypes = set()

for m in M:
	T = L.getModuleTypes(m)
	for t in T:
		# 'block' types must not be parametric, because they must be able to
		# exist even without being connected, and parametric models impose
		# restrictions on the use of ARE_THE_SAME and similar.
		if t.hasParameters():
			continue
		x = D.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
		if x:
			blocktypes.add(t)

blocks = []

from blocktype import *
print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:

	b = BlockType(t,D)

	blocks += [b]

# render icon table
import threading
import gtk
import os, os.path, re

import cairo

class BlockIconView(gtk.IconView):
	"""
	IconView containing the palette of BlockTypes available for use in the
	canvas. The list of blocks is supplied currently as an initialisation
	parameter, but it is intended that this would be dynamic in a final system.

	It should be possible drag icons from the palette into the canvas, but
	that is not yet implemented.
	"""
	def __init__(self,blocks,app):
		# the mode containing the icons themselves...
		self.model = gtk.ListStore(str, gtk.gdk.Pixbuf)
		self.app = app
		self.otank = {}
		n = 0
		#with thread:
		for b in blocks:
			n += 1
			pixbuf = b.get_icon(48,48)
			iter = self.model.append([b.type.getName(), pixbuf])
			path = self.model.get_path(iter)
			self.otank[path] = b

		gtk.IconView.__init__(self)
		self.set_model(self.model)
		self.set_text_column(0)
		self.set_pixbuf_column(1)
		self.set_columns(-1)
		self.set_size_request(180,100)
		self.connect("item-activated", self.item_activated)
		self.connect("selection-changed", self.selection_changed)

	def selection_changed(self,iconview):
		s = self.get_selected_items()
		if len(s)==1:
			b = self.otank[s[0]]
			self.app.set_placement_tool(b)
		
		
	def item_activated(self,iconview, path):
		self.app.set_placement_tool(self.otank[path])
		
from gaphas import GtkView, View
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
#from gaphas.tool import LineSegmentTool
from gaphas.tool import Tool, ItemTool, RubberbandTool
from gaphas.painter import ItemPainter
from blockconnecttool import BlockConnectTool
from blockline import *
from blockitem import *
from contextmenutool import *
from connectortool import *
from blockcanvas import *
#from panzoom import ZoomTool
#from panzoom import PanTool
from gaphas.tool import PanTool, ZoomTool
from blockinstance import *
from solverreporterforcanvas import *
import info, blockproperties

def BlockToolChain():
	"""
	ToolChain for working with BlockCanvas, including several custom Tools.
	"""
	chain = ToolChain()
	chain.append(HoverTool())
	chain.append(BlockConnectTool()) # for connect/disconnect of lines
	chain.append(ConnectorTool()) # for creating new lines by drag from Port
	chain.append(ContextMenuTool()) # right-click
#	chain.append(LineSegmentTool()) # for moving line 'elbows'
	chain.append(ItemTool())
	chain.append(PanTool())
	chain.append(ZoomTool())
	chain.append(RubberbandTool())
	return chain

class app(gtk.Window):
	def __init__(self):
		"""
		Initialise the application -- create all the widgets, and populate
		the icon palette.
		TODO: separate the icon palette into a separate method.
		"""
		self.errorvars = []
		self.errorblocks = []
		self.status = gtk.Statusbar()
		self.temp = []
		self.act = 1 #to be used for storing canvas zoom in/out related data.
		# the Gaphas canvas
		canvas = BlockCanvas()

		# the main window
		gtk.Window.__init__(self)
		self.iconok = self.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconinfo = self.render_icon(gtk.STOCK_DIALOG_INFO,gtk.ICON_SIZE_MENU)
		self.iconwarning = self.render_icon(gtk.STOCK_DIALOG_WARNING,gtk.ICON_SIZE_MENU)
		self.iconerror = self.render_icon(gtk.STOCK_DIALOG_ERROR,gtk.ICON_SIZE_MENU)

		self.set_title("ASCEND Blocks")
		self.set_default_size(650,650)
		self.connect("destroy", gtk.main_quit)
		self.connect("key-press-event", self.key_press_event)

		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
		self.set_icon(windowicon.get_pixbuf())

		# vbox containing the main view and the status bar at the bottom
		vbox = gtk.VBox()
		
		#--------------------------------------------------------------------------------------------------

		mb = gtk.MenuBar()
		
         	#menu items for File menu
		
		fmenu = gtk.Menu()
		agr = gtk.AccelGroup()
		self.add_accel_group(agr)
		
		new = gtk.ImageMenuItem(gtk.STOCK_NEW, agr)
		new.connect("activate", self.new)
		fmenu.append(new)
		new.show()

		opn = gtk.ImageMenuItem(gtk.STOCK_OPEN, agr)
		opn.connect("activate", self.fileopen)
		fmenu.append(opn)
		opn.show()
		
		save = gtk.ImageMenuItem(gtk.STOCK_SAVE, agr)
		save.connect("activate", self.save_canvas)
		fmenu.append(save)
		save.show()
		
		saveas = gtk.ImageMenuItem(gtk.STOCK_SAVE_AS, agr)
		saveas.connect("activate", self.filesave)
		fmenu.append(saveas)
		saveas.show()
		
		recent = gtk.ImageMenuItem('Recent		')
		recent.connect("activate", self.dummy)
		recent.set_sensitive(False)
		fmenu.append(recent)
		recent.show()
		
		export = gtk.ImageMenuItem(gtk.STOCK_CONVERT)
		export.get_children()[0].set_label('Export Canvas As...	')
		export.connect("activate",self.export_svg_as)
		fmenu.append(export)
		export.show()
		
		libraries = gtk.Menu()
		
		load_lib = gtk.MenuItem('Load Library	')
		load_lib.set_submenu(libraries)
		fmenu.append(load_lib)
		load_lib.show()
		
		loadlib1 = gtk.MenuItem('Default :: blocktypes.a4c')
		loadlib1.connect("activate",self.loadlib,'test/canvas/blocktypes.a4c')
		libraries.append(loadlib1)
		loadlib1.show() 
		 
		loadlib2 = gtk.MenuItem('Basic Electronics [DC] Library')
		loadlib2.connect("activate",self.loadlib, 'test/canvas/basic_electronics_model.a4c')
		libraries.append(loadlib2)
		loadlib2.show() 

		loadlib3 = gtk.MenuItem('Rankine For Canvas')
		loadlib3.connect("activate",self.loadlib, 'test/canvas/rankine_for_canvas.a4c')
		libraries.append(loadlib3)
		loadlib3.show()
		
		
		fmenu.add(gtk.SeparatorMenuItem())
		
		qit = gtk.ImageMenuItem(gtk.STOCK_QUIT, agr)
		qit.connect("activate", gtk.main_quit)
		fmenu.append(qit)
		qit.show()
		
		file_menu = gtk.MenuItem("_File")
		file_menu.show()
		file_menu.set_submenu(fmenu)
		
		#menu items for Edit
		
		emenu = gtk.Menu()
		
		undo = gtk.ImageMenuItem(gtk.STOCK_UNDO, agr)
		undo.connect("activate", self.undo_canvas)
		undo.set_sensitive(False)
		emenu.append(undo)
		undo.show()
		
		redo = gtk.ImageMenuItem(gtk.STOCK_REDO, agr)
		redo.connect("activate", self.dummy)
		redo.set_sensitive(False)
		emenu.append(redo)
		redo.show()
		
		copy = gtk.ImageMenuItem(gtk.STOCK_COPY, agr)
		copy.connect("activate", self.dummy)
		copy.set_sensitive(False)
		emenu.append(copy)
		copy.show()
		
		paste = gtk.ImageMenuItem(gtk.STOCK_PASTE, agr)
		paste.connect("activate", self.dummy)
		paste.set_sensitive(False)
		emenu.append(paste)
		paste.show()
		
		delete = gtk.ImageMenuItem(gtk.STOCK_DELETE)
		delete.connect('activate', self.delblock)
		emenu.append(delete)
		delete.show()
		
		emenu.add(gtk.SeparatorMenuItem())
		
		selectall = gtk.MenuItem("Select All	")
		selectall.connect('activate', self.dummy)
		selectall.set_sensitive(False)
		emenu.append(selectall)
		selectall.show()
		
		deselectall = gtk.ImageMenuItem('Deselct All		')
		delete.connect('activate', self.dummy)
		deselectall.set_sensitive(False)
		emenu.append(deselectall)
		deselectall.show()
		
		emenu.add(gtk.SeparatorMenuItem())
		
		bp = gtk.MenuItem('Block Properties	')
		bp.connect("activate", self.bp)
		emenu.append(bp)
		bp.show()
		
		edit_menu = gtk.MenuItem("_Edit")
		edit_menu.show()
		edit_menu.set_submenu(emenu)
		
		#menu items for view
		
		vmenu = gtk.Menu()
		flscrn = gtk.MenuItem('Toggle FullScreen / Normal Mode	', agr)
		k, m = gtk.accelerator_parse("F11")
		flscrn.add_accelerator("activate", agr, k, m, gtk.ACCEL_VISIBLE)
		flscrn.connect("activate", self.fullscrn)
		vmenu.append(flscrn)
		flscrn.show()
		
		vmenu.add(gtk.SeparatorMenuItem())
	
		zoomin = gtk.ImageMenuItem(gtk.STOCK_ZOOM_IN, agr)
		zoomin.connect("activate", self.zoom, 1.2)
		vmenu.append(zoomin)
		zoomin.show()
		
		zoomout = gtk.ImageMenuItem(gtk.STOCK_ZOOM_OUT, agr)
		zoomout.connect("activate", self.zoom, .8)
		vmenu.append(zoomout)
		zoomout.show()
		
		ft2scrn = gtk.MenuItem('Normal Size	')
		ft2scrn.connect("activate", self.zoom, 1)
		vmenu.append(ft2scrn)
		ft2scrn.show()
		
		view_menu = gtk.MenuItem('_View')
		view_menu.show()
		view_menu.set_submenu(vmenu)
		
		#menu items for Tools
		
		tmenu = gtk.Menu()
	
		dbg = gtk.ImageMenuItem('Debug	', agr)
		dbg.connect("activate",self.debug_canvas)
		tmenu.append(dbg)
		dbg.show()

		slv = gtk.ImageMenuItem(gtk.STOCK_EXECUTE, agr)
		slv.connect("activate", self.run_canvas)
		tmenu.append(slv)
		slv.show()

		prv = gtk.ImageMenuItem(gtk.STOCK_PRINT_PREVIEW)
		prv.get_children()[0].set_label('Preview')
		prv.connect("activate", self.preview_canvas)
		tmenu.append(prv)
		prv.show()
		
		tools_menu = gtk.MenuItem('_Tools')
		tools_menu.show()
		tools_menu.set_submenu(tmenu)
		
		#menu items for Help
		
		hmenu = gtk.Menu()
		
		hlp = gtk.ImageMenuItem(gtk.STOCK_HELP)
		hlp.get_children()[0].set_label('Contents')
		k, m = gtk.accelerator_parse("F1")
		hlp.add_accelerator("activate", agr, k, m, gtk.ACCEL_VISIBLE)
		hlp.connect("activate", self.on_contents_click)
		hmenu.append(hlp)
		hlp.show()
		
		hlpo = gtk.MenuItem('Get Help Online...')
		hlpo.connect("activate", self.on_get_help_online_click)
		hmenu.append(hlpo)
		hlpo.show()
		
		hmenu.add(gtk.SeparatorMenuItem())
		
		rprtbug = gtk.MenuItem('Report Bug...')
		rprtbug.connect("activate", self.on_report_a_bug_click)
		hmenu.append(rprtbug)
		rprtbug.show()
		
		
		abt = gtk.ImageMenuItem(gtk.STOCK_ABOUT, agr)
		abt.connect("activate", self.about)
		hmenu.append(abt)
		abt.show()
		
		help_menu = gtk.MenuItem("Help")
		help_menu.show()
		help_menu.set_submenu(hmenu)
		
		vbox.pack_start(mb, False, False)
		mb.show()
		mb.append(file_menu)
		mb.append(edit_menu)
		mb.append(view_menu)
		mb.append(tools_menu)
		mb.append(help_menu)
		
		#-------------------------------------------------------------------------------------
		
		tb = gtk.Toolbar()	
		loadbutton = gtk.ToolButton(gtk.STOCK_OPEN)
		loadbutton.connect("clicked",self.fileopen)
		tb.insert(loadbutton,0)
		savebutton = gtk.ToolButton(gtk.STOCK_SAVE)
		savebutton.connect("clicked",self.save_canvas)
		tb.insert(savebutton,1)
		debugbutton = gtk.ToolButton(gtk.STOCK_PROPERTIES)
		debugbutton.set_label("Debug")
		debugbutton.connect("clicked",self.debug_canvas)
		tb.insert(debugbutton,2)
		previewb = gtk.ToolButton(gtk.STOCK_PRINT_PREVIEW)
		previewb.set_label("Preview")
		previewb.connect("clicked",self.preview_canvas)
		tb.insert(previewb,3)
		exportbutton = gtk.ToolButton(gtk.STOCK_CONVERT)
		exportbutton.set_label("Export SVG")
		exportbutton.connect("clicked",self.export_svg)
		tb.insert(exportbutton,2)
		runb = gtk.ToolButton(gtk.STOCK_EXECUTE)
		runb.set_label("Run")
		runb.connect("clicked",self.run_canvas)
		runb = gtk.ToolButton(gtk.STOCK_EXECUTE)
		runb.set_label("Run")
		runb.connect("clicked",self.run_canvas)
		tb.insert(runb, 4)
		m_entry = gtk.ToolButton(gtk.STOCK_SAVE_AS)
		m_entry.set_label("Custom METHOD")
		m_entry.connect("clicked",self. custommethod)
		tb.insert(m_entry,5)
		vbox.pack_start(tb, False, False)

		# hbox occupies top part of vbox, with icons on left & canvas on right.
		paned = gtk.HPaned()
		
		# the 'view' widget implemented by Gaphas
		import gaphas.view
		#gaphas.view.DEBUG_DRAW_BOUNDING_BOX = True
		self.view = GtkView()	
		self.view.tool =  BlockToolChain()

		# table containing scrollbars and main canvas 
		t = gtk.Table(2,2)
		self.view.canvas = canvas
		self.view.zoom(1)
		self.view.set_size_request(600, 450)
		hs = gtk.HScrollbar(self.view.hadjustment)
		vs = gtk.VScrollbar(self.view.vadjustment)
		t.attach(self.view, 0, 1, 0, 1)
		t.attach(hs, 0, 1, 1, 2, xoptions=gtk.FILL, yoptions=gtk.FILL)
		t.attach(vs, 1, 2, 0, 1, xoptions=gtk.FILL, yoptions=gtk.FILL)

		# a scrolling window to contain the icon palette
		self.scroll = gtk.ScrolledWindow()
		self.scroll.set_border_width(2)
		self.scroll.set_shadow_type(gtk.SHADOW_ETCHED_IN)

		self.scroll.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		# icon palette
		self.blockiconview = BlockIconView(blocks, self)
		self.scroll.add(self.blockiconview)
		self.reporter = ascpy.getReporter()
			
		paned.pack1(self.scroll, True, True)
		paned.pack2(t, True, True)
		vbox.pack_start(paned, True, True)
		vpane = gtk.VPaned()
		vpane.pack1(vbox)
		lower_vbox = gtk.VBox()
		import errorreporter
		ET = errorreporter.ErrorReporter( self.reporter,self.iconok,self.iconinfo,self.iconwarning,self.iconerror)
		self.notebook = gtk.Notebook()
		self.notebook.set_tab_pos(gtk.POS_TOP)
		label = gtk.Label('Error / Status Reporter Console')
		scrolledwindow = gtk.ScrolledWindow()
		scrolledwindow.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		scrolledwindow.add(ET.errorview)
		self.notebook.append_page(scrolledwindow, label)
		lower_vbox.pack_start(self.notebook,True, True)
		lower_vbox.pack_start(self.status, False, False)
		vpane.pack2(lower_vbox)
		self.add(vpane)
		self.show_all()
				
	def set_placement_tool(self,blocktype):
		"""
		Prepare the canvas for drawing a block of the type selected in the
		icon palette.
		"""
		# TODO: add undo handler
		label = blocktype.type.getName()
		def my_block_factory():
			def wrapper():
				b = BlockInstance(blocktype)
				bi = DefaultBlockItem(b)
				self.view.canvas.add(bi)
				return bi
			return wrapper
		self.view.tool.grab(PlacementTool(self.view,my_block_factory(), HandleTool(), 2))
		self.status.push(0,"Selected '%s'..." % blocktype.type.getName())

	def set_connector_tool(self,foobar):
		"""
		Prepare the canvas for drawing a connecting line (note that one can
		alternatively just drag from a port to another port).
		"""
		
		def my_line_factory():
			def wrapper():
				l =  BlockLine()
				self.view.canvas.add(l)
				return l
			return wrapper
		self.view.tool.grab(PlacementTool(self.view,my_line_factory(), HandleTool(), 1))
		
	def key_press_event(self,widget,event):
		"""
		Handle various application-level keypresses. Fall through if keypress
		is not handled here; it might be picked up elsewhere.
		"""
		# TODO: add undo handler
		key = gtk.gdk.keyval_name(event.keyval)
		if key == 'Delete' and self.view.focused_item:
			self.delblock(widget)
			self.status.push(0,"Item deleted.")
		elif key == 'l' or key == 'L':
			self.set_connector_tool()
			self.status.push(0,"Line draw mode...")
		elif key == 'S' or key == 's':
			self.save_canvas(None)
		elif key == 'R' or key == 'r':
			self.load_canvas(None)
		elif key == 'V' or key == 'v':
			self.preview_canvas(None)
		elif key == 'X' or key == 'x':
			self.run_canvas(None)
		elif key == 'G' or key == 'g':
			self.export_svg(None)
	
	def delblock(self, widget):
		if self.view.focused_item:
			self.view.canvas.remove(self.view.focused_item)
			self.status.push(0,"Item deleted.")
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
			
	def new(self, widget):
		for item in self.view.canvas.get_all_items():
			self.view.canvas.remove(item)
		self.reporter.reportNote('Canvas cleared for new Model')
		self.view.canvas.filestate = 0
		self.view.canvas.filename = None	
		self.view.canvas.canvasmodelstate = 'Unsolved'
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)
		self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
		
	def debug_canvas(self,widget):
		"""
		Display an 'object browser' to allow inspection of the objects
		in the canvas.
		"""

		import obrowser
		b = obrowser.Browser("canvas",self.view.canvas, False)
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)

	def save_canvas(self,widget):
		"""
		Save the canvas in 'pickle' format. Currently saving is jointly handled by both self.save_canvas and self.filesave methods
		"""
		import pickle as pickle
		import gaphas.picklers
		if self.view.canvas.filestate == 0:
			self.filesave(widget)
			return
		else:	
			f = file(self.view.canvas.filename,"w")
		try:
			pickle.dump(self.view.canvas,f)
			self.status.push(0,"Canvasmodel saved...")		
		except Exception,e:
			print "ERROR:",str(e)
			self.status.push(0,"Canvasmodel could not be saved : " + str(e))
			import obrowser
			b = obrowser.Browser("canvas",self.view.canvas)
			d = gtk.Dialog("Error",self,gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
			d.vbox.add(gtk.Label(str(e)))
			d.show_all()
			d.run()
			d.hide()
		finally:
			f.close()
	
	def load_canvas(self,widget):
		"""
		Restore a saved canvas in 'pickle' format. Currently not in use as loading now handled by self.fileopen().
		"""
		import pickle as pickle
		import gaphas.picklers
		f = file("./test.a4b","r")
		try:
			self.view.canvas = pickle.load(f)
			if self.view.canvas.model_library is not None:
				print "Loading Library...."
				self.loadlib(self, self.view.canvas.model_library,0)
			self.view.canvas.reattach_ascend(L,D)
			self.view.canvas.update_now()
			self.status.push(0,"Canvasmodel sucessfully loaded...")
		except Exception,e:
			self.status.push(0,"Canvasmodel could not be loaded : " + str(e))
			self.reporter.ReportError("Canvasmodel could not be loaded : " + str(e))	
			self.reporter.ReportNote(" Error occured while attempting to 'Load' the file")
		finally:
			f.close()
		self.load_presaved_canvas(None)			
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)	
	

	def temp_canvas(self,widget):
		pass
		"""
		NOTE: 	re-implementation needed	
		"""	
	
	def undo_canvas(self,widget):
		pass
		"""
		NOTE: 	re-implementation needed	

		"""
		
	def preview_canvas(self,widget):
		"""
		Output an ASCEND representation of the canvas on the commandline.
		Under development.
		"""
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)
		info.Info(self.view.parent.parent.parent.parent.parent,str(self.view.canvas),"Canvas Preview").run()
		
	def export_svg_as(self,widget):
		
		f = None
		dialog = gtk.FileChooserDialog("Export Canvas As...", None,
                                       gtk.FILE_CHOOSER_ACTION_SAVE, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)
		filter = gtk.FileFilter()
		filter.set_name("Image File")
		filter.add_pattern("*.svg")
		dialog.add_filter(filter)
		dialog.show()
		response = dialog.run()
				
		svgview = View(self.view.canvas)
		svgview.painter = ItemPainter()

		# Update bounding boxes with a temporaly CairoContext
		# (used for stuff like calculating font metrics)
		tmpsurface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 0, 0)
		tmpcr = cairo.Context(tmpsurface)
		svgview.update_bounding_box(tmpcr)
		tmpcr.show_page()
		tmpsurface.flush()
		
		if response == gtk.RESPONSE_OK:
			name = dialog.get_filename()
			if '.svg' not in name:
				name += '.svg'
			if f == None and f != name:
				f = open(name, 'w')
			elif f != None and name == f:
				dialog.set_do_overwrite_confirmation(True)
				dialog.connect("confirm-overwrite", self.confirm_overwrite_callback)
				
			fn = name
			w, h = svgview.bounding_box.width, svgview.bounding_box.height
			surface = cairo.SVGSurface(fn , w, h)
			cr = cairo.Context(surface)
			svgview.matrix.translate(-svgview.bounding_box.x, -svgview.bounding_box.y)
			svgview.paint(cr)
			cr.show_page()
			surface.flush()
			surface.finish()
			self.reporter.reportNote(" File ' %s ' saved successfully." % name )
			self.status.push(0,"Wrote SVG file '%s'." % fn)
		dialog.destroy()
		
		
	def export_svg(self,widget):	
		svgview = View(self.view.canvas)
		svgview.painter = ItemPainter()

		# Update bounding boxes with a temporaly CairoContext
		# (used for stuff like calculating font metrics)
		tmpsurface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 0, 0)
		tmpcr = cairo.Context(tmpsurface)
		svgview.update_bounding_box(tmpcr)
		tmpcr.show_page()
		tmpsurface.flush()

		fn = 'demo.svg'
		w, h = svgview.bounding_box.width, svgview.bounding_box.height
		surface = cairo.SVGSurface(fn , w, h)
		cr = cairo.Context(surface)
		svgview.matrix.translate(-svgview.bounding_box.x, -svgview.bounding_box.y)
		svgview.paint(cr)
		cr.show_page()
		surface.flush()
		surface.finish()

		self.status.push(0,"Wrote SVG file '%s'." % fn)
		
	def run_presaved_canvas(self,widget):
		if self.view.canvas.saved_model is not None:
			model = self.view.canvas.saved_model 
			L.loadString(model,"canvasmodel")
			T = L.findType("canvasmodel")
			M = T.getSimulation('canvassim')
			M.setSolver(ascpy.Solver("QRSlv"))
			M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
			for item in self.view.canvas.get_all_items():

				if hasattr(item, 'blockinstance'):
					bi = item.blockinstance
					for i in M.getChildren()[0].getChildren():
						if str(bi.name) == str(i.getName()):
							bi.instance = i
							
	def load_presaved_canvas(self,widget):
		print self.view.canvas.saved_data
		try:
			if self.view.canvas.saved_data is not None:
				model = str(self.view.canvas)
				L.loadString(model,"canvasmodel")
				T = L.findType("canvasmodel")
				M = T.getSimulation('canvassim')
				
				def assignval(sim_inst, name):	
					if sim_inst.isAtom():
						try:
							sim_inst.setRealValue(self.view.canvas.saved_data[name])
						except Exception,e:
							print e	
					elif sim_inst.isRelation():
						pass	
					else:
						for i in sim_inst.getChildren():
							k = name+"."+str(i.getName())
							assignval(i,k)
							
				for i in M.getChildren()[0].getChildren():
					assignval(i, str(i.getName()))
					
				for item in self.view.canvas.get_all_items():
					if hasattr(item, 'blockinstance'):
						bi = item.blockinstance
						for i in M.getChildren()[0].getChildren():
							if str(bi.name) == str(i.getName()):
								bi.instance = i
				self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)	
		except AttributeError:					
			self.status.push(0,"Canvasmodel state : Unsolved")
			
	def run_canvas(self,widget):
		"""
		Exports canvas to ASCEND solver and attempts to solve it. Also provides realtime feedback.
		"""
		model = str(self.view.canvas)
		print model
		self.view.canvas.saved_model = model
		self.view.canvas.saved_data = {}
		#print "RUN NOT IMPLEMENTED"
		L.loadString(model,"canvasmodel")
		print "STRING MODEL LOADED"
		T = L.findType("canvasmodel")
		print "Starting simulation...."
		self.M = T.getSimulation('canvassim')
		self.M.setSolver(ascpy.Solver("QRSlv"))
		self.reporter = ascpy.getReporter()
		reporter = PopupSolverReporter(self,self.M.getNumVars())
		try:
			self.M.build()
		except RuntimeError,e:
			print "Couldn't build system: %s" % str(e)
			self.status.push(0,"Couldn't build system: %s" % str(e));
			return
		
		self.status.push(0,"Solving with 'QRSlv'. Please Wait...")
		
		try:
			self.M.solve(ascpy.Solver("QRSlv"),reporter)
			self.reporterrorblock(self.M)
		except RuntimeError:
			self.reporterrorblock(self.M)
			
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				for i in self.M.getChildren()[0].getChildren():
					if str(bi.name) == str(i.getName()):
						bi.instance = i

		#Storing solved variables into a dictionary which will be pickled
		for i in self.M.getallVariables():
			self.view.canvas.saved_data[str(i.getName())] = i.getValue()						
		
	def about(self, widget):
        	about = gtk.AboutDialog()
        	about.set_program_name("ASCEND CANVAS")
        	about.set_version("0.9.6x alpha")
        	about.set_copyright("Carnegie Mellon University")
        	about.set_comments("Canvas - Based GUI Modeller for Energy Systems")
        	about.set_website("http://www.ascend.cheme.cmu.edu")
		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
        	about.set_icon(windowicon.get_pixbuf())
		about.set_logo(gtk.gdk.pixbuf_new_from_file("../glade/ascend.png"))
        	about.run()
        	about.destroy()			   
			   
	def dummy(self, widget):		   
		dum = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "Sorry ! This fuctionality is not implemented yet.")
        	dum.run()
        	dum.destroy() 
	
	def fileopen(self, widget):
		dialog = gtk.FileChooserDialog("Open..",self,gtk.FILE_CHOOSER_ACTION_OPEN, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))
     		dialog.set_default_response(gtk.RESPONSE_OK)
		filter = gtk.FileFilter()
		filter.set_name("Canvas Files")
		filter.add_mime_type("Canvas Files/a4b")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		filter = gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		res = dialog.run()
		if res == gtk.RESPONSE_OK:
			result = dialog.get_filename() 
			import pickle as pickle
			import gaphas.picklers
			f = file(result,"r")
			try:
				self.view.canvas = pickle.load(f)
				if self.view.canvas.model_library is not None:
					print "Loading Library...."
					self.loadlib(self, self.view.canvas.model_library,0)
				self.view.canvas.reattach_ascend(L,D)
				self.view.canvas.update_now()
				self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
				self.load_presaved_canvas(None)
				self.reporter.reportError(" File %s successfully loaded." % result)
				self.status.push(0,"File %s Loaded." % result)
				self.view.canvas.filename = result
			except:
					self.reporter.reportError(" Error occured while attempting to 'Load' the file. File could be loaded properly.")
			finally:
				f.close()
		dialog.destroy()
				
	def filesave(self, widget):
	 	f = None
		dialog = gtk.FileChooserDialog("Save..", self, gtk.FILE_CHOOSER_ACTION_SAVE, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)
		if self.view.canvas.filestate == 0:
			dialog.set_filename("untitled")
		filter = gtk.FileFilter()
		filter.set_name("Canvas File")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		dialog.show()
		dialog.set_do_overwrite_confirmation(True)
		dialog.connect("confirm-overwrite", self.confirm_overwrite_callback)
		response = dialog.run()
		if response == gtk.RESPONSE_OK:
			name = dialog.get_filename()
			if '.a4b' not in name:
				name += '.a4b'
			if f == None and f != name:
				f = open(name, 'w')
				
			import pickle as pickle
			import gaphas.picklers
			try:
				pickle.dump(self.view.canvas,f)
				self.reporter.reportNote(" File ' %s ' saved successfully." % name )
				self.status.push(0,"CanvasModel Saved.")
				self.view.canvas.filestate = 1
				self.view.canvas.filename = name
			except Exception,e:
				print "ERROR:",str(e)
				import obrowser
				b = obrowser.Browser("canvas",self.view.canvas)
				d = gtk.Dialog("Error",self,gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
				d.vbox.add(gtk.Label(str(e)))
				d.show_all()
				d.run()
				d.hide()
			finally:
				f.close()
		dialog.destroy()

	def confirm_overwrite_callback(self, widget):
		uri = gtk.FileChooserDialog.get_filename()
		if is_uri_read_only(uri):
			if user_wants_to_replace_read_only_file (uri):
				return gtk.FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME
			else:
				return gtk.FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
		else:
			return gtk.FILE_CHOOSER_CONFIRMATION_CONFIRM 
		return
		
		
	def bp(self, widget):
		if self.view.focused_item:
			blockproperties.BlockProperties(self, self.view.focused_item).run()
		else:
			m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "No Block was selected! Please select a Block to view its properties.")
			m.run()	
	   		m.destroy()
			
	def custommethod(self, widget):
		if self.view.focused_item:
			blockproperties.BlockProperties(self, self.view.focused_item,2).run()
		else:
			m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "No Block was selected! Please select a Block first.")
			m.run()	
	   		m.destroy()	
		
	def reporterrorblock(self, sim):
		self.errortext = "Canvasmodel could NOT be solved !!\n------------------------------------\nBlocks which couldnot be solved :: "
		
		def checkifsolved(sim_inst, name):
				if sim_inst.getType().isRefinedSolverVar():
					try:
						#if sim_inst.getStatus() == 2 :
						#	print name + " -> " + str(sim_inst.getStatus())
						#	self.errorvars.append(name)
						if sim_inst.getStatus() == 3:
							self.errorvars.append(name)	
							
					except Exception,e:
						print e	
				elif sim_inst.isRelation():
					pass	
				else:
					for i in sim_inst.getChildren():
						k = name+"."+str(i.getName())
						checkifsolved(i,k)
		
		self.errorvars = []
		self.errorblocks = []
		self.activevar = []			
		for i in sim.getChildren()[0].getChildren():
			print i.getName()
			checkifsolved(i, str(i.getName()))
			
		print self.errorvars
		for i in sim.getChildren()[0].getChildren():
			for j in self.errorvars:
				if str(i.getName()) in j:
					if str(i.getName()) in self.errorblocks:
						pass
					else:
						self.errortext += '\n'
						self.errortext += str(i.getName()) #+' --> ' + ' couldnot be solved'
						self.errorblocks.append(str(i.getName()))
		print self.errorblocks									
		self.fill_blocks()
	
		
	def fill_blocks(self):
		cr = self.view.canvas._obtain_cairo_context()	
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				bi.color_r = 1
				bi.color_g = 1
				bi.color_b = 1
				flag = 0		
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				for i in self.errorblocks:
					if str(bi.name) == str(i):
						bi.color_r = .5
						bi.color_g = 0
						bi.color_b = 0
						flag = 1
	
		print "updating canvas"				
		self.view.canvas.update_now()
		if flag == 1:
			 self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#F88'))
			 flag = 0
		else:
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#AFA'))	 
			
	def loadlib(self, widget, lib_path, loadcondition = 1):
		if self.view.canvas.model_library == lib_path and loadcondition == 1:
				m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_WARNING, gtk.BUTTONS_CLOSE, "The selected Library is already loaded. ")
				m.run()	
				m.destroy()
				return
		if self.view.canvas.get_all_items() and loadcondition == 1:	
				m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "Unable to switch Library ! The canvas contains models from present Model Library. ")
				m.run()	
				m.destroy()
				return

		L.clear()
		self.view.canvas.model_library = lib_path
		L.load(lib_path)

		D = L.getAnnotationDatabase()

		M = L.getModules()
		
		blocktypes = set()
		
		for m in M:
			T = L.getModuleTypes(m)
			for t in T:
				if t.hasParameters():
					continue
				x = D.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
				if x:
					blocktypes.add(t)
		blocks = []
		print blocktypes
		print "block types:"
		if not blocktypes:
			print "NONE FOUND"
		for t in blocktypes:
			b = BlockType(t,D)
			blocks += [b]
		self.scroll.remove(self.blockiconview)
		self.blockiconview = BlockIconView(blocks, self)
		self.scroll.add(self.blockiconview)
		self.show_all()
		self.status.push(0, " Library '%s' loaded :: Found %d block types." %(lib_path, (len(blocks))))
		
	def methodentry(self, widget):
		import methodentry
		methodentry.MethodEntry().run()
			
	def zoom(self, widget, x):
		if x == 1: 
			self.view.zoom(1/self.act)
			self.act = 1
			return
		self.act = self.act*x
		self.view.zoom(x)
	
		
	def fullscrn(self, widget):
		try:
			if  self.flag == 1:
			 	self.unfullscreen()
			 	self.flag = 0
			else:
				self.fullscreen()
				self.flag = 1	
		except:
			self.fullscreen()
			self.flag = 1
				 	

	def on_contents_click(self, widget):
		import urllib, help
		_help = help.Help(url="http://ascendwiki.cheme.cmu.edu/User:Arijit#Tutorial_for_Canvas_Modeler")
		_help.run()
					
	def on_get_help_online_click(self, widget):
		import urllib, help
		_help = help.Help(url="http://ascendwiki.cheme.cmu.edu/Canvas-based_modeller_for_ASCEND")
		_help.run()				
					
	def on_report_a_bug_click(self, widget):
		import urllib, help
		_help = help.Help(url="http://ascendbugs.cheme.cmu.edu/report/")
		_help.run()				
		
a = app()
gtk.main()
