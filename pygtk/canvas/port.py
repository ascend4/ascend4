import pygtk
pygtk.require('2.0') 

import math
import gtk
import cairo

from gaphas.item import Item
from gaphas.state import observed, reversible_property
from gaphas.tool import HandleTool

from gaphas import Canvas, GtkView, View
from gaphas.item import Line, SW, NE, NW, SE, Element, Handle
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import ItemTool, RubberbandTool
from gaphas.geometry import point_on_rectangle, distance_rectangle_point
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint
from gaphas.canvas import CanvasProjection
from gaphas.solver import Variable

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

    def __init__(self, block, x, y):
        self.block = block
        self._connectable = True
        self.x = x
        self.y = y

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
#        assert isinstance(p1[0],Variable)
#        assert isinstance(p1[1],Variable)
#        assert isinstance(p2[0],Variable)
#        assert isinstance(p2[1],Variable)

        super(PointConstraint, self).__init__(p1[0].variable(),p1[1].variable(),p2[0].variable(),p2[1].variable())
        self.p1 = p1
        self.p2 = p2

    def solve_for(self, var):
        assert var in (self.p1, self.p2)

        if var is self.p1:
            _update(self.p1[0], self.p2[0].value)
            _update(self.p1[1], self.p2[1].value)
        else:
            _update(self.p2[0], self.p1[0].value)
            _update(self.p2[1], self.p1[1].value)    

class Block(Element):
    """
    This is an ASCEND 'block' in the canvas-based modeller. The block will have
    sets of input and output ports to which connector lines can be 'glued'.
    The block will also have a corresponding ASCEND MODEL type, and a name
    which will be used in ASCEND to refer to this block. Each of the ports will
    be special visual elements, but note that these are not 'handles', because
    they can not be used to resize/modify the element.
    """

    def __init__(self, label="unnamed", width=10, height=10, ports=None):

        if ports is None:
            ports = []

        self.ports = ports
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
            c.set_source_rgba(0.8,0.8,1, 0.8)
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

    def __init__(self, label="unnamed", width=10, height=10, inputs=2, outputs=2):

        ports = []
        for i in range(inputs):
            ports.append(Port(self,0,(0.5 + i) * (height/inputs)))
        for i in range(outputs):
            ports.append(Port(self,width,(0.5 + i) * (height/outputs)))
            
        super(DefaultBlock, self).__init__(label,width, height, ports)

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
    This is a HandleTool which supports a simple connection algorithm,
    using LineConstraint.
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
        for i in view.canvas.get_all_items():
            if not i is item:
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

        def handle_disconnect():
            try:
                view.canvas.solver.remove_constraint(handle._connect_constraint)
            except KeyError:
                print 'constraint was already removed for', item, handle
                pass # constraint was alreasy removed
            else:
                print 'constraint removed for', item, handle
            handle._connect_constraint = None
            handle.connected_to = None
            # Remove disconnect handler:
            handle.disconnect = lambda: 0

        #print 'Handle.connect', view, item, handle, wx, wy
        glue_port = self.glue(view, item, handle, wx, wy)

        if glue_port and glue_port is handle.connected_to:
            try:
                view.canvas.solver.remove_constraint(handle._connect_constraint)
            except KeyError:
                pass # constraint was already removed

                handle._connect_constraint = PointConstraint(
                    p1=CanvasProjection(handle.pos,item)
                    ,p2=CanvasProjection(glue_port.pos, glue_port.block)
                )                
                view.canvas.solver.add_constraint(handle._connect_constraint)
                handle.connected_to = glue_port
                handle.disconnect = handle_disconnect

            handle.disconnect = handle_disconnect
            return

        # drop old connection
        if handle.connected_to:
            handle.disconnect()

        if glue_port:
            if isinstance(glue_port, Port):
                print "Gluing to port",glue_port
                handle._connect_constraint = PointConstraint(
                    p1=CanvasProjection(handle.pos,item)
                    ,p2=CanvasProjection(glue_port.pos, glue_port.block)
                )
                view.canvas.solver.add_constraint(handle._connect_constraint)
                handle.connected_to = glue_port
                handle.disconnect = handle_disconnect

    def disconnect(self, view, item, handle):
        if handle.connected_to:
            #print 'Handle.disconnect', view, item, handle
            view.canvas.solver.remove_constraint(handle._connect_constraint)


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




from gaphas.view import View
import pygtk
pygtk.require('2.0') 

import math
import gtk


def create_window(canvas, zoom=1.0):
    view = View()
    
    w = gtk.Window()
    h = gtk.HBox()
    w.add(h)

    # VBox contains buttons that can be used to manipulate the canvas:
    v = gtk.VBox()
    v.set_property('border-width', 3)
    v.set_property('spacing', 2)
    f = gtk.Frame()
    f.set_property('border-width', 1)
    f.add(v)
    h.pack_start(f, expand=False)

    v.add(gtk.Label('Item placement:'))
    
    b = gtk.Button('Add box')

    def on_clicked(button, view):
        #view.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.CROSSHAIR))
        view.tool.grab(PlacementTool(MyBox, HandleTool(), 2))

    b.connect('clicked', on_clicked, view)
    v.add(b)

    b = gtk.Button('Add line')

    def on_clicked(button):
        view.tool.grab(PlacementTool(MyLine, HandleTool(), 1))

    b.connect('clicked', on_clicked)
    v.add(b)

    v.add(gtk.Label('Zooming:'))
   
    b = gtk.Button('Zoom in')

    def on_clicked(button):
        view.zoom(1.2)

    b.connect('clicked', on_clicked)
    v.add(b)

    b = gtk.Button('Zoom out')

    def on_clicked(button):
        view.zoom(1/1.2)

    b.connect('clicked', on_clicked)
    v.add(b)

    v.add(gtk.Label('Misc:'))

    b = gtk.Button('Split line')

    def on_clicked(button):
        if isinstance(view.focused_item, Line):
            view.focused_item.split_segment(0)
            view.queue_draw_item(view.focused_item, handles=True)

    b.connect('clicked', on_clicked)
    v.add(b)

#    b = gtk.Button('Cursor')
#
#    def on_clicked(button, li):
#        c = li[0]
#        li[0] = (c+2) % 154
#        button.set_label('Cursor %d' % c)
#        button.window.set_cursor(gtk.gdk.Cursor(c))
#
#    b.connect('clicked', on_clicked, [0])
#    v.add(b)

    # Add the actual View:

    t = gtk.Table(2,2)
    h.add(t)

    w.connect('destroy', gtk.main_quit)

    view.canvas = canvas
    view.zoom(zoom)
    view.set_size_request(150, 120)
    hs = gtk.HScrollbar(view.hadjustment)
    vs = gtk.VScrollbar(view.vadjustment)
    t.attach(view, 0, 1, 0, 1)
    t.attach(hs, 0, 1, 1, 2, xoptions=gtk.FILL, yoptions=gtk.FILL)
    t.attach(vs, 1, 2, 0, 1, xoptions=gtk.FILL, yoptions=gtk.FILL)

    w.show_all()
    
    def handle_changed(view, item, what):
        print what, 'changed: ', item

    view.connect('focus-changed', handle_changed, 'focus')
    view.connect('hover-changed', handle_changed, 'hover')
    view.connect('selection-changed', handle_changed, 'selection')



if __name__=="__main__":
    from gaphas.canvas import Canvas
    canvas = Canvas()

    create_window(canvas)
    


# vim: sw=4:et:ai
