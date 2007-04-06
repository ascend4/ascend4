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

import ipython_view

if ipython_view.IPython:
	def create_widget(browser):
		V = ipython_view.IPythonView()
		V.modify_font(pango.FontDescription(FONT))
		V.set_wrap_mode(gtk.WRAP_CHAR)
		V.show()
		browser.consolescroll.add(V)
		V.updateNamespace({'browser': browser})
else:
	def create_widget(browser):
		V = gtk.Label()
		V.set_text("IPython not found. Is it installed?");
		V.show()
		browser.consolescroll.add(V)
		
