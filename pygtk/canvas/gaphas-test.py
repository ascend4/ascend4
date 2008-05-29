import pygtk
pygtk.require('2.0') 

import math
import gtk
import cairo
from gaphas import Canvas, GtkView, View
from gaphas.item import Line, SW, NE, NW, SE, Element, Handle
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import ItemTool, RubberbandTool
from gaphas.geometry import point_on_rectangle, distance_rectangle_point
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint
from gaphas.canvas import CanvasProjection
from gaphas.examples import Box

from gaphas.painter import ItemPainter
from gaphas import state
from gaphas.util import text_extents

from gaphas import painter
#painter.DEBUG_DRAW_BOUNDING_BOX = True

from port import *

class ConnectingHandleTool(HandleTool):
    """
    This is a HandleTool which supports a simple connection algorithm,
    using LineConstraint.
    """

    def glue(self, view, item, handle, wx, wy):
        """
        It allows the tool to glue to a Box or (other) Line item.
        The distance from the item to the handle is determined in canvas
        coordinates, using a 10 pixel glue distance.
        """
        if not handle.connectable:
            return

        # Make glue distance depend on the zoom ratio (should be about 10 pixels)
        inverse = cairo.Matrix(*view.matrix)
        inverse.invert()
        #glue_distance, dummy = inverse.transform_distance(10, 0)
        glue_distance = 10
        glue_point = None
        glue_item = None
        for i in view.canvas.get_all_items():
            if not i is item:
                v2i = view.get_matrix_v2i(i).transform_point
                ix, iy = v2i(wx, wy)
                try:
                    distance, point = i.glue(item, handle, ix, iy)
                    # Transform distance to world coordinates
                    #distance, dumy = matrix_i2w(i).transform_distance(distance, 0)
                    if distance <= glue_distance:
                        glue_distance = distance
                        i2v = view.get_matrix_i2v(i).transform_point
                        glue_point = i2v(*point)
                        glue_item = i
                except AttributeError:
                    pass
        if glue_point:
            v2i = view.get_matrix_v2i(item).transform_point
            handle.x, handle.y = v2i(*glue_point)
        return glue_item

    def connect(self, view, item, handle, wx, wy):
        """
        Connect a handle to another item.

        In this "method" the following assumptios are made:
         1. The only item that accepts handle connections are the Box instances
         2. The only items with connectable handles are Line's
         
        """
        def side(handle, glued):
            handles = glued.handles()
            hx, hy = view.get_matrix_i2v(item).transform_point(handle.x, handle.y)
            ax, ay = view.get_matrix_i2v(glued).transform_point(handles[NW].x, handles[NW].y)
            bx, by = view.get_matrix_i2v(glued).transform_point(handles[SE].x, handles[SE].y)

            if abs(hx - ax) < 0.01:
                return handles[NW], handles[SW]
            elif abs(hy - ay) < 0.01:
                return handles[NW], handles[NE]
            elif abs(hx - bx) < 0.01:
                return handles[NE], handles[SE]
            else:
                return handles[SW], handles[SE]
            assert False


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
        glue_item = self.glue(view, item, handle, wx, wy)
        if glue_item and glue_item is handle.connected_to:
            try:
                view.canvas.solver.remove_constraint(handle._connect_constraint)
            except KeyError:
                pass # constraint was already removed

            h1, h2 = side(handle, glue_item)
            handle._connect_constraint = LineConstraint(line=(CanvasProjection(h1.pos, glue_item),
                                      CanvasProjection(h2.pos, glue_item)),
                                point=CanvasProjection(handle.pos, item))
            view.canvas.solver.add_constraint(handle._connect_constraint)

            handle.disconnect = handle_disconnect
            return

        # drop old connetion
        if handle.connected_to:
            handle.disconnect()

        if glue_item:
            if isinstance(glue_item, Box):
                h1, h2 = side(handle, glue_item)

                # Make a constraint that keeps into account item coordinates.
                handle._connect_constraint = \
                        LineConstraint(line=(CanvasProjection(h1.pos, glue_item),
                            CanvasProjection(h2.pos, glue_item)),
                            point=CanvasProjection(handle.pos, item))
                view.canvas.solver.add_constraint(handle._connect_constraint)

                handle.connected_to = glue_item
                handle.disconnect = handle_disconnect

    def disconnect(self, view, item, handle):
        if handle.connected_to:
            #print 'Handle.disconnect', view, item, handle
            view.canvas.solver.remove_constraint(handle._connect_constraint)


def factory(view, cls):
    """
    Simple canvas item factory.
    """
    def wrapper():
        item = cls()
        view.canvas.add(item)
        return item
    return wrapper


def create_window(canvas, title, zoom=1.0):
    view = GtkView()
    view.tool = DefaultExampleTool()

    w = gtk.Window()
    w.set_title(title)
    #w.set_default_size(800,600)
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
    
    b = gtk.Button('Add box')

    def on_clicked(button, view):
        #view.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.CROSSHAIR))
        view.tool.grab(PlacementTool(factory(view, Box), HandleTool(), 2))

    b.connect('clicked', on_clicked, view)
    v.add(b)

    b = gtk.Button('Add block')

    def on_clicked(button, view):
        #view.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.CROSSHAIR))
        view.tool.grab(PlacementTool(factory(view, DefaultBlock), HandleTool(), 2))

    b.connect('clicked', on_clicked, view)
    v.add(b)

    b = gtk.Button('Add line')

    def on_clicked(button):
        view.tool.grab(PlacementTool(factory(view, Line), HandleTool(), 1))

    b.connect('clicked', on_clicked)
    v.add(b)


    b = gtk.Button('Delete focused')

    def on_clicked(button):
        if view.focused_item:
            canvas.remove(view.focused_item)
            print 'items:', canvas.get_all_items()

    b.connect('clicked', on_clicked)
    v.add(b)

    # Add the actual View:

    t = gtk.Table(2,2)
    h.add(t)

    w.connect('destroy', gtk.main_quit)

    view.canvas = canvas
    view.zoom(zoom)
    view.set_size_request(800, 600)
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

def main():
    c=Canvas()

    create_window(c, 'View created before')

    # Add stuff to the canvas:

    b=Box()
    b.min_width = 20
    b.min_height = 30
    print 'box', b
    b.matrix=(1.0, 0.0, 0.0, 1, 20,20)
    b.width = b.height = 40
    c.add(b)

    bb=Box()
    print 'box', bb
    bb.matrix=(1.0, 0.0, 0.0, 1, 10,10)
    c.add(bb, parent=b)

    # AJM: extra boxes:
    bb=Box()
    print 'box', bb
    bb.matrix.rotate(math.pi/4.)
    c.add(bb, parent=b)

    l=Line()
    l.handles()[1].pos = (30, 30)
    l.split_segment(0, 3)
    l.matrix.translate(30, 60)
    c.add(l)
    l.orthogonal = True

    gtk.main()

if __name__ == '__main__':
    main()

# vim: sw=4:et:ai

