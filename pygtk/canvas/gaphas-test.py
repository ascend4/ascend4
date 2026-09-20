#!/usr/bin/env python3
"""Minimal Gaphas 3/Gtk3 smoke window for the canvas model."""

import inspect

from gtkcompat import gtk
from gaphas.tool import hover_tool, item_tool, scroll_tools, zoom_tools
from gaphas.view import GtkView

from blockcanvas import BlockCanvas


def _gaphas_tool(factory, view, *args):
	params = list(inspect.signature(factory).parameters)
	if params and params[0] == "view":
		return factory(view, *args)
	return factory(*args)


def create_window(canvas, title, zoom=1.0):
	view = GtkView(canvas)
	view.canvas = canvas
	view.add_controller(
		_gaphas_tool(hover_tool, view),
		_gaphas_tool(item_tool, view),
		*_gaphas_tool(scroll_tools, view),
		*_gaphas_tool(zoom_tools, view),
	)
	view.zoom(zoom)
	view.set_size_request(800, 600)

	window = gtk.Window()
	window.set_title(title)
	window.connect("destroy", gtk.main_quit)

	table = gtk.Table(2, 2)
	hs = gtk.HScrollbar(view.hadjustment)
	vs = gtk.VScrollbar(view.vadjustment)
	table.attach(view, 0, 1, 0, 1)
	table.attach(hs, 0, 1, 1, 2, xoptions=gtk.FILL, yoptions=gtk.FILL)
	table.attach(vs, 1, 2, 0, 1, xoptions=gtk.FILL, yoptions=gtk.FILL)
	window.add(table)
	window.show_all()
	return window


def main():
	create_window(BlockCanvas(), "ASCEND model canvas")
	gtk.main()


if __name__ == "__main__":
	main()
