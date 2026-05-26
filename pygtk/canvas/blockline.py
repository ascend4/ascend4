
from gaphas.item import Line
from gaphas.connections import Connections
from blockinstance import LineInstance

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
	
	def get_connected_ports(self):
		ports = [self.lineinstance.fromport,self.lineinstance.toport]
		return ports
