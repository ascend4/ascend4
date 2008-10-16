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

print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:
	print t.getName()

	nn = D.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
	for n in nn: 
		print "\t%s" % n.getText()
	nn = D.getTypeRefinedNotesLang(t,ascpy.SymChar("inline"))

	inputs = []
	outputs = []
	for n in nn:
		t = n.getText()
		if t[0:min(len(t),3)]=="in:":
			inputs += [n]
		elif t[0:min(len(t),4)]=="out:":
			outputs += [n]

	print "\t\tinputs:",[n.getId() for n in inputs]
	for n in inputs:
		print "\t\t\t%s: %s (type = %s)" % (n.getId(),n.getText(),n.getType())
	print "\t\toutputs:",[n.getId() for n in outputs]
	for n in outputs:
		print "\t\t\t%s: %s" % (n.getId(),n.getText())

