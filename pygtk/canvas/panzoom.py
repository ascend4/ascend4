# Pan and Zoom tools for Gaphas by John Pye, 4 Nov 2008.
# http://pye.dyndns.org

import gtk
import gtk.gdk as gdk

from gaphas.tool import Tool

ZOOM_MASK = gdk.CONTROL_MASK | gdk.SHIFT_MASK | gdk.MOD1_MASK
ZOOM_VALUE =gdk.CONTROL_MASK

class ZoomTool(Tool):
	"""
	Tool for zooming using either of two techniquies:
		* ctrl + middle-mouse dragging in the up-down direction.
		* ctrl + mouse-wheeel

	This tool checks for correct modifier key
	
	
	"""

	def __init__(self):
		self.x0, self.y0 = 0, 0
		self.lastdiff = 0;

	def on_button_press(self, context, event):
		if event.button == 2 \
				and event.state & ZOOM_MASK == ZOOM_VALUE:
			print "GRABBING"
			context.grab()
			self.x0 = event.x
			self.y0 = event.y
			self.lastdiff = 0
			return True

	def on_button_release(self, context, event):
		context.ungrab()
		self.lastdiff = 0
		return True

	def on_motion_notify(self, context, event):
		if event.state & ZOOM_MASK == ZOOM_VALUE:
			view = context.view
			dy = event.y - self.y0

			sx = view._matrix[0]
			sy = view._matrix[3]
			ox = (view._matrix[4] - self.x0) / sx
			oy = (view._matrix[5] - self.y0) / sy

			if abs(dy - self.lastdiff) > 20:
				if dy - self.lastdiff < 0:
					factor = 1./0.9
				else:
					factor = 0.9

				view._matrix.translate(-ox, -oy)
				view._matrix.scale(factor, factor)
				view._matrix.translate(+ox, +oy)

				# Make sure everything's updated
				map(view.update_matrix, view._canvas.get_all_items())
				view.request_update(view._canvas.get_all_items())

				self.lastdiff = dy;
			return True

	def on_scroll(self, context, event):
		if event.state & gtk.gdk.CONTROL_MASK:
			view = context.view
			context.grab()
			sx = view._matrix[0]
			sy = view._matrix[3]
			ox = (view._matrix[4] - event.x) / sx
			oy = (view._matrix[5] - event.y) / sy
			factor = 0.9
			if event.direction == gtk.gdk.SCROLL_UP:	
				factor = 1. / factor
			view._matrix.translate(-ox, -oy)
			view._matrix.scale(factor, factor)
			view._matrix.translate(+ox, +oy)
			# Make sure everything's updated
			map(view.update_matrix, view._canvas.get_all_items())
			view.request_update(view._canvas.get_all_items())
			context.ungrab()
			return True



class PanTool(Tool):
    """
    Captures drag events with the middle mouse button and uses them to
    translate the canvas within the view. Trumps the ZoomTool, so should be
    placed later in the ToolChain.
    """
    def __init__(self):
        self.x0, self.y0 = 0, 0

    def on_button_press(self,context,event):
        if event.button == 2:
            context.grab()
            self.x0, self.y0 = event.x, event.y
            return True

    def on_button_release(self, context, event):
        context.ungrab()
        self.x0, self.y0 = event.x, event.y
        return True

    def on_motion_notify(self, context, event):
        if event.state & gtk.gdk.BUTTON2_MASK:
            view = context.view
            self.x1, self.y1 = event.x, event.y
            dx = self.x1 - self.x0
            dy = self.y1 - self.y0
            view._matrix.translate(dx/view._matrix[0],dy/view._matrix[3])
            # Make sure everything's updated
            map(view.update_matrix, view._canvas.get_all_items())
            view.request_update(view._canvas.get_all_items())
            self.x0 = self.x1
            self.y0 = self.y1
            return True
