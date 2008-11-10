#!/usr/bin/env python
from __future__ import with_statement
import os, sys

os.chdir(os.path.abspath(os.path.dirname(sys.argv[0])))

os.environ['ASCENDLIBRARY'] = "../../models"

if sys.platform.startswith("win"):
	os.environ['PATH'] += ";..\.."
else:
	os.environ['LD_LIBRARY_PATH'] = "../.."
	
sys.path.append("..")

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

# FIXME need to add way to add/remove modules from the Library?
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

#gtk.gdk.threads_init()

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
		#thread = threading.RLock()
		n = 0
		#with thread:
		for b in blocks:
			n += 1
			pixbuf = b.get_icon(64,64)
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
from gaphas.tool import Tool, ItemTool, RubberbandTool
from gaphas.item import Line
from blockitem import *
from contextmenutool import *
from connectortool import *
from portconnectinghandletool import *
from blockcanvas import *
from panzoom import *
from blockinstance import *

def BlockToolChain():
	"""
	The default tool chain build from HoverTool, ItemTool and HandleTool.
	"""
	chain = ToolChain()
	chain.append(HoverTool())
	chain.append(PortConnectingHandleTool())
	chain.append(ConnectorTool())
	chain.append(ContextMenuTool())
	chain.append(ItemTool())
	chain.append(ZoomTool())
	chain.append(PanTool())
	chain.append(RubberbandTool())
	return chain

class app(gtk.Window):
	def __init__(self):
		self.status = gtk.Statusbar()

		# the Gaphas canvas
		canvas = BlockCanvas()

		# the main window
		gtk.Window.__init__(self)
		self.set_title("ASCEND Blocks")
		self.set_default_size(400, 500)
		self.connect("destroy", gtk.main_quit)
		self.connect("key-press-event", self.key_press_event)

		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
		self.set_icon(windowicon.get_pixbuf())

		# vbox containing the main view and the status bar at the bottom
		vbox = gtk.VBox()

		# hbox occupies top part of vbox, with icons on left & canvas on right.
		hbox = gtk.HBox()

		# the 'view' widget implemented by Gaphas
		self.view = GtkView()
		self.view.tool =  BlockToolChain()

		# table containing scrollbars and main canvas 
		t = gtk.Table(2,2)
		self.view.canvas = canvas
		self.view.zoom(1)
		self.view.set_size_request(600, 500)
		hs = gtk.HScrollbar(self.view.hadjustment)
		vs = gtk.VScrollbar(self.view.vadjustment)
		t.attach(self.view, 0, 1, 0, 1)
		t.attach(hs, 0, 1, 1, 2, xoptions=gtk.FILL, yoptions=gtk.FILL)
		t.attach(vs, 1, 2, 0, 1, xoptions=gtk.FILL, yoptions=gtk.FILL)

		# a scrolling window to contain the icon palette
		scroll = gtk.ScrolledWindow()
		scroll.set_border_width(2)
		scroll.set_shadow_type(gtk.SHADOW_ETCHED_IN)
		scroll.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)

		# icon palette
		self.blockiconview = BlockIconView(blocks, self)
		scroll.add(self.blockiconview)

		hbox.pack_start(scroll, True, True)
		hbox.pack_start(t, True, True)
		vbox.pack_start(hbox, True, True)
		vbox.pack_start(self.status, False, False)
		self.add(vbox)
		self.show_all()

		# a message about the found blocks
		self.status.push(0, "Found %d block types." % (len(blocks)))
				
	def set_placement_tool(self,blocktype):
		# TODO: add undo handler
		label = blocktype.type.getName()
		def my_block_factory():
			def wrapper():
				b = BlockInstance(blocktype)
				bi = DefaultBlockItem(b)
				self.view.canvas.add(bi)
				return bi
			return wrapper
		self.view.tool.grab(PlacementTool(my_block_factory(), HandleTool(), 2))
		self.status.push(0,"Selected '%s'..." % blocktype.type.getName())

	def set_connector_tool(self):
		def my_line_factory():
			def wrapper():
				l =  Line()
				self.view.canvas.add(l)
				return l
			return wrapper
		self.view.tool.grab(PlacementTool(my_line_factory(), HandleTool(), 1))

	def key_press_event(self,widget,event):
		# TODO: add undo handler
		key = gtk.gdk.keyval_name(event.keyval)
		if key == 'Delete' and self.view.focused_item:
			self.view.canvas.remove(self.view.focused_item)
			self.status.push(0,"Item deleted.")
		elif key == 'l' or key == 'L':
			self.set_connector_tool()
			self.status.push(0,"Line draw mode...")
	   
a = app()
gtk.main() 

