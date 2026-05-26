
from gaphas.item import Line
from gaphas.connections import Connections
from gaphas.connector import Connector, LineConnector
from blockinstance import LineInstance, PORT_IN, PORT_OUT, PORT_INOUT

def portinstance_color(portinstance):
	if portinstance is None:
		return None
	if portinstance.io == PORT_IN:
		return (1.0, 0.05, 0.0)
	if portinstance.io == PORT_OUT:
		return (0.0, 0.65, 0.25)
	if portinstance.io == PORT_INOUT:
		return (0.1, 0.35, 1.0)
	return (0.0, 0.0, 0.0)

class BlockLine(Line):
	"""
	This is a subclass of Line that keeps a line to an embedded
	`ConnectorInstance` which holds the block-diagram data relating to the
	ports connected at either end of the line.

	The `ConnectorInstance` will be manipulated by the `BlockConnectorTool`.
	"""

	def __init__(self, connections=None):
		super(BlockLine, self).__init__(connections or Connections())
		self.lineinstance = LineInstance()
		self.fuzziness = 8.0
		for handle in self.handles():
			handle.visible = False
	
	def get_connected_ports(self):
		ports = [self.lineinstance.fromport,self.lineinstance.toport]
		return ports

	def draw(self, context):
		super(BlockLine, self).draw(context)
		self.draw_endpoint_marker(
			context, self.handles()[0], self.lineinstance.fromport, context.selected
		)
		self.draw_endpoint_marker(
			context, self.handles()[-1], self.lineinstance.toport, context.selected
		)

	def draw_endpoint_marker(self, context, handle, portinstance, selected=False):
		cr = context.cairo
		x, y = handle.pos
		if selected:
			color = portinstance_color(portinstance) or (0.0, 0.0, 0.0)
			size = 5
			opacity = 0.9
		else:
			color = (0.0, 0.0, 0.0)
			size = 3
			opacity = 1.0
		r, g, b = color
		cr.save()
		try:
			cr.rectangle(x - size, y - size, size * 2, size * 2)
			cr.set_source_rgba(r, g, b, opacity)
			cr.fill_preserve()
			cr.set_line_width(1.5)
			if selected:
				cr.set_source_rgb(max(r * 0.45, 0), max(g * 0.45, 0), max(b * 0.45, 0))
			else:
				cr.set_source_rgb(0.0, 0.0, 0.0)
			cr.stroke()
		finally:
			cr.restore()


@Connector.register(BlockLine)
class BlockLineConnector(LineConnector):
	def connect_handle(self, sink, callback=None):
		super(BlockLineConnector, self).connect_handle(
			sink, callback=self.disconnect_handle
		)
		self.set_lineinstance_port(sink.port)

	def disconnect_handle(self, item, handle, connected, port):
		self.set_lineinstance_port(None, handle)

	def set_lineinstance_port(self, port, handle=None):
		handle = handle or self.handle
		portinstance = getattr(port, "portinstance", None)
		if handle is self.item.handles()[0]:
			self.item.lineinstance.fromport = portinstance
		elif handle is self.item.handles()[-1]:
			self.item.lineinstance.toport = portinstance
