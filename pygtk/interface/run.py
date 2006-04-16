import sys, dl
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)
import ascpy, gtkbrowser

L = ascpy.Library()
L.load("clfr-models/water4.a4c")
t = L.findType("steam_state")
t2 = L.findType("solver_var")
sim = t.getSimulation("i")
sim.build()
b = gtkbrowser.Browser(sim)
b.run()

print "BACK AT TOP LEVEL"
del(b)
