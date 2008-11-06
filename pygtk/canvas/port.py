import pygtk
pygtk.require('2.0') 

import math
import gtk
import cairo

from gaphas.item import Item
from gaphas.state import observed, reversible_property, disable_dispatching
from gaphas.tool import HandleTool

from gaphas import GtkView, View

from gaphas.solver import Variable, solvable, WEAK, NORMAL, STRONG, VERY_STRONG
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint
from gaphas.canvas import CanvasProjection

#------------------------------------------------------------------------------

class Port(object):
    """
    Ports are special places onto which Handles can be connected, specifically
    when users are drawing 'connector' lines between modelling 'blocks'.

    A subclass of Item called 'BlockItem' will be given the ability to store
    an array of these Ports, in such a way that gluing methods will be able to
    select appropriate ports for a given connector line. The Ports also give
    data about where these available connected are located graphically, in the
    BlockItem's local coordinate system.

    It is not intended that the number or location of ports would be editable
    once a port is instantiated. Only the connection of a port to a handle
    is editable by the user.

    It is intended that subclasses to the Port class would be created to provide
    finer control over whether or not a certain handle can be permitted to
    connect to any given port.

    Attributes:
    - block: BlockItem to which Port belongs
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
        #print "A =",A
        #print "A[0] =",A[0].variable()
        #print "A[0].strength =",A[0].variable().strength

        #print "B =",B
        #print "B[0] =",B[0].variable()
        #print "B[0].strength =",B[0].variable().strength

#        assert isinstance(p1[0],Variable)
#        assert isinstance(p1[1],Variable)
#        assert isinstance(p2[0],Variable)
#        assert isinstance(p2[1],Variable)

        super(PointConstraint, self).__init__(A[0],A[1],B[0],B[1])
        self._A = A
        self._B = B

    def solve_for(self, var=None):
        #print "Solving PointConstraint",self,"for var",var

        _update(self._B[0], self._A[0].value)
        _update(self._B[1], self._A[1].value)

# vim: sw=4:et:ai
