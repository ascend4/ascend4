import pygtk
pygtk.require('2.0') 

import math
import gtk
import cairo

from gaphas.item import Item
from gaphas.state import observed, reversible_property, disable_dispatching
from gaphas.tool import HandleTool

from gaphas import GtkView, View
from gaphas.item import Line, SW, NE, NW, SE, Element, Handle
from gaphas.tool import Tool, HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import ItemTool, RubberbandTool
from gaphas.geometry import point_on_rectangle, distance_rectangle_point
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint
from gaphas.canvas import CanvasProjection
from gaphas.solver import Variable, solvable, WEAK, NORMAL, STRONG, VERY_STRONG

from gaphas.painter import ItemPainter
from gaphas import state
from gaphas.util import text_extents

from gaphas import painter

from blockcanvas import *
from panzoom import *
#painter.DEBUG_DRAW_BOUNDING_BOX = True

#------------------------------------------------------------------------------

class Port(object):
    """
    Ports are special places onto which Handles can be connected, specifically
    when users are drawing 'connector' lines between modelling 'blocks'.

    A subclass of Item called 'Block' will be given the ability to store
    an array of these Ports, in such a way that gluing methods will be able to
    select appropriate ports for a given connector line. The Ports also give
    data about where these available connected are located graphically, in the
    Block's local coordinate system.

    It is not intended that the number or location of ports would be editable
    once a port is instantiated. Only the connection of a port to a handle
    is editable by the user.

    It is intended that subclasses to the Port class would be created to provide
    finer control over whether or not a certain handle can be permitted to
    connect to any given port.

    Attributes:
    - block: Block to which Port belongs
    - connected_to: Handle to which port is connected, if any
    - x: port x-location in item's local coordinate system
    - y: port y-location in item's local coordinate system

    Private:
    - _x: port x-location in item's local coordinate system
    - _y: port y-location in item's local coordinate system
    - _connected_to: Handle to which port is connected, if any
    """

    _x = solvable()
    _y = solvable()

    def __init__(self, block, strength = STRONG):
        self.block = block
        self._x.strength = strength
        self._y.strength = strength
        self._connected_to = None

    @observed
    def _set_x(self, x):
        self._x = x

    x = reversible_property(lambda s: s._x, _set_x, bind={'x': lambda self: float(self.x) })
    disable_dispatching(_set_x)

    @observed
    def _set_y(self, y):
        self._y = y

    y = reversible_property(lambda s: s._y, _set_y, bind={'y': lambda self: float(self.y) })
    disable_dispatching(_set_y)

    @observed
    def _set_connected_to(self, connected_to):
        self._connected_to = connected_to

    connected_to = reversible_property(lambda s: s._connected_to,
                                       _set_connected_to)

    def draw(self, context):
        """
        Render the item to a canvas view.
        Context contains the following attributes:

        - cairo: the Cairo Context use this one to draw
        - view: the view that is to be rendered to
        - selected, focused, hovered, dropzone: view state of items (True/False)
        - draw_all: a request to draw everything, for bounding box calculation
        """
        pass

    def point(self, x, y):
        """
        Get the distance from a point (``x``, ``y``) to the item.
        ``x`` and ``y`` are in item coordinates.

        Defined here because a port is just a 'point' at this stage.
        """
        return math.sqrt((x-self.x)**2 + (y-self.y)**2)

    @observed
    def _set_pos(self, pos):
        """
        Set handle position (Item coordinates).
        """
        self.x, self.y = pos

    pos = property(lambda s: (s.x, s.y), _set_pos)

class PointConstraint(Constraint):
    """
    Ensure that point B is always kept on top of point A

    Attributes:
    _A: first point, defined by (x,y)
    _B: second point, defined by (x,y)
    """

    def __init__(self, A, B):
        print "A =",A
        print "A[0] =",A[0].variable()
        print "A[0].strength =",A[0].variable().strength

        print "B =",B
        print "B[0] =",B[0].variable()
        print "B[0].strength =",B[0].variable().strength

#        assert isinstance(p1[0],Variable)
#        assert isinstance(p1[1],Variable)
#        assert isinstance(p2[0],Variable)
#        assert isinstance(p2[1],Variable)

        super(PointConstraint, self).__init__(A[0],A[1],B[0],B[1])
        self._A = A
        self._B = B

    def solve_for(self, var=None):
        print "Solving PointConstraint",self,"for var",var

        _update(self._B[0], self._A[0].value)
        _update(self._B[1], self._A[1].value)

class Block(Element):
    """
    This is an ASCEND 'block' in the canvas-based modeller. The block will have
    sets of input and output ports to which connector lines can be 'glued'.
    The block will also have a corresponding ASCEND MODEL type, and a name
    which will be used in ASCEND to refer to this block. Each of the ports will
    be special visual elements, but note that these are not 'handles', because
    they can not be used to resize/modify the element.
    """

    def __init__(self, label="unnamed", width=64, height=64):

        self.ports = []
        self.label = label        
        super(Block, self).__init__(width, height)

    def draw(self, context):
        #print 'Box.draw', self
        c = context.cairo
        nw = self._handles[NW]
        c.rectangle(nw.x, nw.y, self.width, self.height)
        if context.hovered:
            c.set_source_rgba(.8,.8,1, .8)
        else:
            c.set_source_rgba(1,1,1, .8)
        c.fill_preserve()
        c.set_source_rgb(0,0,0.8)
        c.stroke()
    
        phalfsize = 3
        for p in self.ports:
            c.rectangle(p.x - phalfsize, p.y - phalfsize, 2*phalfsize, 2*phalfsize)
            if p.connected_to is None:
                c.set_source_rgba(0.8,0.8,1, 0.8)
            else:
                c.set_source_rgba(1,0,0,1)
            c.fill_preserve()
            c.set_source_rgb(0.8,0.8,0)
            c.stroke()

    def glue(self,item, handle, ix, iy):
        gluerange = 10
        mindist = -1;
        minport = None
        for p in self.ports:
            dist = math.sqrt((ix-p.x)**2 + (iy-p.y)**2)
            if dist < gluerange:
                if not minport or dist<mindist:
                    mindist = dist
                    minport = p
        return mindist, minport

    def pre_update(self,context):
        print "PRE-UPDATE BLOCK"

class DefaultBlock(Block):
    """
    This is a 'default block' with a certain number of input and output ports
    shown depending on the values sent to __init__. It is drawn as a simple
    box with the input ports on the left and the output ports on the right.
    """

    def __init__(self, label="unnamed", width=64, height=64, inputs=2, outputs=3):

        super(DefaultBlock, self).__init__(label, width, height)

        eq = EqualsConstraint
        bal = BalanceConstraint
        handles = self._handles
        h_nw = handles[NW]
        h_ne = handles[NE]
        h_sw = handles[SW]
        h_se = handles[SE]

        for i in range(inputs):
            p = Port(self)
            self.ports.append(p)
            self._constraints.append(eq(p.x, h_nw.x))
            self._constraints.append(bal(band=(h_nw.y, h_sw.y),v=p.y, balance=(0.5 + i)/inputs))

        for i in range(outputs):
            p = Port(self)
            self.ports.append(p)
            self._constraints.append(eq(p.x, h_ne.x))
            self._constraints.append(bal(band=(h_ne.y,h_se.y),v=p.y, balance=(0.5 + i)/outputs))

    def draw(self, context):
        # draw the box itself
        c = context.cairo
        nw = self._handles[NW]
        c.rectangle(nw.x, nw.y, self.width, self.height)
        if context.hovered:
            c.set_source_rgba(.8,.8,1, .8)
        else:
            c.set_source_rgba(1,1,1, .8)
        c.fill_preserve()
        c.set_source_rgb(0,0,0.8)
        c.stroke()

        # now the draw the ports using the base class
        super(DefaultBlock, self).draw(context)

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
        if not handle.connectable:
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
                print "i is item"
        if glue_point:
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

        # create a special local handle_disconnect function 
        def handle_disconnect():
            try:
                view.canvas.solver.remove_constraint(handle.connection_data)
            except KeyError:
                print 'constraint was already removed for', item, handle
                pass # constraint was alreasy removed
            else:
                print 'constraint removed for', item, handle
            handle.connected_port.connected_to = None
            handle.connection_data = None
            handle.connection_port = None
            handle.connected_to = None
            
            # Remove disconnect handler:
            handle.disconnect = lambda: 0

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
                print "Gluing to port",glue_port

                print "handle.pos =",handle.pos
                print "glue_port =",glue_port
                print "glue_port.pos = ",glue_port.pos
                print "glue_port.block =",glue_port.block

                handle.connection_data = PointConstraint(
                    B=CanvasProjection(handle.pos,item)
                    ,A=CanvasProjection(glue_port.pos, glue_port.block)
                )
                view.canvas.solver.add_constraint(handle.connection_data)
                #glue_port.block._constraints.append(handle.connection_data)

                handle.connected_to = glue_port.block
                handle.connected_port = glue_port
                handle.disconnect = handle_disconnect
                glue_port.connected_to = handle

    def disconnect(self, view, item, handle):
        if handle.connected_to:
            print 'Handle.disconnect', view, item, handle
            view.canvas.solver.remove_constraint(handle.connection_data)



# vim: sw=4:et:ai
