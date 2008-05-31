import pygtk
pygtk.require('2.0') 

import math
import gtk
import cairo

from gaphas.item import Item
from gaphas.state import observed, reversible_property, disable_dispatching
from gaphas.tool import HandleTool

from gaphas import Canvas, GtkView, View
from gaphas.item import Line, SW, NE, NW, SE, Element, Handle
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import ItemTool, RubberbandTool
from gaphas.geometry import point_on_rectangle, distance_rectangle_point
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint
from gaphas.canvas import CanvasProjection
from gaphas.solver import Variable, solvable, WEAK, NORMAL, STRONG, VERY_STRONG

from gaphas.painter import ItemPainter
from gaphas import state
from gaphas.util import text_extents

from gaphas import painter
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

    We don't include a 'connected_to' attribute in this class because the
    Handle objects belonging to the Line can keep track of those connections.

    Attributes:
    - block: Block to which Port belongs
    - connectable: whether or not handles can currently be connected to this port

    Private:
    - _x: port x-location in item's local coordinate system
    - _y: port y-location in item's local coordinate system
    - _connectable
    
    """

    _x = solvable()
    _y = solvable()

    def __init__(self, block, strength = STRONG):
        self.block = block
        self._x.strength = strength
        self._y.strength = strength
        self._connectable = True

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
    def _set_connectable(self, connectable):
        """
            A port must be set to be unconnectable if it already has
            something connected to it.
        """
        self._connectable = connectable

    connectable = reversible_property(lambda s: s._connectable, _set_connectable)

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
    Ensure two points are kept together

    Attributes:
    _p1: first point, defined by (x,y)
    _p2: second point, defined by (x,y)
    """

    def __init__(self, p1, p2):
        print "p1 =",p1
        print "p1[0] =",p1[0].variable()
        print "p1[0].strength =",p1[0].variable().strength

        print "p2 =",p2
        print "p2[0] =",p2[0].variable()
        print "p2[0].strength =",p2[0].variable().strength

#        assert isinstance(p1[0],Variable)
#        assert isinstance(p1[1],Variable)
#        assert isinstance(p2[0],Variable)
#        assert isinstance(p2[1],Variable)

        super(PointConstraint, self).__init__(p1[0],p1[1],p2[0],p2[1])
        self.p1 = p1
        self.p2 = p2

    def solve_for(self, var):
        print "Solving PointConstraint..."
        match = {0:2,1:3,2:0,3:1}
        for k in match:
            if var is self._variables[k]:
                print "Updating variable %d to equal value of variable %d" % (k,match[k])
                _update(self._variables[match[k]], self._variables[k].value)
                return
        raise AssertionError("wrong variable in solve_for")

class Block(Element):
    """
    This is an ASCEND 'block' in the canvas-based modeller. The block will have
    sets of input and output ports to which connector lines can be 'glued'.
    The block will also have a corresponding ASCEND MODEL type, and a name
    which will be used in ASCEND to refer to this block. Each of the ports will
    be special visual elements, but note that these are not 'handles', because
    they can not be used to resize/modify the element.
    """

    def __init__(self, label="unnamed", width=10, height=10):

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
            if p.connectable:
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

class DefaultBlock(Block):
    """
    This is a 'default block' with a certain number of input and output ports
    shown depending on the values sent to __init__. It is drawn as a simple
    box with the input ports on the left and the output ports on the right.
    """

    def __init__(self, label="unnamed", width=10, height=10, inputs=2, outputs=3):

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
        print "Gluing..."
        for i in view.canvas.get_all_items():
            if not hasattr(i,'ports'):
                continue
            if not i is item:
                print "Trying glue to",i
                v2i = view.get_matrix_v2i(i).transform_point
                ix, iy = v2i(wx, wy)
                try:
                    distance, port = i.glue(item, handle, ix, iy)
                    # Transform distance to world coordinates
                    #distance, dumy = matrix_i2w(i).transform_distance(distance, 0)
                    if distance <= glue_distance:
                        glue_distance = distance
                        i2v = view.get_matrix_i2v(i).transform_point
                        glue_point = i2v(port.x, port.y)
                        glue_port = port
                except AttributeError:
                    pass
        if glue_point:
            v2i = view.get_matrix_v2i(item).transform_point
            handle.x, handle.y = v2i(*glue_point)
            print "Found glue point ",handle.x,handle.y 
        return glue_port

    def connect(self, view, item, handle, wx, wy):
        """
        Connect a handle to another item.

        In this "method" the following assumptios are made:
         1. The only item that accepts handle connections are the Box instances
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
            handle.connected_to.connectable = True
            handle.connection_data = None
            handle.connected_to = None
            
            # Remove disconnect handler:
            handle.disconnect = lambda: 0

        #print 'Handle.connect', view, item, handle, wx, wy
        glue_port = self.glue(view, item, handle, wx, wy)

        if glue_port:
            if glue_port is handle.connected_to:    
                hand

        if glue_port and glue_port is handle.connected_to:
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
                p2 = CanvasProjection(glue_port.pos, glue_port.block)     
                print "connect: p2 =",p2
                print "connect: p2[0] =",p2[0].variable()
                print "connect: p2[0].strength =",p2[0].variable().strength

                handle.connection_data = PointConstraint(
                    p1=CanvasProjection(handle.pos,item)
                    ,p2=p2
                )
                view.canvas.solver.add_constraint(handle.connection_data)
                handle.connected_to = glue_port
                handle.disconnect = handle_disconnect
                glue_port.connectable = False

    def disconnect(self, view, item, handle):
        if handle.connected_to:
            print 'Handle.disconnect', view, item, handle
            view.canvas.solver.remove_constraint(handle.connection_data)


def DefaultExampleTool():
    """
    The default tool chain build from HoverTool, ItemTool and HandleTool.
    """
    chain = ToolChain()
    chain.append(HoverTool())
    chain.append(PortConnectingHandleTool())
    chain.append(ItemTool())
    chain.append(RubberbandTool())
    return chain

# vim: sw=4:et:ai
