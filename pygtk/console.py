# CONSOLE ACCESS to ASCEND from PYTHON

argv=['-gthread','-pi1','In <\\#>:','-pi2','   .\\D.:','-po','Out<\\#>:','-noconfirm_exit']
banner = "\n\n>>> ASCEND PYTHON CONSOLE: type 'help(ascpy)' for info, ctrl-D to resume ASCEND"
exitmsg = '>>> CONSOLE EXIT'

import gtk
import pango

import platform
if platform.system()=="Windows":
	FONT = "Lucida Console 9"
else:
	FONT = "Luxi Mono 10"

try:
	import ipython_view
	have_ipython = 1
except:
	have_ipython = 0

def create_widget(browser):
	try:
		if not have_ipython:
			raise Exception("IPython could not be load (is it installed?)")
		V = ipython_view.IPythonView()
	except Exception,e:
		V = gtk.Label()
		V.set_text("IPython error: %s" % str(e));
		V.show()
		browser.consolescroll.add(V)
		browser.consoletext = V
		return

	V.modify_font(pango.FontDescription(FONT))
	V.set_wrap_mode(gtk.WRAP_CHAR)
	V.show()
	browser.consolescroll.add(V)
	browser.consoletext = V
	V.updateNamespace({'browser': browser})
		
