from __future__ import with_statement
import sys

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
		return gtk.gdk.pixbuf_new_from_file_at_size("canvas/defaultblock.svg",width,height)

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
gtk.gdk.threads_init()

class app(gtk.Window):
	def __init__(self):
		gtk.Window.__init__(self)
		self.set_title("ASCEND Blocks")
		self.set_default_size(400, 500)
		self.connect("destroy", gtk.main_quit)

		scroll = gtk.ScrolledWindow()
		scroll.set_border_width(2)
		scroll.set_shadow_type(gtk.SHADOW_ETCHED_IN)
		scroll.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)

		thumb_view = gtk.IconView()
		model = gtk.ListStore(str, gtk.gdk.Pixbuf)
		thumb_view.set_model(model)
		thumb_view.set_text_column(0)
		thumb_view.set_pixbuf_column(1)
		thumb_view.set_columns(-1)

		vbox = gtk.VBox()
		status = gtk.Statusbar()

		scroll.add(thumb_view)
		vbox.pack_start(scroll, True, True)
		vbox.pack_start(status, False, False)
		self.add(vbox)
		self.show_all()

		dirn = "/home/john/ENGN2222/Pictures"
		files = os.listdir(dirn)
		thread = threading.RLock()
		r = re.compile("^.*\\.(png|jpg)")
		n = 0
		with thread:
			for b in blocks:
				n += 1
				pixbuf = b.get_icon(96,96)
				model.append([b.type.getName(), pixbuf])
			   
		status.push(0, "Found %d images amongst %d files." % (n,len(files)))
	   
a = app()
gtk.main() 

