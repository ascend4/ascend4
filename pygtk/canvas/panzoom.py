# Pan and Zoom tools for Gaphas by John Pye, 4 Nov 2008.

from gi.repository import Gdk
from gaphas.tool import ZoomTool, PanTool


class ZoomTool(ZoomTool):

	def __init__(self, view=None):
		super(ZoomTool, self).__init__(view)

	def on_scroll(self, event):
		if event.get_state()[1] & Gdk.ModifierType.CONTROL_MASK:
			view = self.view
			sx = view._matrix[0]
			sy = view._matrix[3]
			ox = (view._matrix[4] - event.scroll.x) / sx
			oy = (view._matrix[5] - event.scroll.y) / sy
			factor = 0.9
			if event.scroll.direction == Gdk.ScrollDirection.UP:
				factor = 1. / factor
			view._matrix.translate(-ox, -oy)
			view._matrix.scale(factor, factor)
			view._matrix.translate(+ox, +oy)
			# Make sure everything's updated
			view.request_update((), view._canvas.get_all_items())
			return True

PAN_MASK = Gdk.ModifierType.SHIFT_MASK | Gdk.ModifierType.MOD1_MASK | Gdk.ModifierType.CONTROL_MASK
PAN_VALUE = 0


class PanTool(PanTool):
	def __init__(self, view=None):
		super(PanTool, self).__init__(view)

	def on_scroll(self, event):
		# Ensure no modifiers
		if not event.get_state()[1] & PAN_MASK == PAN_VALUE:
			return False
		view = self.view
		direction = event.scroll.direction
		if direction == Gdk.ScrollDirection.LEFT:
			view._matrix.translate(self.speed / view._matrix[0], 0)
		elif direction == Gdk.ScrollDirection.RIGHT:
			view._matrix.translate(-self.speed / view._matrix[0], 0)
		elif direction == Gdk.ScrollDirection.UP:
			view._matrix.translate(0, self.speed / view._matrix[3])
		elif direction == Gdk.ScrollDirection.DOWN:
			view._matrix.translate(0, -self.speed / view._matrix[3])
		view.request_update((), view._canvas.get_all_items())
		return True
