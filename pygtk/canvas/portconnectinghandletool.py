import cairo
from gaphas.tool import HandleTool
from port import *

class PortDisconnector:
	"""
	A special Handle.disconnect function that knows when it's connected
	to a Port, and treats them specially.
	"""

	def __init__(self, canvas, handle):
		self.canvas = canvas
		self.handle = handle

	def __call__(self):
		try:
			self.canvas.solver.remove_constraint(self.handle.connection_data)
		except KeyError:
			print 'constraint was already removed for', self.handle
			pass # constraint was alreasy removed
		else:
			print 'constraint removed for', self.handle
		self.handle.connected_port.connected_to = None
		self.handle.connection_data = None
		self.handle.connection_port = None
		self.handle.connected_to = None

		# Remove disconnect handler:
		self.handle.disconnect = lambda: 0


class PortConnectingHandleTool(HandleTool):
    """
    This is a HandleTool which supports the connection of lines to the Ports
    of Blocks, for the purpose of building up process flow diagrams, control
    diagrams, etc, for the proposed canvas-based modeller of ASCEND. 
    """

    def glue(self, view, item, handle, wx, wy):
        """
        This allows the tool to glue a handle to any connectable Port of a Block.
        The distance from the item to the handle is determined in canvas
        coordinates, using a 10 pixel glue distance.

        Returns the closest Port that is within the glue distance. 
        """
        if handle and not handle.connectable:
            return

        # Make glue distance depend on the zoom ratio (should be about 10 pixels)
        inverse = cairo.Matrix(*view.matrix)
        inverse.invert()
        #glue_distance, dummy = inverse.transform_distance(10, 0)
        glue_distance = 10
        glue_port = None
        glue_point = None
        #print "Gluing..."   
        for i in view.canvas.get_all_items():
            if not hasattr(i,'ports'):
                continue
            if not i is item:
                #print "Trying glue to",i
                v2i = view.get_matrix_v2i(i).transform_point
                ix, iy = v2i(wx, wy)
                distance, port = i.glue(item, handle, ix, iy)
                # Transform distance to world coordinates
                #distance, dumy = matrix_i2w(i).transform_distance(distance, 0)
                if not port is None and distance <= glue_distance:
                    glue_distance = distance
                    i2v = view.get_matrix_i2v(i).transform_point
                    glue_point = i2v(port.x, port.y)
                    glue_port = port
            else:
				pass
                #print "i is item"
        if glue_point and handle and item:
            v2i = view.get_matrix_v2i(item).transform_point
            handle.x, handle.y = v2i(*glue_point)
            #print "Found glue point ",handle.x,handle.y 
        return glue_port

    def connect(self, view, item, handle, wx, wy):
        """
        Connect a handle to a port. 'item' is the line to which the handle
        belongs; wx and wy are the location of the cursor, so we run the 'glue'
        routine to find the desired gluing point, then make the connection to
        the object which 'glue' returns, which will be a Port object (in the
        context of this tool).

        In this "method" the following assumptios are made:
         1. Only ``Port``s of ``Block``s will accept connections from handles.
         2. The only items with connectable handles are ``Line``s
         
        """
	
        #print 'Handle.connect', view, item, handle, wx, wy
        glue_port = self.glue(view, item, handle, wx, wy)

        if glue_port and hasattr(handle,'connected_port') and handle.connected_port is glue_port:
            try:
                view.canvas.solver.remove_constraint(handle.connection_data)
            except KeyError:
                pass
        else:
            # ie no glue_port found, or the handle connected to something else
            if handle.connected_to:
                handle.disconnect()

        if glue_port:
            if isinstance(glue_port, Port):
                #print "Gluing to port",glue_port

                #print "handle.pos =",handle.pos
                #print "glue_port =",glue_port
                #print "glue_port.pos = ",glue_port.pos
                #print "glue_port.block =",glue_port.block

                handle.connection_data = PointConstraint(
                    B=CanvasProjection(handle.pos,item)
                    ,A=CanvasProjection(glue_port.pos, glue_port.block)
                )
                view.canvas.solver.add_constraint(handle.connection_data)
                #glue_port.block._constraints.append(handle.connection_data)

                handle.connected_to = glue_port.block
                handle.connected_port = glue_port
                handle.disconnect = PortDisconnector(view.canvas, handle)
                glue_port.connected_to = handle

    def disconnect(self, view, item, handle):
        if handle.connected_to:
            print 'Handle.disconnect', view, item, handle
            view.canvas.solver.remove_constraint(handle.connection_data)

