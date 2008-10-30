from __future__ import with_statement
import os, sys

os.chdir(os.path.dirname(sys.argv[0]))
os.environ['ASCENDLIBRARY'] = "../../models"
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

class Block:
	def __init__(self, typedesc, notesdb):
		self.type = typedesc
		self.notesdb = notesdb

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))

		self.inputs = []
		self.outputs = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [n]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [n]

	def get_icon(self, width, height):
		return gtk.gdk.pixbuf_new_from_file_at_size("defaultblock.svg",width,height)


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

print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:

	b = Block(t,D)

	blocks += [b]

	print b.type.getName()

	print "\t\tinputs:",[n.getId() for n in b.inputs]
	for n in b.inputs:
		print "\t\t\t%s: %s (type = %s)" % (n.getId(),n.getText(),n.getType())
	print "\t\toutputs:",[n.getId() for n in b.outputs]
	for n in b.outputs:
		print "\t\t\t%s: %s" % (n.getId(),n.getText())


# render icon table
import threading
import gtk
import os, os.path, re

import cairo
from gaphas import GtkView, View
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import Tool, ItemTool, RubberbandTool
from port import *

gtk.gdk.threads_init()

class BlockIconView(gtk.IconView):
	def __init__(self,blocks,app):
		# the mode containing the icons themselves...
		self.model = gtk.ListStore(str, gtk.gdk.Pixbuf)
		self.app = app
		self.otank = {}
		thread = threading.RLock()
		n = 0
		with thread:
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
	def item_activated(self,iconview,path):
		b = self.otank[path]
		self.app.status.push(0, "Activated '%s'..." % b.type.getName())
        #view.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.CROSSHAIR))
		self.app.set_placement_tool(b)

	def selection_changed(self,iconview):
		s = self.get_selected_items()
		if len(s)==1:
			b = self.otank[s[0]]
			self.app.set_placement_tool(b)
			self.app.status.push(0,"Selected '%s'..." % b.type.getName())

class ContextMenuTool(Tool):
	"""
	Context menu for blocks and connectors on the canvas, intended to be
	the main mouse-based way by which interaction with blocks occurs (blocks
	renamed, parameters specified, values examined, etc).
	Code for performing these tasks should not be here; this should just
	hook into the appropriate code in the Application layer.
	"""
	def __init__(self):
		pass
	def on_button_press(self, context, event):
		if event.button != 3:
			return False
		if context.view.hovered_item:
			menu = gtk.Menu()
			menurename = gtk.MenuItem("Re_name",True);
			menurename.connect("activate",self.rename)
			menu.add(menurename)
			menu.show_all()		
			menu.popup( None, None, None, event.button, event.time)

	def rename(self,widget):
		print "RENAMING OBJECT"

def BlockToolChain():
    """
    The default tool chain build from HoverTool, ItemTool and HandleTool.
    """
    chain = ToolChain()
    chain.append(HoverTool())
    chain.append(PortConnectingHandleTool())
    chain.append(ContextMenuTool())
    chain.append(ItemTool())
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
				
	def set_placement_tool(self,block):
		# TODO: add undo handler
		label = block.type.getName()
		def my_block_factory():
			def wrapper():
				b = DefaultBlock(label,inputs=len(block.inputs),outputs=len(block.outputs))
				self.view.canvas.add(b)
				return b
			return wrapper
		self.view.tool.grab(PlacementTool(my_block_factory(), HandleTool(), 2))

	def key_press_event(self,widget,event):
		# TODO: add undo handler
		key = gtk.gdk.keyval_name(event.keyval)
		if key == 'Delete' and self.view.focused_item:
			self.view.canvas.remove(self.view.focused_item)
			self.status.push(0,"Item deleted.")
	   
a = app()
gtk.main() 

