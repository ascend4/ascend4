from gaphas.tool import ConnectHandleTool
from gaphas.aspect import HandleFinder, Connector, ConnectionSink
from blockline import BlockLine
from blockitem import DefaultBlockItem
from gaphas.connector import LinePort
from gaphas.state import observed, reversible_pair

#TODO: Do not use flags! [<In connection>,<PortInstance Object>]
#Note that both Input/Output type and ASCEND Type should match to have port highlighted
SET_CONNECTION_FLAG = [False,None]

class BlockConnectTool(ConnectHandleTool):
	"""
		This class modifies the ConnectHandleTool for Drag and Connect support.
		The `ConnectorTool` will also be making use of this class.
	"""	
	def on_button_press(self,event):
		"""
		Returns a True in case of a click on a Line Handle. This allows the 
		on_button_release of this tool to be triggered. This provides a 
		mechanism to rejoin a stray line handle previously disconnected.
		"""
		self.h_finder = HandleFinder(None,self.view)
		try:
			handle = self.h_finder.get_handle_at_point((event.x,event.y))
			if type(handle[0])==BlockLine and handle[1]:
				line = handle[0]
				ports = line.get_connected_ports()
				connectedport = ports[0] or ports[1]
				self.toggle_highlight_ports(connectedport)
				return True
		except Exception as e:
			print 'Connection Failed, Disconnect/Connect the last Connection again: /n',e
		finally:
			return super(BlockConnectTool,self).on_button_press(event)
	
	def on_button_release(self,event):
		"""
		This handles and 'drag' part of any connection, after the 
		on_button_press  of this tool returns a True (i.e by clicking on an 
		Line Handle) the on_button_release is triggered on mouse button release.
		Note that this handles both the new connections and modified 
		connections.
		Hint: 
		ConnectorTool is a child of this class.
		Uncommenting all the print statements should give a fair idea how this 
		tool works.
		"""
		line = self.grabbed_item
		handle = self.grabbed_handle
		glueitem,glueport,gluepos = self.view.get_port_at_point((event.x,event.y),distance = 10,exclude = [line])	
		line = self.grabbed_item
		handle = self.grabbed_handle
		
		try: 
			if glueport and hasattr(glueport,"point"):
				#print 'READY_CONNECTION_CHECK_PASSED_1'
				conn = Connector(line,handle)
				sink = ConnectionSink(glueitem,glueport)
				conn.connect_port(sink)
				self.ungrab_handle()
				#print 'READY_CONNECTION_CHECK_PASSED_2'
		except Exception as e:
			print 'Connection Failed, Disconnect/Connect the last Connection again: /n',e
		finally:
			self.toggle_highlight_ports()
			return super(BlockConnectTool, self).on_button_release(event)			
			
	def toggle_highlight_ports(self,portinstance=None):
		global SET_CONNECTION_FLAG
		if SET_CONNECTION_FLAG[0]:
			SET_CONNECTION_FLAG[0] = False
			SET_CONNECTION_FLAG[1] = None
		elif portinstance:
			SET_CONNECTION_FLAG[0] = True
			SET_CONNECTION_FLAG[1] = portinstance
		#request for an update to draw the ports again, coloured
		self.view.queue_draw_refresh()
		
@Connector.when_type(BlockLine)
class CanvasItemConnector(Connector.default):
	"""
	Decorator for the Connector Aspect, does the application-level connection too
	"""		
	def connect_port(self,sink):
		self.sink = sink
		canvas = self.item.canvas
		handle = self.handle
		item = self.item
		
		cinfo = item.canvas.get_connection(handle)
		if cinfo:
			self.disconnect()
		self.connect_handle(sink,callback=self.disconnect_port)
		self.post_connect()
		#print 'ASCEND_CONNECTION_DONE'
	
	def disconnect_port(self):
		self.post_disconnect()
			
	#@observed
	def post_connect(self):
		#print '~~Connected'
		handle = self.handle
		port = self.sink.port
		line = self.item
		assert(line.lineinstance is not None)
		assert(port.portinstance is not None)
		if handle is line._handles[0]:
			line.lineinstance.fromport = port.portinstance
		elif handle is line._handles[-1]:
			line.lineinstance.toport = port.portinstance
		else:
			raise RuntimeError("Invalid handle, neither start nor end")
	#@observed
	def post_disconnect(self):
		#print '~~Disconnected'
		handle = self.handle
		port = self.sink.port
		line = self.item
		assert(line.lineinstance is not None)
		if handle is line._handles[0]:
			line.lineinstance.fromport = None
		elif handle is line._handles[-1]:
			line.lineinstance.toport = None
			
	reversible_pair(post_connect,post_disconnect,bind1={},bind2={})
