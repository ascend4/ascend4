
from gaphas.tool import HandleTool, PlacementTool
from gaphas import geometry
from blockconnecttool import BlockConnectTool
from blockline import BlockLine
import cairo

# ConnectorTool for Gaphas canvas, by John Pye, 4 Nov 2008.
# http://pye.dyndns.org

class ConnectorTool(BlockConnectTool):
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
		self.grabbed_handle = None
		self.grabbed_item = None
		self._new_item = None
		self.motion_handle = None

	def on_button_press(self,event):		
		
		if event.button != 1:
			return False

		glueitem,glueport,gluepos = self.view.get_port_at_point((event.x,event.y),distance = 10,exclude = [])
		
		if glueport and hasattr(glueport,"point"):
			
			#print "Conn.Tool L36\n: "glueitem,glueport,gluepos
			self.line = self._create_line((event.x, event.y))
			self._new_item = self.line
			h_glue = self.line.handles()[self._handle_index_glued]
			self.connect(self.line,h_glue,(event.x,event.y))
			self.post_connect(self.line,h_glue,None,glueport)
			#print "Conn.Tool L40\n"
			
			h_drag = self.line.handles()[self._handle_index_dragged]
			self._handle_tool.grab_handle(self.line, h_drag)
			self.grabbed_handle = h_drag
			self.grabbed_item = self.line
			
			return True

	def on_button_release(self,event):

		dragitem,dragport,dragpos = self.view.get_port_at_point((event.x,event.y),distance = 10,exclude = [])
		
		try:
			if dragport:
				h_drag = self.line.handles()[self._handle_index_dragged]
				self.post_connect(self.line,h_drag,None,dragport)
		finally:
			return super(ConnectorTool, self).on_button_release(event)
		
		
	def _create_line(self, (x, y)):
		canvas = self.view.canvas
		line = BlockLine()
		#line.orthogonal = True
		line.fuzziness = 5
		canvas.add(line)
		x, y = self.view.get_matrix_v2i(line).transform_point(x, y)
		line.matrix.translate(x, y)
		return line