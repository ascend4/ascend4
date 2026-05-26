"""Legacy block-connection tool placeholder.

The old implementation subclassed Gaphas' removed PyGTK ``ConnectHandleTool``
and aspect system.  Gaphas 3 uses Gtk.EventController based tools and
``Canvas.connections``.  The visual port highlighting flag is still consumed by
``blockitem``; actual drag-to-connect behaviour needs a dedicated Gtk3/Gaphas 3
tool port.
"""

SET_CONNECTION_FLAG = [False, None]


class BlockConnectTool:
	def __init__(self, view=None):
		self.view = view

	def toggle_highlight_ports(self, portinstance=None):
		if SET_CONNECTION_FLAG[0]:
			SET_CONNECTION_FLAG[0] = False
			SET_CONNECTION_FLAG[1] = None
		elif portinstance:
			SET_CONNECTION_FLAG[0] = True
			SET_CONNECTION_FLAG[1] = portinstance
		if self.view is not None:
			self.view.queue_draw()
