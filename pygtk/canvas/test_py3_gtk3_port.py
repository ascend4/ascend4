#!/usr/bin/env python3
"""Static checks for the Python 3/Gtk3 canvas port."""

from __future__ import annotations

import py_compile
import importlib
import os
import re
import sys
import unittest
from pathlib import Path


CANVAS_DIR = Path(__file__).resolve().parent
PYGTK_DIR = CANVAS_DIR.parent
REPO_ROOT = PYGTK_DIR.parent
ASCXX_DIR = REPO_ROOT / "ascxx"


class CanvasPortingTests(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		for path in (CANVAS_DIR, PYGTK_DIR, ASCXX_DIR):
			path = str(path)
			if path not in sys.path:
				sys.path.insert(0, path)

	def python_files(self):
		return sorted(CANVAS_DIR.glob("*.py"))

	def import_canvas_module(self, name):
		try:
			return importlib.import_module(name)
		except ImportError as e:
			if "libascend" in str(e) or name == "ascpy":
				self.skipTest("ASCEND shared library is not available to this Python process")
			raise

	def require_ascend_runtime(self):
		try:
			importlib.import_module("ascpy")
		except ImportError as e:
			if "libascend" in str(e):
				self.skipTest("ASCEND shared library is not available to this Python process")
			raise

	def test_canvas_python_files_compile(self):
		for path in self.python_files():
			with self.subTest(path=path.name):
				py_compile.compile(str(path), doraise=True)

	def test_canvas_entry_import_preserves_process_environment(self):
		before_cwd = os.getcwd()
		before_paths = {
			name: os.environ.get(name)
			for name in ("ASCENDLIBRARY", "ASCENDSOLVERS", "LD_LIBRARY_PATH")
		}
		module = self.import_canvas_module("canvas")
		importlib.reload(module)
		self.assertEqual(before_cwd, os.getcwd())
		self.assertEqual(
			before_paths,
			{
				name: os.environ.get(name)
				for name in ("ASCENDLIBRARY", "ASCENDSOLVERS", "LD_LIBRARY_PATH")
			},
		)

	def test_removed_legacy_imports_do_not_return(self):
		legacy_patterns = [
			re.compile(r"^\s*import\s+gtk\b", re.MULTILINE),
			re.compile(r"^\s*import\s+pygtk\b", re.MULTILINE),
			re.compile(r"pygtk\.require"),
			re.compile(r"gtksourceview2"),
			re.compile(r"^\s*from\s+gaphas\.state|^\s*import\s+gaphas\.state", re.MULTILINE),
			re.compile(r"from\s+gaphas\.tool\s+import\s+[A-Z]"),
			re.compile(r"=\s*file\("),
		]
		for path in self.python_files():
			if path.name == "test_py3_gtk3_port.py":
				continue
			text = path.read_text()
			for pattern in legacy_patterns:
				with self.subTest(path=path.name, pattern=pattern.pattern):
					self.assertIsNone(pattern.search(text))

	def test_gtkcompat_box_pack_start_accepts_legacy_arguments(self):
		gtkcompat = self.import_canvas_module("gtkcompat")

		box = gtkcompat.gtk.VBox()
		label = gtkcompat.gtk.Label(label="compat")

		box.pack_start(label, False, False)

		self.assertIn(label, box.get_children())

	def test_gtkcompat_exposes_legacy_pango_weight_aliases(self):
		gtkcompat = self.import_canvas_module("gtkcompat")

		self.assertEqual(gtkcompat.pango.Weight.NORMAL, gtkcompat.pango.WEIGHT_NORMAL)
		self.assertEqual(gtkcompat.pango.Weight.BOLD, gtkcompat.pango.WEIGHT_BOLD)

	def test_blockcanvas_update_now_matches_gaphas_model_protocol(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockline = self.import_canvas_module("blockline")

		canvas = blockcanvas.BlockCanvas()
		line = blockline.BlockLine(canvas.connections)
		canvas.add(line)

		canvas.update_now([line])
		canvas.update_now()
		canvas.update_constraints([line])

	def test_canvas_modules_import_with_ascend_runtime(self):
		self.require_ascend_runtime()
		for path in self.python_files():
			if path.name in {"gaphas-test.py", "test_py3_gtk3_port.py"}:
				continue
			with self.subTest(path=path.name):
				self.import_canvas_module(path.stem)

	def test_blockline_can_share_canvas_connections(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockline = self.import_canvas_module("blockline")

		canvas = blockcanvas.BlockCanvas()
		line = blockline.BlockLine(canvas.connections)

		self.assertIs(line._connections, canvas.connections)

	def test_blockline_has_wide_selection_hit_region_and_coloured_endpoint_roles(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockline = self.import_canvas_module("blockline")

		class PortInstance:
			def __init__(self, io):
				self.io = io

		line = blockline.BlockLine(blockcanvas.BlockCanvas().connections)
		line.handles()[-1].pos = (100, 0)

		self.assertEqual(8.0, line.fuzziness)
		self.assertEqual(0.0, line.point(50, 7))
		self.assertFalse(line.handles()[0].visible)
		self.assertFalse(line.handles()[-1].visible)
		self.assertEqual((1.0, 0.05, 0.0), blockline.portinstance_color(PortInstance(blockinstance.PORT_IN)))
		self.assertEqual((0.0, 0.65, 0.25), blockline.portinstance_color(PortInstance(blockinstance.PORT_OUT)))
		self.assertEqual((0.1, 0.35, 1.0), blockline.portinstance_color(PortInstance(blockinstance.PORT_INOUT)))

	def test_blockline_endpoint_markers_are_black_unselected_and_role_coloured_selected(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockline = self.import_canvas_module("blockline")

		class PortInstance:
			io = blockinstance.PORT_OUT

		class Cairo:
			def __init__(self):
				self.rectangles = []
				self.rgba = []

			def save(self):
				pass

			def restore(self):
				pass

			def rectangle(self, x, y, width, height):
				self.rectangles.append((x, y, width, height))

			def set_source_rgba(self, r, g, b, a):
				self.rgba.append((r, g, b, a))

			def fill_preserve(self):
				pass

			def set_line_width(self, width):
				pass

			def set_source_rgb(self, r, g, b):
				pass

			def stroke(self):
				pass

		class Context:
			def __init__(self, cairo):
				self.cairo = cairo

		line = blockline.BlockLine(blockcanvas.BlockCanvas().connections)
		cairo = Cairo()

		line.draw_endpoint_marker(Context(cairo), line.handles()[0], PortInstance(), selected=False)
		line.draw_endpoint_marker(Context(cairo), line.handles()[0], PortInstance(), selected=True)

		self.assertEqual((-3.0, -3.0, 6, 6), cairo.rectangles[0])
		self.assertEqual((0.0, 0.0, 0.0, 1.0), cairo.rgba[0])
		self.assertEqual((-5.0, -5.0, 10, 10), cairo.rectangles[1])
		self.assertEqual((0.0, 0.65, 0.25, 0.9), cairo.rgba[1])

	def test_main_window_canvas_setter_updates_model_and_legacy_alias(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blocklist = self.import_canvas_module("blocklist")

		class View:
			pass

		window = blocklist.mainWindow.__new__(blocklist.mainWindow)
		window.view = View()
		canvas = blockcanvas.BlockCanvas()

		window._set_canvas(canvas)

		self.assertIs(window.view.model, canvas)
		self.assertIs(window.view.canvas, canvas)

	def test_main_window_clear_placement_tool_removes_controller(self):
		blocklist = self.import_canvas_module("blocklist")

		class View:
			def __init__(self):
				self.removed = []

			def remove_controller(self, controller):
				self.removed.append(controller)
				return True

		window = blocklist.mainWindow.__new__(blocklist.mainWindow)
		window.view = View()
		window._placement_controller = object()
		controller = window._placement_controller

		window._clear_placement_tool()

		self.assertEqual([controller], window.view.removed)
		self.assertIsNone(window._placement_controller)

	def test_main_window_delete_removes_focused_item_when_selection_is_empty(self):
		blocklist = self.import_canvas_module("blocklist")

		class Selection:
			def __init__(self, focused_item):
				self.selected_items = set()
				self.focused_item = focused_item
				self.cleared = False

			def unselect_all(self):
				self.cleared = True

		class Canvas:
			def __init__(self):
				self.removed = []

			def remove(self, item):
				self.removed.append(item)

		class View:
			def __init__(self, item):
				self.canvas = Canvas()
				self.selection = Selection(item)
				self.background = []

			def modify_bg(self, state, color):
				self.background.append((state, color))

		class Status:
			def __init__(self):
				self.messages = []

			def push(self, context, message):
				self.messages.append(message)

		item = object()
		window = blocklist.mainWindow.__new__(blocklist.mainWindow)
		window.view = View(item)
		window.status = Status()

		window.delblock()

		self.assertEqual([item], window.view.canvas.removed)
		self.assertTrue(window.view.selection.cleared)

	def test_run_canvas_reports_load_failure_without_findtype_traceback(self):
		blocklist = self.import_canvas_module("blocklist")
		blockcanvas = self.import_canvas_module("blockcanvas")

		class Library:
			def __init__(self):
				self.find_type_called = False

			def loadString(self, model, name):
				raise RuntimeError("parse failed")

			def findType(self, name):
				self.find_type_called = True

		class Ascwrap:
			def __init__(self):
				self.library = Library()

		class View:
			def __init__(self):
				self.canvas = blockcanvas.BlockCanvas()

		class UndoManager:
			def reset(self):
				pass

		class Status:
			def __init__(self):
				self.messages = []

			def push(self, context, message):
				self.messages.append(message)

		class Reporter:
			def __init__(self):
				self.errors = []

			def reportError(self, message):
				self.errors.append(message)

		window = blocklist.mainWindow.__new__(blocklist.mainWindow)
		window.undo_manager = UndoManager()
		window.view = View()
		window.ascwrap = Ascwrap()
		window.status = Status()
		window.reporter = Reporter()

		window.run_canvas(None)

		self.assertFalse(window.ascwrap.library.find_type_called)
		self.assertIn("Canvasmodel could not be loaded: parse failed", window.status.messages)
		self.assertIn("Canvasmodel could not be loaded: parse failed", window.reporter.errors)

	def test_run_canvas_reports_instantiation_failure_without_traceback(self):
		blocklist = self.import_canvas_module("blocklist")
		blockcanvas = self.import_canvas_module("blockcanvas")

		class Type:
			def getSimulation(self, name, build):
				raise RuntimeError("instantiation failed")

		class Library:
			def loadString(self, model, name):
				pass

			def findType(self, name):
				return Type()

		class Ascwrap:
			def __init__(self):
				self.library = Library()

		class View:
			def __init__(self):
				self.canvas = blockcanvas.BlockCanvas()

		class UndoManager:
			def reset(self):
				pass

		class Status:
			def __init__(self):
				self.messages = []

			def push(self, context, message):
				self.messages.append(message)

		class Reporter:
			def __init__(self):
				self.errors = []

			def reportError(self, message):
				self.errors.append(message)

		window = blocklist.mainWindow.__new__(blocklist.mainWindow)
		window.undo_manager = UndoManager()
		window.view = View()
		window.ascwrap = Ascwrap()
		window.status = Status()
		window.reporter = Reporter()

		window.run_canvas(None)

		self.assertIn(
			"Canvasmodel could not be instantiated: instantiation failed",
			window.status.messages,
		)
		self.assertIn(
			"Canvasmodel could not be instantiated: instantiation failed",
			window.reporter.errors,
		)

	def test_block_placement_ignores_drag_resize(self):
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blocklist = self.import_canvas_module("blocklist")
		matrix = self.import_canvas_module("gaphas.matrix")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = []
			outputs = []
			params = []
			port_in = {}
			port_out = {}
			gr = []

		class Model:
			def __init__(self):
				self.updated = []

			def request_update(self, item):
				self.updated.append(item)

		class Selection:
			def __init__(self):
				self.focused_item = None

			def unselect_all(self):
				pass

		class View:
			def __init__(self):
				self.model = Model()
				self.selection = Selection()

			def get_matrix_v2i(self, item):
				return matrix.Matrix()

		class Gesture:
			def __init__(self, view):
				self.view = view
				self.state = None

			def get_widget(self):
				return self.view

			def set_state(self, state):
				self.state = state

		item = blockitem.DefaultBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		view = View()
		gesture = Gesture(view)

		blocklist.on_block_placement_begin(gesture, 40, 50, lambda: item)
		blocklist.on_block_placement_update(gesture, 100, 120)

		self.assertEqual((64.0, 64.0), (item.width, item.height))
		self.assertEqual((40.0, 50.0), item.matrix.tuple()[4:6])
		self.assertIs(view.selection.focused_item, item)

	def test_toolbar_buttons_are_marked_important_for_horizontal_labels(self):
		blocklist = self.import_canvas_module("blocklist")
		gtkcompat = self.import_canvas_module("gtkcompat")

		button = blocklist.configure_toolbar_button(
			gtkcompat.gtk.ToolButton(stock_id=gtkcompat.gtk.STOCK_OPEN), "Open"
		)

		self.assertEqual("Open", button.get_label())
		self.assertTrue(button.get_is_important())

	def test_compact_toolbar_css_installs_once(self):
		blocklist = self.import_canvas_module("blocklist")
		blocklist._COMPACT_TOOLBAR_CSS_INSTALLED = False

		blocklist.install_compact_toolbar_css()
		blocklist.install_compact_toolbar_css()

		self.assertTrue(blocklist._COMPACT_TOOLBAR_CSS_INSTALLED)

	def test_block_icon_view_uses_hashable_tree_path_keys(self):
		blocklist = self.import_canvas_module("blocklist")
		gtkcompat = self.import_canvas_module("gtkcompat")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			iconfile = str(CANVAS_DIR / "defaultblock.svg")

			def get_icon(self, width, height):
				return gtkcompat.gtk.gdk.pixbuf_new_from_file_at_size(self.iconfile, width, height)

		view = blocklist.BlockIconView([BlockType()])

		self.assertEqual(["0"], list(view.otank))

	def test_graphical_block_item_updates_port_positions(self):
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		instance = blockinstance.BlockInstance(BlockType(), name="unit1")
		item = blockitem.GraphicalBlockItem(instance)

		item.up1()

		self.assertEqual((0.0, 32.0), item._ports[0].point.tuple())
		self.assertEqual((64.0, 32.0), item._ports[1].point.tuple())

	def test_default_block_item_places_unspecified_ports_on_left_and_right_edges(self):
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "boiler_simple"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {}
			port_out = {}
			gr = []

		item = blockitem.DefaultBlockItem(blockinstance.BlockInstance(BlockType(), name="boiler1"))
		item.up1()

		self.assertEqual((0.0, 32.0), item._ports[0].point.tuple())
		self.assertEqual((64.0, 32.0), item._ports[1].point.tuple())

		item.width = 80
		item.height = 100
		item.up1()

		self.assertEqual((0.0, 50.0), item._ports[0].point.tuple())
		self.assertEqual((80.0, 50.0), item._ports[1].point.tuple())

	def test_block_resize_constraints_keep_corners_rectangular(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = []
			outputs = []
			params = []
			port_in = {}
			port_out = {}
			gr = []

		canvas = blockcanvas.BlockCanvas()
		item = blockitem.DefaultBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		canvas.add(item)

		item.handles()[1].pos = (100, -10)
		canvas.update_now([item])

		self.assertEqual((0.0, 0.0), item.handles()[0].pos.tuple())
		self.assertEqual((100.0, 0.0), item.handles()[1].pos.tuple())
		self.assertEqual((100.0, 74.0), item.handles()[2].pos.tuple())
		self.assertEqual((0.0, 74.0), item.handles()[3].pos.tuple())
		self.assertEqual((0.0, -10.0), item.matrix.tuple()[4:6])

	def test_nw_resize_normalizes_block_origin(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		canvas = blockcanvas.BlockCanvas()
		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		canvas.add(item)

		item.handles()[0].pos = (12, 14)
		canvas.update_now([item])

		self.assertEqual((0.0, 0.0), item.handles()[0].pos.tuple())
		self.assertEqual((52.0, 0.0), item.handles()[1].pos.tuple())
		self.assertEqual((52.0, 50.0), item.handles()[2].pos.tuple())
		self.assertEqual((0.0, 50.0), item.handles()[3].pos.tuple())
		self.assertEqual((12.0, 14.0), item.matrix.tuple()[4:6])
		item.up1()
		self.assertEqual((0.0, 25.0), item._ports[0].point.tuple())
		self.assertEqual((52.0, 25.0), item._ports[1].point.tuple())

	def test_gaphas_segment_merge_workaround_skips_block_handles(self):
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blocklist = self.import_canvas_module("blocklist")
		itemtool = self.import_canvas_module("gaphas.tool.itemtool")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = []
			outputs = []
			params = []
			port_in = {}
			port_out = {}
			gr = []

		item = blockitem.DefaultBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))

		blocklist.install_gaphas_itemtool_workarounds()
		itemtool.maybe_merge_segments(None, item, item.handles()[1])

	def test_gaphas_scroll_workaround_skips_missing_adjustments(self):
		blocklist = self.import_canvas_module("blocklist")
		scrolltool = self.import_canvas_module("gaphas.tool.scroll")

		class Controller:
			def get_widget(self):
				class View:
					hadjustment = None
					vadjustment = None
				return View()

		blocklist.install_gaphas_scrolltool_workarounds()

		self.assertFalse(scrolltool.on_scroll(Controller(), 1, 1, 10))

	def test_gaphas_block_handle_colour_is_orange(self):
		blocklist = self.import_canvas_module("blocklist")
		handlepainter = self.import_canvas_module("gaphas.painter.handlepainter")

		blocklist.install_gaphas_handle_colors()

		self.assertEqual((0.86, 0.48, 0.18), handlepainter.GREEN_4)

	def test_blockline_connections_update_lineinstance_ports(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blockline = self.import_canvas_module("blockline")
		connector = self.import_canvas_module("gaphas.connector")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		canvas = blockcanvas.BlockCanvas()
		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		line = blockline.BlockLine(canvas.connections)
		canvas.add(item)
		canvas.add(line)
		item.up1()

		line.handles()[0].pos = item._ports[0].point.tuple()
		connector.Connector(line, line.handles()[0], canvas.connections).connect(
			connector.ConnectionSink(item)
		)
		line.handles()[-1].pos = item._ports[1].point.tuple()
		connector.Connector(line, line.handles()[-1], canvas.connections).connect(
			connector.ConnectionSink(item)
		)

		self.assertEqual("inlet", line.lineinstance.fromport.name)
		self.assertEqual("outlet", line.lineinstance.toport.name)
		self.assertIn("unit1.inlet, unit1.outlet ARE_THE_SAME;", str(line.lineinstance))

	def test_blockconnecttool_finds_ports_in_view_coordinates(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockconnecttool = self.import_canvas_module("blockconnecttool")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blockline = self.import_canvas_module("blockline")
		matrix = self.import_canvas_module("gaphas.matrix")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		class Model:
			def __init__(self, items):
				self.items = items

			def get_all_items(self):
				return self.items

		class View:
			def __init__(self, items):
				self.model = Model(items)

			def get_matrix_i2v(self, item):
				return matrix.Matrix()

			def get_matrix_v2i(self, item):
				return matrix.Matrix()

		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		line = blockline.BlockLine(blockcanvas.BlockCanvas().connections)
		item.up1()
		view = View([item, line])

		self.assertIs(blockconnecttool.find_item_at_view_point(view, 32, 32), item)
		self.assertIs(blockconnecttool.find_port_at_view_point(view, 0, 32)[1], item._ports[0])
		self.assertEqual([], blockconnecttool.canvas_ports(line))

	def test_blockconnecttool_uses_directional_port_targets(self):
		blockconnecttool = self.import_canvas_module("blockconnecttool")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		item.up1()
		inlet, outlet = item._ports

		self.assertTrue(blockconnecttool.ports_can_connect(outlet, inlet))
		self.assertTrue(blockconnecttool.ports_can_connect(inlet, outlet))
		self.assertFalse(blockconnecttool.ports_can_connect(inlet, inlet))
		self.assertFalse(blockconnecttool.ports_can_connect(outlet, outlet))

	def test_blockitem_port_labels_are_placed_outside_side_ports(self):
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		class Cairo:
			def __init__(self):
				self.moves = []
				self.text = []

			def text_extents(self, text):
				return (0, -8, len(text) * 6, 10, 0, 0)

			def move_to(self, x, y):
				self.moves.append((x, y))

			def show_text(self, text):
				self.text.append(text)

			def set_source_rgb(self, r, g, b):
				pass

		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		item.up1()
		cr = Cairo()

		blockitem.draw_port_label(cr, item, item._ports[0])
		blockitem.draw_port_label(cr, item, item._ports[1])

		self.assertLess(cr.moves[0][0], -30)
		self.assertGreater(cr.moves[1][0], item.width + 6)

	def test_blockconnecttool_port_sink_updates_lineinstance_ports(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockconnecttool = self.import_canvas_module("blockconnecttool")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blockline = self.import_canvas_module("blockline")
		connector = self.import_canvas_module("gaphas.connector")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		canvas = blockcanvas.BlockCanvas()
		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		line = blockline.BlockLine(canvas.connections)
		canvas.add(item)
		canvas.add(line)
		item.up1()

		connector.Connector(line, line.handles()[0], canvas.connections).connect(
			blockconnecttool.BlockPortConnectionSink(item, item._ports[0])
		)
		connector.Connector(line, line.handles()[-1], canvas.connections).connect(
			blockconnecttool.BlockPortConnectionSink(item, item._ports[1])
		)

		self.assertEqual("inlet", line.lineinstance.fromport.name)
		self.assertEqual("outlet", line.lineinstance.toport.name)

	def test_blockconnecttool_cancel_removes_only_dangling_lines(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockconnecttool = self.import_canvas_module("blockconnecttool")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")
		blockline = self.import_canvas_module("blockline")
		connector = self.import_canvas_module("gaphas.connector")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = [["inlet", "stream", "in: inlet"]]
			outputs = [["outlet", "stream", "out: outlet"]]
			params = []
			port_in = {"inlet": ["0", "5"]}
			port_out = {"outlet": ["10", "5"]}
			gr = [[["0", "0"], ["10", "10"]]]

		class View:
			def __init__(self, model):
				self.model = model
				self.queued = False

			def queue_draw(self):
				self.queued = True

		canvas = blockcanvas.BlockCanvas()
		item = blockitem.GraphicalBlockItem(blockinstance.BlockInstance(BlockType(), name="unit1"))
		dangling = blockline.BlockLine(canvas.connections)
		complete = blockline.BlockLine(canvas.connections)
		canvas.add(item)
		canvas.add(dangling)
		canvas.add(complete)
		item.up1()

		connector.Connector(dangling, dangling.handles()[0], canvas.connections).connect(
			blockconnecttool.BlockPortConnectionSink(item, item._ports[0])
		)
		connector.Connector(complete, complete.handles()[0], canvas.connections).connect(
			blockconnecttool.BlockPortConnectionSink(item, item._ports[0])
		)
		connector.Connector(complete, complete.handles()[-1], canvas.connections).connect(
			blockconnecttool.BlockPortConnectionSink(item, item._ports[1])
		)

		blockconnecttool.cancel_pending_connections(View(canvas))

		items = list(canvas.get_all_items())
		self.assertNotIn(dangling, items)
		self.assertIn(complete, items)

	def test_blockcanvas_generates_minimal_ascend_code(self):
		blockcanvas = self.import_canvas_module("blockcanvas")
		blockinstance = self.import_canvas_module("blockinstance")
		blockitem = self.import_canvas_module("blockitem")

		class TypeDescription:
			def getName(self):
				return "unit_block"

		class BlockType:
			type = TypeDescription()
			inputs = []
			outputs = []
			params = []
			port_in = {}
			port_out = {}
			gr = []

		canvas = blockcanvas.BlockCanvas()
		canvas.model_library = "blocktypes.a4c"
		instance = blockinstance.BlockInstance(BlockType(), name="unit1")
		canvas.add(blockitem.DefaultBlockItem(instance))

		code = str(canvas)

		self.assertIn('REQUIRE "blocktypes.a4c";', code)
		self.assertIn("unit1 IS_A unit_block;", code)


if __name__ == "__main__":
	unittest.main()
