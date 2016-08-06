# -*- coding: utf-8 -*-
import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk
from gaphas import GtkView
from gaphas.tool import PlacementTool, HandleTool
from gaphas.tool import DefaultTool
from blockcanvas import BlockCanvas
from blockitem  import DefaultBlockItem


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
    view.tool = DefaultTool()

    w = Gtk.Window()
    w.set_title(title)
    #w.set_default_size(800,600)
    h = Gtk.HBox()
    w.add(h)

    # VBox contains buttons that can be used to manipulate the canvas:
    v = Gtk.VBox()
    v.set_property('border-width', 3)
    v.set_property('spacing', 2)
    f = Gtk.Frame()
    f.set_property('border-width', 1)
    f.add(v)
    h.pack_start(f, False, True, 0)
    
    b = Gtk.Button('Add block')

    def on_clicked(button, view):
        #view.window.set_cursor(Gdk.Cursor.new(Gdk.CursorType.CROSSHAIR))
        view.tool.grab(PlacementTool(factory(view, DefaultBlockItem), HandleTool(), 2))

    b.connect('clicked', on_clicked, view)
    v.add(b)

    b = Gtk.Button('Add line')

    def on_clicked(button):
        view.tool.grab(PlacementTool(factory(view, Line), HandleTool(), 1))

    b.connect('clicked', on_clicked)
    v.add(b)

    b = Gtk.Button('Delete focused')

    def on_clicked(button):
        if view.focused_item:
            canvas.remove(view.focused_item)
            print 'items:', canvas.get_all_items()

    b.connect('clicked', on_clicked)
    v.add(b)

    # Add the actual View:

    t = Gtk.Table(2, 2)
    h.add(t)

    w.connect('destroy', Gtk.main_quit)

    view.canvas = canvas
    view.zoom(zoom)
    view.set_size_request(800, 600)
    hs = Gtk.HScrollbar(view.hadjustment)
    vs = Gtk.VScrollbar(view.vadjustment)
    t.attach(view, 0, 1, 0, 1)
    t.attach(hs, 0, 1, 1, 2, xoptions=Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.FILL)
    t.attach(vs, 1, 2, 0, 1, xoptions=Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.FILL)

    w.show_all()
    
    def handle_changed(view, item, what):
        print what, 'changed: ', item

    view.connect('focus-changed', handle_changed, 'focus')
    view.connect('hover-changed', handle_changed, 'hover')
    view.connect('selection-changed', handle_changed, 'selection')


def main():
    c=BlockCanvas()

    create_window(c, 'ASCEND model canvas')

    # Add stuff to the canvas:

    b=DefaultBlock(inputs=1,outputs=1)
    b.width = 100
    b.height = 100
    c.add(b)
    
    # run
    Gtk.main()

if __name__ == '__main__':
    main()

# vim: sw=4:et:ai

