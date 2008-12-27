# ConnectorTool for Gaphas canvas, by John Pye, 4 Nov 2008.
# http://pye.dyndns.org

from gaphas.tool import HandleTool
from connecthandletool import ConnectHandleTool
from gaphas.item import Line
import cairo

class ConnectorTool(ConnectHandleTool):
	"""
	This is a port-connecting handle tool that additionally initiates the
	creation of connector lines when the user is withing gluing distance of an 
	as-yet-unconnected Port.
	
	handle_tool should normally be set to HandleTool()
	"""
	def __init__(self):
		self._handle_tool = HandleTool()
		self._handle_index_glued = 0
		self._handle_index_dragged = 1
		self._grabbed_handle = None
		self._grabbed_item = None
		self._new_item = None

	def on_button_press(self,context,event):
		if event.button != 1:
			return False

		view = context.view
		canvas = view.canvas
		glueitem, glueport = self.glue(view, None, None, (event.x, event.y))
		if glueport and hasattr(glueport,"point"):
			line = self._create_line(context,event.x, event.y)
			canvas.get_matrix_i2c(line, calculate=True)
			self._new_item = line
			view.focused_item = line
			del view.selected_items
			context.grab()

			h_glue = line.handles()[self._handle_index_glued]
			self.connect(view,line,h_glue, (event.x, event.y))

			h_drag = line.handles()[self._handle_index_dragged]
			self._handle_tool.grab_handle(line, h_drag)
			self._grabbed_handle = h_drag
			self._grabbed_item = line
			
			print "STARTED NEW CONNECTOR"
			return True

	def _create_line(self, context, x, y):
		view = context.view
		canvas = view.canvas
		line = Line()
		#line.orthogonal = True
		line.fuzziness = 5
		canvas.add(line)
		x, y = view.get_matrix_v2i(line).transform_point(x, y)
		line.matrix.translate(x, y)
		return line

