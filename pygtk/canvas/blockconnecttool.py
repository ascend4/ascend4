
from gaphas.tool import ConnectHandleTool

class BlockConnectTool(ConnectHandleTool):
	"""
		This class makes the necessary changes to the application-layer data
		when a handle is connected to a port, or disconnected.

		The `ConnectorTool` will also be making use of this class.
	"""
	def post_connect(self, line, handle, item, port):
		assert(line.lineinstance is not None)
		assert(port.portinstance is not None)

		if handle is line._handles[0]:
			line.lineinstance.fromport = port.portinstance
		elif handle is line._handles[-1]:
			line.lineinstance.toport = port.portinstance
		else:
			raise RuntimeError("Invalid handle, neither start nor end")
		
	def disconnect(self, view, line, handle):
		super(BlockConnectTool,self).disconnect(view, line, handle)
		assert(line.lineinstance is not None)
		if handle is line._handles[0]:
			line.lineinstance.fromport = None
		elif handle is line._handles[-1]:
			line.lineinstance.toport = None

