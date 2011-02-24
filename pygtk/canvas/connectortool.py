
from gaphas.tool import HandleTool, PlacementTool
from gaphas import geometry
from gaphas.aspect import Connector, ConnectionSink, HandleInMotion
from gaphas.segment import Segment
from blockconnecttool import BlockConnectTool
from blockline import BlockLine
import cairo
import undo

class ConnectorTool(BlockConnectTool):
	"""
	This is a port-connecting handle tool that additionally initiates the
	creation of connector lines when the user is withing gluing distance of an 
	as-yet-unconnected Port.
	"""
	def __init__(self):
		self._handle_index_glued = 0
		self._handle_index_dragged = 1
		self.grabbed_handle = None
		self.grabbed_item = None
		self._new_item = None
		self.motion_handle = None
	
	def on_button_press(self,event):
		
		glueitem,glueport,gluepos = self.view.get_port_at_point((event.x,event.y),distance = 10,exclude = [])
		line,handle =  self.view.get_handle_at_point((event.x,event.y))
		try:
			if glueport and hasattr(glueport,"point"):
				self.toggle_highlight_ports(glueport.portinstance)
				self.line = self._create_line((event.x, event.y))
				self._new_item = self.line
				h_glue = self.line.handles()[self._handle_index_glued]
				conn = Connector(self.line,h_glue)
				sink = ConnectionSink(glueitem,glueport)
				conn.connect_port(sink)
				h_drag = self.line.handles()[self._handle_index_dragged]
				self.grab_handle(self.line,h_drag)
				return True
		except Exception as e:
			print 'Connection Failed, Disconnect/Connect the last Connection again: /n',e
			
	def _create_line(self, (x, y)):
		
		canvas = self.view.canvas
		line = BlockLine()
		segment = Segment(line, view=self.view)
		canvas.add(line)
		x, y = self.view.get_matrix_v2i(line).transform_point(x, y)
		line.matrix.translate(x, y)
		line.fuzziness = 1.0
		
		return line