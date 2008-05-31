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

    create_window(c, 'ASCEND model canvas')

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

    b=DefaultBlock(inputs=1,outputs=1)
    b.width = 100
    b.height = 100
    c.add(b)

    gtk.main()

if __name__ == '__main__':
    main()

# vim: sw=4:et:ai

