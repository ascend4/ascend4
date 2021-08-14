# CONSOLE ACCESS to ASCEND from PYTHON

argv=['-gthread','-pi1','In <\\#>:','-pi2','   .\\D.:','-po','Out<\\#>:','-noconfirm_exit']
banner = "\n\n>>> ASCEND PYTHON CONSOLE: type 'help(ascpy)' for info, ctrl-D to resume ASCEND"
exitmsg = '>>> CONSOLE EXIT'

from gi.repository import Gtk
from gi.repository import Pango

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


# Displays a confimation dialog box to quit pygtk main_window
# This method is mapped to quit() and exit() calls in the ipython console.
def close_window_on_confirm(browser):
	dialog = Gtk.MessageDialog(browser.window, Gtk.DialogFlags.MODAL,
	                           Gtk.MessageType.INFO, Gtk.ButtonsType.YES_NO,"Are you sure you want to quit ASCEND?")

	dialog.set_title("Quit")
	response = dialog.run()
	dialog.destroy()
	if response == Gtk.ResponseType.YES:
		browser.do_quit()
		return False
	else:
		return True

def create_widget(browser):
	try:
		if not have_ipython:
			raise Exception("IPython could not be load (is it installed?)")
		V = ipython_view.IPythonView()
	except Exception as e:
		V = Gtk.Label()
		V.set_text("IPython error: %s" % str(e));
		V.show()
		browser.consolescroll.add(V)
		browser.consoletext = V
		return

	V.modify_font(Pango.FontDescription(FONT))
	V.set_wrap_mode(Gtk.WrapMode.CHAR)
	V.show()
	browser.consolescroll.add(V)
	browser.consoletext = V
	V.updateNamespace({'browser': browser})
	V.updateNamespace({'exit':lambda:close_window_on_confirm(browser)})
	V.updateNamespace({'quit':lambda:close_window_on_confirm(browser)})

