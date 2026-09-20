"""Gtk3/Gaphas 3 block-connection helpers."""

from __future__ import annotations

import math

from gtkcompat import gtk
from gaphas.connector import Connector

from blockline import BlockLine
from blockinstance import PORT_IN, PORT_OUT, PORT_INOUT


# Kept for the old drawing path in blockitem.py.
SET_CONNECTION_FLAG = [False, None]
HOVERED_PORT = [None]
HOVERED_ITEM = [None]
ACTIVE_SOURCE_PORT = [None]
ACTIVE_LINE = [None]


class BlockPortConnectionSink:
	"""Connection sink pinned to one ASCEND block port."""

	def __init__(self, item, port):
		self.item = item
		self.port = port

	def glue(self, pos, secondary_pos=None):
		point, _distance = self.port.glue(pos)
		return point

	def constraint(self, item, handle):
		return self.port.constraint(item, handle, self.item)


class PortDragState:
	def reset(self):
		self.line = None
		self.source_port = None


def port_connect_hover_tool(view):
	controller = gtk.EventControllerMotion.new(view)
	controller.connect("motion", on_motion)
	controller.connect("leave", on_leave)
	return controller


def port_connect_drag_tool(view):
	gesture = gtk.GestureDrag.new(view)
	state = PortDragState()
	state.reset()
	gesture.connect("drag-begin", on_drag_begin, state)
	gesture.connect("drag-update", on_drag_update, state)
	gesture.connect("drag-end", on_drag_end, state)
	return gesture


def on_motion(controller, x, y):
	view = controller.get_widget()
	_match = find_port_at_view_point(view, x, y, source_port=ACTIVE_SOURCE_PORT[0])
	hovered_item = _match[0] if _match else find_item_at_view_point(view, x, y)
	set_hovered_item(view, hovered_item)
	set_hovered_port(view, _match[1] if _match else None)


def on_leave(controller):
	set_hovered_item(controller.get_widget(), None)
	set_hovered_port(controller.get_widget(), None)


def on_drag_begin(gesture, start_x, start_y, state):
	view = gesture.get_widget()
	_match = find_port_at_view_point(view, start_x, start_y)
	if not _match:
		gesture.set_state(gtk.EventSequenceState.DENIED)
		return

	source_item, source_port = _match
	model = view.model
	line = BlockLine(model.connections)
	model.add(line)

	start_pos = view.get_matrix_v2i(line).transform_point(start_x, start_y)
	line.handles()[0].pos = start_pos
	line.handles()[-1].pos = start_pos
	Connector(line, line.handles()[0], model.connections).connect(
		BlockPortConnectionSink(source_item, source_port)
	)

	view.selection.unselect_all()
	view.selection.focused_item = line
	view.grab_focus()
	set_hovered_item(view, source_item)
	set_hovered_port(view, source_port)
	gesture.set_state(gtk.EventSequenceState.CLAIMED)

	state.line = line
	state.source_port = source_port
	ACTIVE_LINE[0] = line
	ACTIVE_SOURCE_PORT[0] = source_port
	model.request_update(line)


def on_drag_update(gesture, offset_x, offset_y, state):
	if state.line is None:
		return
	_, start_x, start_y = gesture.get_start_point()
	x = start_x + offset_x
	y = start_y + offset_y
	move_line_tail(gesture.get_widget(), state.line, x, y)
	target = find_port_at_view_point(gesture.get_widget(), x, y, source_port=state.source_port)
	set_hovered_item(gesture.get_widget(), target[0] if target else None)
	set_hovered_port(gesture.get_widget(), target[1] if target else None)


def on_drag_end(gesture, offset_x, offset_y, state):
	if state.line is None:
		return
	view = gesture.get_widget()
	model = view.model
	_, start_x, start_y = gesture.get_start_point()
	x = start_x + offset_x
	y = start_y + offset_y
	move_line_tail(view, state.line, x, y)

	target = find_port_at_view_point(view, x, y, source_port=state.source_port)
	if target:
		target_item, target_port = target
		Connector(state.line, state.line.handles()[-1], model.connections).connect(
			BlockPortConnectionSink(target_item, target_port)
		)

	if not model.connections.get_connection(state.line.handles()[-1]):
		model.remove(state.line)

	set_hovered_item(view, target[0] if target else None)
	set_hovered_port(view, target[1] if target else None)
	ACTIVE_LINE[0] = None
	ACTIVE_SOURCE_PORT[0] = None
	state.reset()


def move_line_tail(view, line, x, y):
	line.handles()[-1].pos = view.get_matrix_v2i(line).transform_point(x, y)
	view.model.request_update(line)


def set_hovered_port(view, port):
	if HOVERED_PORT[0] is port:
		return
	HOVERED_PORT[0] = port
	if view is not None:
		view.queue_draw()


def set_hovered_item(view, item):
	if HOVERED_ITEM[0] is item:
		return
	HOVERED_ITEM[0] = item
	if view is not None:
		view.queue_draw()


def find_item_at_view_point(view, x, y, distance=3):
	model = getattr(view, "model", None)
	if model is None:
		return None

	for item in reversed(list(model.get_all_items())):
		if not canvas_ports(item):
			continue
		v2i = view.get_matrix_v2i(item)
		ix, iy = v2i.transform_point(x, y)
		point = getattr(item, "point", None)
		if point is not None and point(ix, iy) <= distance:
			return item
	return None


def find_port_at_view_point(view, x, y, distance=8, source_port=None):
	model = getattr(view, "model", None)
	if model is None:
		return None

	best = None
	best_distance = distance
	for item in reversed(list(model.get_all_items())):
		ports = canvas_ports(item)
		if not ports:
			continue
		update_ports = getattr(item, "up1", None)
		if update_ports is not None:
			update_ports()
		i2v = view.get_matrix_i2v(item)
		for port in ports:
			if not getattr(port, "connectable", True):
				continue
			if source_port is not None and not ports_can_connect(source_port, port):
				continue
			px, py = i2v.transform_point(*port.point.pos)
			d = math.hypot(px - x, py - y)
			if d <= best_distance:
				best = (item, port)
				best_distance = d
	return best


def canvas_ports(item):
	return [
		port
		for port in getattr(item, "ports", lambda: ())()
		if hasattr(port, "point") and hasattr(port, "portinstance")
	]


def cancel_pending_connections(view):
	model = getattr(view, "model", None)
	if model is None:
		return

	for item in list(model.get_all_items()):
		if not isinstance(item, BlockLine):
			continue
		head, tail = item.handles()[0], item.handles()[-1]
		if not (
			model.connections.get_connection(head)
			and model.connections.get_connection(tail)
		):
			model.remove(item)

	ACTIVE_LINE[0] = None
	ACTIVE_SOURCE_PORT[0] = None
	set_hovered_item(view, None)
	set_hovered_port(view, None)


def ports_can_connect(source_port, target_port):
	if source_port is target_port:
		return False
	source = source_port.portinstance
	target = target_port.portinstance
	if str(source.type) != str(target.type):
		return False
	if source.io == PORT_OUT:
		return target.io in (PORT_IN, PORT_INOUT)
	if source.io == PORT_IN:
		return target.io in (PORT_OUT, PORT_INOUT)
	if source.io == PORT_INOUT:
		return target.io in (PORT_IN, PORT_OUT, PORT_INOUT)
	return False


class BlockConnectTool:
	def __init__(self, view=None):
		self.view = view

	def toggle_highlight_ports(self, portinstance=None):
		if SET_CONNECTION_FLAG[0]:
			SET_CONNECTION_FLAG[0] = False
			SET_CONNECTION_FLAG[1] = None
		elif portinstance:
			SET_CONNECTION_FLAG[0] = True
			SET_CONNECTION_FLAG[1] = portinstance
		if self.view is not None:
			self.view.queue_draw()
