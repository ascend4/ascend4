
from gaphas.item import Line
from blockinstance import ConnectorInstance

class BlockLine(Line):
	"""
	This is a subclass of Line that keeps a line to an embedded
	`ConnectorInstance` which holds the block-diagram data relating to the
	ports connected at either end of the line.

	The `ConnectorInstance` will be manipulated by the `BlockConnectorTool`.
	"""

	class __init__(self):
		super(BlockLine, self).__init__()
		self.connectorinstance = ConnectorInstance()


