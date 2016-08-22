#!/usr/bin/env python

if __name__ == '__main__':
	print "ERROR: ASCEND Canvas should now be invoked using the file 'canvas.py' instead of 'blocklist.py'."
	exit(1)

from gi.repository import Gtk, GdkPixbuf, Gdk, GObject
import os
import os.path
import glob
import cairo
import ascpy


class BlockIconView(Gtk.IconView):
	"""
	IconView containing the palette of BlockTypes available for use in the
	canvas. The list of blocks is supplied currently as an initialisation
	parameter, but it is intended that this would be dynamic in a final system.

	It should be possible drag icons from the palette into the canvas, but
	that is not yet implemented.
	"""
	def __init__(self, blocks=None, app=None):
		self.model = Gtk.ListStore(str, GdkPixbuf.Pixbuf)
		self.app = app
		self.otank = {}
		try:
			for b in blocks:
				pixbuf = b.get_icon(48,48)
				#print pixbuf
				iter = self.model.append([b.name, pixbuf])
				path = self.model.get_path(iter)
				self.otank[path.to_string()] = b
		except Exception as e:
			pass

		Gtk.IconView.__init__(self)
		self.set_model(self.model)
		self.set_text_column(0)
		self.set_pixbuf_column(1)
		self.set_columns(-1)
		self.set_size_request(180,100)
		self.connect("item-activated", self.item_activated)
		self.connect("selection-changed", self.selection_changed)

	def refresh_view():
		pass

	def selection_changed(self, iconview):
		s = self.get_selected_items()
		if len(s) == 1:
			b = self.otank[s[0].to_string()]
			self.app.set_placement_tool(b)

	def item_activated(self, iconview, path):
		self.app.set_placement_tool(self.otank[path])


from gaphas import GtkView, View
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain
from gaphas.tool import ItemTool, RubberbandTool, PanTool, ZoomTool
from gaphas.painter import ItemPainter
from blockconnecttool import BlockConnectTool
from blockline import BlockLine
from blockitem import DefaultBlockItem, GraphicalBlockItem
from contextmenutool import ContextMenuTool
from connectortool import ConnectorTool
from blockcanvas import BlockCanvas
from blockinstance import BlockInstance
from solverreporterforcanvas import PopupSolverReporter
import canvasproperties
import blockproperties
import undo
from undo import UndoMonitorTool
import errorreporter
import pickle as pickle
# needed to allow cairo.Matrix to be pickled
import gaphas.picklers
import obrowser
import help
from preferences import Preferences


def BlockToolChain():
	"""
	ToolChain for working with BlockCanvas, including several custom Tools.
	"""
	chain = ToolChain()
	chain.append(UndoMonitorTool())
	chain.append(HoverTool())
	chain.append(BlockConnectTool())  # for connect/disconnect of lines
	chain.append(ConnectorTool())  # for creating new lines by drag from Port
	chain.append(ContextMenuTool())  # right-click
	#chain.append(LineSegmentTool())  # for moving line 'elbows'
	chain.append(ItemTool())
	chain.append(PanTool())
	chain.append(ZoomTool())
	chain.append(RubberbandTool())
	return chain

class mainWindow(Gtk.Window):

	menu_xml = '''<ui>
	<menubar name='MenuBar'>
    <menu action='File'>
		<menuitem action='New' />
		<menuitem action='Open' />
		<menuitem action='Save' />
		<menuitem action='SaveAs' />
		<separator />
		<menuitem action='Export' />
		<separator />
		<menuitem action='LoadLibrary' />
		<separator />
		<menuitem action='Quit' />
	  </menu>
	  <menu action='Edit'>
		<menuitem action='Undo' />
		<menuitem action='Redo' />
		<separator />
		<menuitem action='BlockProperties' />
		<menuitem action='Rotate' />
		<menuitem action='Flip' />
		<separator />
		<menuitem action='Delete' />
	  </menu>
	  <menu action='View'>
		<menuitem action='Fullscreen' />
		<separator />
		<menuitem action='ZoomIn' />
		<menuitem action='ZoomOut' />
	  </menu>
	  <menu action='Tools'>
		<menuitem action='Debug' />
		<menuitem action='Run' />
		<menuitem action='Preview' />
	  </menu>
	  <menu action='Help'>
		<menuitem action='Development' />
		<menuitem action='ReportBug' />
		<menuitem action='About' />
	  </menu>
	</menubar>
	</ui>
	'''

	def __init__(self, library, options=None):
		"""
		  Initialise the application -- create all the widgets, and populate the icon palette.
		  TODO: separate the icon palette into a separate method.
		  """
		self.ascwrap= library
		self.errorvars = []
		self.errorblocks = []
		self.status = Gtk.Statusbar()
		self.temp = []
		self.act = 1 # to be used for storing canvas zoom in/out related data.
		# the Gaphas canvas
		canvas = BlockCanvas()

		# the main window
		Gtk.Window.__init__(self)
		self.iconok = self.render_icon(Gtk.STOCK_YES,Gtk.IconSize.MENU)
		self.iconinfo = self.render_icon(Gtk.STOCK_DIALOG_INFO,Gtk.IconSize.MENU)
		self.iconwarning = self.render_icon(Gtk.STOCK_DIALOG_WARNING,Gtk.IconSize.MENU)
		self.iconerror = self.render_icon(Gtk.STOCK_DIALOG_ERROR,Gtk.IconSize.MENU)

		self.set_title("ASCEND Canvas Modeller")
		self.set_default_size(800, 800)

		#self.connect("destroy", Gtk.main_quit)
		self.connect("delete-event", self.quit_confirm)
		# check this to define the function of close button on the main window

		windowicon = Gtk.Image()
		windowicon.set_from_file(os.path.join('..', 'glade', 'ascend.svg'))
		self.set_icon(windowicon.get_pixbuf())

		vbox = Gtk.VBox()

		ui_manager = Gtk.UIManager()
		accelgroup = ui_manager.get_accel_group()
		self.add_accel_group(accelgroup)

		actiongroup = Gtk.ActionGroup('CanvasActionGroup')

		self.prefs = Preferences()

		self.contextmenutool = ContextMenuTool()

		actions = [('File', None, '_File')
			,('Quit', Gtk.STOCK_QUIT, '_Quit', None,'Quit the Program', self.quit_confirm)
			,('New', Gtk.STOCK_NEW,'_New',None,'Start a new Simulation', self.new)
			,('Open', Gtk.STOCK_OPEN,'_Open',None,'Open a saved Canvas file', self.fileopen)
			,('Save', Gtk.STOCK_SAVE,'_Save',None,'Save a Canvas file', self.save_canvas)
			,('SaveAs', Gtk.STOCK_SAVE_AS,'_Save As...',None,'Save a Canvas file as ...', self.filesave)
			,('Export', Gtk.STOCK_PRINT, '_Export SVG', None,'Quit the Program', self.export_svg_as)
			,('LoadLibrary', Gtk.STOCK_OPEN, '_Load Library...', '<Control>l','Load Library', self.load_library_dialog)
			,('Edit', None, '_Edit')
			,('Undo', Gtk.STOCK_UNDO, '_Undo', '<Control>z', 'Undo Previous Action', self.undo_canvas)
			,('Redo',Gtk.STOCK_REDO, '_Redo', '<Control>y', 'Redo Previous Undo', self.redo_canvas)
			,('BlockProperties',Gtk.STOCK_PROPERTIES, '_Block Properties', None, 'Edit Block Properties', self.bproperties)
			,('Delete', Gtk.STOCK_DELETE, '_Delete', 'Delete', 'Delete Selected Item', self.delblock)
			,('View', None, '_View')
			,('Fullscreen', Gtk.STOCK_FULLSCREEN, '_Full Screen', 'F11', 'Toggle Full Screen', self.fullscrn)
			,('ZoomIn', Gtk.STOCK_ZOOM_IN, '_Zoom In', None, 'Zoom In Canvas', self.zoom)
			,('ZoomOut', Gtk.STOCK_ZOOM_OUT, '_Zoom Out', None, 'Zoom Out Canvas', self.zoom)
			#,('BestFit', Gtk.STOCK_ZOOM_FIT, '_Best Fit', None, 'Best Fit Canvas',self.zoom)
			,('Tools', None, '_Tools')
			,('Debug', None, '_Debug', None, 'View Instance Browser',self.debug_canvas)
			,('Run', Gtk.STOCK_EXECUTE, '_Run', 'F5', 'Solve Canvas', self.run_canvas)
			,('Preview', Gtk.STOCK_PRINT_PREVIEW, '_Preview', None, 'Preview Generated Code', self.preview_canvas)
			,('Help', None, '_Help')
			,('Development', Gtk.STOCK_INFO, '_Development', None, 'Check Development', self.on_get_help_online_click)
			,('ReportBug', None, '_Report Bug', None, 'Report a Bug!', self.on_report_a_bug_click)
			,('About', Gtk.STOCK_ABOUT, '_About', None, 'About Us', self.about)
			,('Rotate',None, '_Rotate', '<Control>r', 'Rotate Clockwise', self.brotate)
			,('Flip', None, '_Flip', '<Control>f', 'Flip', self.bflip)
		]

		actiongroup.add_actions(actions)

		ui_manager.insert_action_group(actiongroup, 0)
		ui_manager.add_ui_from_string(self.menu_xml)

		# Creating Menu Bar
		menubar = ui_manager.get_widget('/MenuBar')
		vbox.pack_start(menubar, False, False, 0)

		# Creating Tool Bar
		'''The Toolbar Definations start here'''

		tb = Gtk.Toolbar()
		tb.set_style(Gtk.ToolbarStyle.BOTH)
		# Load Button
		loadbutton = Gtk.ToolButton(Gtk.STOCK_OPEN)
		loadbutton.set_label("Load")
		loadbutton.connect("clicked", self.fileopen)
		tb.insert(loadbutton, 0)

		# Save Button
		savebutton = Gtk.ToolButton(Gtk.STOCK_SAVE)
		savebutton.set_label("Save")
		savebutton.connect("clicked", self.save_canvas)
		tb.insert(savebutton, 1)

		# Undo Button
		undob = Gtk.ToolButton(Gtk.STOCK_UNDO)
		undob.set_label("Undo")
		undob.connect("clicked", self.undo_canvas)
		tb.insert(undob, 2)

		# Redo Button
		redob = Gtk.ToolButton(Gtk.STOCK_REDO)
		redob.set_label("Redo")
		redob.connect("clicked", self.redo_canvas)
		tb.insert(redob, 3)

		# Debug Button
		debugbutton = Gtk.ToolButton(Gtk.STOCK_PROPERTIES)
		debugbutton.set_label("Debug")
		debugbutton.connect("clicked", self.debug_canvas)
		tb.insert(debugbutton, 4)

		# Preview Button
		previewb = Gtk.ToolButton(Gtk.STOCK_PRINT_PREVIEW)
		previewb.set_label("Preview")
		previewb.connect("clicked", self.preview_canvas)
		tb.insert(previewb, 5)

		# Export Button
		exportbutton = Gtk.ToolButton(Gtk.STOCK_CONVERT)
		exportbutton.set_label("Export SVG")
		exportbutton.connect("clicked", self.export_svg_as)
		tb.insert(exportbutton, 6)

		# Run Button
		runb = Gtk.ToolButton(Gtk.STOCK_EXECUTE)
		runb.set_label("Run")
		runb.connect("clicked", self.run_canvas)
		tb.insert(runb, 7)

		##Custom Entry
		#m_entry = Gtk.ToolButton(Gtk.STOCK_SAVE_AS)
		#m_entry.set_label("Custom METHOD")
		#m_entry.connect("clicked",self.custommethod)
		#tb.insert(m_entry,5)

		vbox.pack_start(tb, False, False, 0)

		# hbox occupies top part of vbox, with icons on left & canvas on right.
		paned = Gtk.HPaned()

		# the 'view' widget implemented by Gaphas
		#gaphas.view.DEBUG_DRAW_BOUNDING_BOX = True
		self.view = GtkView()
		# workaround to provide proper adjustments
		hadj = GObject.ParamSpecObject
		hadj.name = "hadjustment"
		self.view.do_set_property(hadj, None)

		self.view.tool = BlockToolChain()

		# table containing scrollbars and main canvas
		t = Gtk.Table(2,2)
		self.view.canvas = canvas
		self.view.zoom(1)
		self.view.set_size_request(450, 300)
		hs = Gtk.HScrollbar(self.view.hadjustment)
		vs = Gtk.VScrollbar(self.view.vadjustment)
		t.attach(self.view, 0, 1, 0, 1)
		t.attach(hs, 0, 1, 1, 2, xoptions=Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.FILL)
		t.attach(vs, 1, 2, 0, 1, xoptions=Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.FILL)

		# a scrolling window to contain the icon palette
		self.scroll = Gtk.ScrolledWindow()
		self.scroll.set_border_width(2)
		self.scroll.set_shadow_type(Gtk.ShadowType.ETCHED_IN)

		self.scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

		self.blockiconview = BlockIconView(blocks=self.ascwrap.canvas_blocks, app=self)
		self.scroll.set_size_request(250, 300)
		self.scroll.add(self.blockiconview)
		self.reporter = self.ascwrap.reporter

		paned.pack1(self.scroll, False, False)
		paned.pack2(t)
		vbox.pack_start(paned, True, True, 0)
		vpane = Gtk.VPaned()
		vpane.pack1(vbox, False, False)
		lower_vbox = Gtk.VBox()

		self.ET = errorreporter.ErrorReporter(self.reporter, self.iconok, self.iconinfo, self.iconwarning, self.iconerror)
		self.notebook = Gtk.Notebook()
		self.notebook.set_tab_pos(Gtk.PositionType.TOP)
		label = Gtk.Label(label='Error / Status Reporter Console')
		scrolledwindow = Gtk.ScrolledWindow()
		scrolledwindow.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
		scrolledwindow.add(self.ET.errorview)
		self.notebook.append_page(scrolledwindow, label)
		self.notebook.set_size_request(700, 100)
		lower_vbox.pack_start(self.notebook, True, True, 0)
		lower_vbox.pack_start(self.status, False, False, 0)
		vpane.pack2(lower_vbox, False, False)

		self.undo_manager = undo.UndoManager(self)
		self.undo_manager.start()

		self.add(vpane)
		self.show_all()

	@undo.block_observed
	def set_placement_tool(self,blocktype):
		"""
		  Prepare the canvas for drawing a block of the type selected in the
		  icon palette.
		  """
		# TODO: add undo handler
		graphic = blocktype.gr
		def my_block_factory():
			def wrapper():
				b = BlockInstance(blocktype)
				if len(graphic) == 0:
					bi = DefaultBlockItem(b)
				else:
					bi = GraphicalBlockItem(b)
				self.view.canvas.add(bi)
				return bi
			return wrapper
		self.view.unselect_all()
		self.view.tool.grab(PlacementTool(self.view,my_block_factory(), HandleTool(), 2))
		self.status.push(0,"Selected '%s'..." % blocktype.type.getName())

	def set_connector_tool(self, foobar):
		"""
		  Prepare the canvas for drawing a connecting line (note that one can
		  alternatively just drag from a port to another port).
		  """

		def my_line_factory():
			def wrapper():
				l = BlockLine()
				self.view.canvas.add(l)
				return l
			return wrapper
		self.view.tool.grab(PlacementTool(self.view, my_line_factory(), HandleTool(), 1))

	@undo.block_observed
	def delblock(self, widget=None):
		'''Both individual and multiple selected items are deleted by the logic of following routine'''

		if self.view.selected_items:
			while len(self.view.selected_items) != 0:
				self.view.canvas.remove(self.view._selected_items.pop())
				self.status.push(0,"Item deleted.")
				self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#FFF'))

		'''if self.view.focused_item:
			self.view.canvas.remove(self.view.focused_item)
			self.status.push(0,"Item deleted.")
			self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#FFF'))'''

	@undo.block_observed
	def new(self, widget):
		for item in self.view.canvas.get_all_items():
			self.view.canvas.remove(item)
		self.reporter.reportNote('Canvas cleared for new Model')
		self.view.canvas.filestate = 0
		self.view.canvas.filename = None
		self.view.canvas.canvasmodelstate = 'Unsolved'
		self.status.push(0, "Canvasmodel state : %s" % self.view.canvas.canvasmodelstate)
		self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#FFF'))

	def debug_canvas(self, widget):
		"""
		  Display an 'object browser' to allow inspection of the objects
		  in the canvas.
		"""

		b = obrowser.Browser("canvas", self.view.canvas, False)
		self.status.push(0, "Canvasmodel state : %s" % self.view.canvas.canvasmodelstate)

	def save_canvas(self,widget):
		"""
		  Save the canvas in 'pickle' format. Currently saving is jointly handled by both self.save_canvas and self.filesave methods
		  """
		if not self.view.canvas.filestate:
			self.filesave(widget)
			return
		else:
			f = file(self.view.canvas.filename, "w")
		try:
			pickle.dump(self.view.canvas, f)
			self.status.push(0, "Canvasmodel saved...")
		except Exception,e:
			print "ERROR:",str(e)
			self.status.push(0,"Canvasmodel could not be saved : " + str(e))
			b = obrowser.Browser("canvas", self.view.canvas)
			d = Gtk.Dialog("Error", self, Gtk.DialogFlags.DESTROY_WITH_PARENT, (Gtk.STOCK_OK, Gtk.ResponseType.ACCEPT))
			d.vbox.add(Gtk.Label(label=str(e)))
			d.show_all()
			d.run()
			d.hide()
		finally:
			f.close()

	def load_canvas(self, widget):
		"""
		  Restore a saved canvas in 'pickle' format. Currently not in use as loading now handled by self.fileopen().
		"""
		f = file("./test.a4b", "r")
		try:
			self.view.canvas = pickle.load(f)
			if self.view.canvas.model_library is not None:
				print "Loading Library...."
				self.loadlib(self, self.view.canvas.model_library, 0)
			self.view.canvas.reattach_ascend(self.ascwrap.library, self.ascwrap.annodb)
			self.view.canvas.update_now()
			self.status.push(0, "Canvasmodel sucessfully loaded...")
		except Exception, e:
			self.status.push(0, "Canvasmodel could not be loaded : " + str(e))
			self.reporter.ReportError("Canvasmodel could not be loaded : " + str(e))
			self.reporter.ReportNote(" Error occured while attempting to load the file")
		finally:
			f.close()
		self.load_presaved_canvas(None)
		self.status.push(0, "Canvasmodel state : %s" % self.view.canvas.canvasmodelstate)

	def temp_canvas(self, widget):
		"""
		  NOTE: 	re-implementation needed
		"""
		pass

	def undo_canvas(self, widget):
		"""
		  Undo
		  """
		self.undo_manager.undo()

	def redo_canvas(self, widget):
		"""
		  NOTE: 	re-implementation needed

		  """
		self.undo_manager.redo()

	def preview_canvas(self, widget):
		"""
		  Output an ASCEND representation of the canvas on the commandline.
		  Under development.
		  """
		self.status.push(0, "Canvasmodel state : %s" % self.view.canvas.canvasmodelstate)
		#info.Info(self.view.parent.parent.parent.parent.parent,str(self.view.canvas),"Canvas Preview").run()
		canvasproperties.CanvasProperties(self).run()

	def export_svg_as(self, widget):

		f = None
		dialog = Gtk.FileChooserDialog("Export Canvas As...", self,
			Gtk.FileChooserAction.SAVE, (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
		dialog.set_default_response(Gtk.ResponseType.OK)
		filter = Gtk.FileFilter()
		filter.set_name("Image File")
		filter.add_pattern("*.svg")
		dialog.add_filter(filter)
		dialog.show()
		response = dialog.run()

		svgview = View(self.view.canvas)
		svgview.painter = ItemPainter()

		# Update bounding boxes with a temporaly CairoContext
		# (used for stuff like calculating font metrics)
		tmpsurface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 0, 0)
		tmpcr = cairo.Context(tmpsurface)
		svgview.update_bounding_box(tmpcr)
		tmpcr.show_page()
		tmpsurface.flush()

		if response == Gtk.ResponseType.OK:
			name = dialog.get_filename()
			if '.svg' not in name:
				name += '.svg'
			if f == None and f != name:
				f = open(name, 'w')
			elif f != None and name == f:
				dialog.set_do_overwrite_confirmation(True)
				dialog.connect("confirm-overwrite", self.confirm_overwrite_callback)

			fn = name
			w, h = svgview.bounding_box.width, svgview.bounding_box.height
			surface = cairo.SVGSurface(fn, w, h)
			cr = cairo.Context(surface)
			svgview.matrix.translate(-svgview.bounding_box.x, -svgview.bounding_box.y)
			svgview.paint(cr)
			cr.show_page()
			surface.flush()
			surface.finish()
			self.reporter.reportNote(" File ' %s ' saved successfully." % name )
			self.status.push(0, "Wrote SVG file '%s'." % fn)
		dialog.destroy()

	def export_svg(self, widget):
		svgview = View(self.view.canvas)
		svgview.painter = ItemPainter()

		# Update bounding boxes with a temporaly CairoContext
		# (used for stuff like calculating font metrics)
		tmpsurface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 0, 0)
		tmpcr = cairo.Context(tmpsurface)
		svgview.update_bounding_box(tmpcr)
		tmpcr.show_page()
		tmpsurface.flush()

		fn = 'demo.svg'
		w, h = svgview.bounding_box.width, svgview.bounding_box.height
		surface = cairo.SVGSurface(fn , w, h)
		cr = cairo.Context(surface)
		svgview.matrix.translate(-svgview.bounding_box.x, -svgview.bounding_box.y)
		svgview.paint(cr)
		cr.show_page()
		surface.flush()
		surface.finish()

		self.status.push(0, "Wrote SVG file '%s'." % fn)

		#def run_presaved_canvas(self,widget):
		##TODO: Separate
		#if self.view.canvas.saved_model is not None:
		#model = self.view.canvas.saved_model
		#self.ascwrap.library.loadString(model,"canvasmodel")

		T = self.ascwrap.library.findType("canvasmodel")
		M = T.getSimulation('canvassim', True)
		M.setSolver(ascpy.Solver("QRSlv"))
		M.solve(ascpy.Solver("QRSlv"), ascpy.SolverReporter())

	#for item in self.view.canvas.get_all_items():
	#if hasattr(item, 'blockinstance'):
	#bi = item.blockinstance
	#for i in self.ascwrap.library.modules.getChildren()[0].getChildren():
	#if str(bi.name) == str(i.getName()):
	#bi.instance = i

	def load_presaved_canvas(self, widget):
		#TODO: Separate
		print self.view.canvas.saved_data
		try:
			if self.view.canvas.saved_data is not None:
				model = str(self.view.canvas)
				self.ascwrap.library.loadString(model, "canvasmodel")
				T = self.ascwrap.library.findType("canvasmodel")
				M = T.getSimulation("canvassim", True)

				def assignval(sim_inst, name):
					if sim_inst.isAtom():
						try:
							sim_inst.setRealValue(self.view.canvas.saved_data[name])
						except Exception, e:
							print e
					elif sim_inst.isRelation():
						pass
					else:
						for i in sim_inst.getChildren():
							k = name + "." + str(i.getName())
							assignval(i, k)

				for i in M.getChildren()[0].getChildren():
					assignval(i, str(i.getName()))

				for item in self.view.canvas.get_all_items():
					if hasattr(item, 'blockinstance'):
						bi = item.blockinstance
						for i in M.getChildren()[0].getChildren():
							if str(bi.name) == str(i.getName()):
								bi.instance = i
				self.status.push(0, "Canvasmodel state : %s" % self.view.canvas.canvasmodelstate)
		except AttributeError:
			self.status.push(0, "Canvasmodel state : Unsolved")

	def run_canvas(self, widget):
		#TODO: Separate
		"""
		  Exports canvas to ASCEND solver and attempts to solve it. Also provides realtime feedback.
		  """
		self.undo_manager.reset()
		model = str(self.view.canvas)
		#print model
		self.view.canvas.saved_model = model
		self.view.canvas.saved_data = {}

		self.ascwrap.library.loadString(model, "canvasmodel")
		T = self.ascwrap.library.findType("canvasmodel")


		self.M = T.getSimulation("canvassim", True)
		self.M.setSolver(ascpy.Solver("QRSlv"))
		self.reporter = ascpy.getReporter()
		reporter = PopupSolverReporter(self, self.M.getNumVars())

		try:
			self.M.build()
		except RuntimeError, e:
			print "Couldn't build system: %s" % str(e)
			self.status.push(0, "Couldn't build system: %s" % str(e));
			return
		#try:
		#met=T.getMethod('on_load')
		#self.M.run(met)
		#except Exception,e:
		#print "Couldn't run on_load method: %s"% str(e)
		#return

		self.status.push(0, "Solving with 'QRSlv'. Please Wait...")

		try:
			self.M.solve(ascpy.Solver("QRSlv"), reporter)
			self.reporterrorblock(self.M)
		except RuntimeError:
			self.reporterrorblock(self.M)

		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				for i in self.M.getChildren()[0].getChildren():
					if str(bi.name) == str(i.getName()):
						bi.instance = i

		# Storing solved variables into a dictionary which will be pickled
		for i in self.M.getallVariables():
			self.view.canvas.saved_data[str(i.getName())] = i.getValue()

	def about(self, widget):
		about = Gtk.AboutDialog()
		about.set_program_name("ASCEND CANVAS")
		about.set_version("0.9.6x alpha")
		about.set_copyright("Copyright \xc2\xa9 Carnegie Mellon University")
		about.set_comments("Canvas-Based GUI Modeller for Energy System")
		about.set_website("http://ascend4.org/Main_Page")
		windowicon = Gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
		about.set_icon(windowicon.get_pixbuf())
		about.set_logo(GdkPixbuf.Pixbuf.new_from_file("../glade/ascend.png"))
		about.set_transient_for(self)
		about.run()
		about.destroy()

	def dummy(self, widget):
		dum = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Sorry ! This fuctionality is not implemented yet.")
		dum.run()
		dum.destroy()

	def fileopen(self, widget):
		dialog = Gtk.FileChooserDialog("Open..",self,Gtk.FileChooserAction.OPEN,
									   (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
		dialog.set_default_response(Gtk.ResponseType.OK)
		filter = Gtk.FileFilter()
		filter.set_name("Canvas Files")
		filter.add_mime_type("Canvas Files/a4b")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		filter = Gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		res = dialog.run()
		if res == Gtk.ResponseType.OK:
			result = dialog.get_filename()
			self.load_canvas_file(result)
		dialog.destroy()

	def load_canvas_file(self, filename):
		#TODO: Separate
		f = file(filename, "r")
		try:
			self.view.canvas = pickle.load(f)
			if self.view.canvas.model_library is not None:
				self.loadlib(self, self.view.canvas.model_library)
			self.view.canvas.reattach_ascend(self.ascwrap.library, self.ascwrap.annodb)
			self.view.canvas.update_now()
			self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#FFF'))
			self.load_presaved_canvas(None)
			self.reporter.reportSuccess("File %s successfully loaded." % filename)
			self.status.push(0, "File %s Loaded." % filename)
			self.view.canvas.filename = filename
		except Exception, e:
			self.reporter.reportError("Error occured while attempting to load the file: %s." % filename)
			self.status.push(0, "Error occured when loading the file: %s." % filename)
			print e
		finally:
			f.close()

	def filesave(self, widget):
		f = None
		dialog = Gtk.FileChooserDialog("Save...", self, Gtk.FileChooserAction.SAVE, (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
		dialog.set_default_response(Gtk.ResponseType.OK)
		if self.view.canvas.filestate == 0:
			dialog.set_filename("untitled")
		filter = Gtk.FileFilter()
		filter.set_name("Canvas File")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		dialog.show()
		dialog.set_do_overwrite_confirmation(True)
		#dialog.connect("confirm-overwrite", self.confirm_overwrite_callback)
		response = dialog.run()
		if response == Gtk.ResponseType.OK:
			name = dialog.get_filename()
			if '.a4b' not in name:
				name += '.a4b'
			if f == None and f != name:
				f = open(name, 'w')
			try:
				pickle.dump(self.view.canvas, f)
				self.reporter.reportNote(" File ' %s ' saved successfully." % name)
				self.status.push(0, "CanvasModel Saved.")
				self.view.canvas.filestate = 1
				self.view.canvas.filename = name
			except Exception, e:
				print "ERROR:", str(e)
				b = obrowser.Browser("canvas", self.view.canvas)
				d = Gtk.Dialog("Error", self, Gtk.DialogFlags.DESTROY_WITH_PARENT, (Gtk.STOCK_OK, Gtk.ResponseType.ACCEPT))
				d.vbox.add(Gtk.Label(label=str(e)))
				d.show_all()
				d.run()
				d.hide()
			finally:
				f.close()
		dialog.destroy()

	def confirm_overwrite_callback(self, widget):

		uri = widget.get_filename()
		if is_uri_read_only(uri):
			if user_wants_to_replace_read_only_file(uri):
				return Gtk.FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME
			else:
				return Gtk.FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
		else:
			return Gtk.FILE_CHOOSER_CONFIRMATION_CONFIRM

	# Block properties
	def bproperties(self, widget=None):
		if self.view.focused_item:
			blockproperties.BlockProperties(self, self.view.focused_item).run()
		else:
			m = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE,
								  "No Block was selected! Please select a Block to view its properties.")
			m.run()
			m.destroy()

	# Block rotate
	def brotate(self, widget=None):
		#b = BlockInstance(blocktype)
		#self.blockitem = BlockItem(b)
		if self.view.focused_item:
			self.contextmenutool.blockrotate_clock(self, self.view.focused_item, None)
		else:
			m = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.ERROR,
								  Gtk.ButtonsType.CLOSE,
								  "No Block was selected! Please select a Block to rotate")
			m.run()
			m.destroy()

	# Block flip
	def bflip(self, widget=None):
		# b = BlockInstance(blocktype)
		# self.blockitem = BlockItem(b)
		if self.view.focused_item:
			self.contextmenutool.blockflip(self, self.view.focused_item, None)
		else:
			m = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.ERROR,
								  Gtk.ButtonsType.CLOSE,
								  "No Block was selected! Please select a Block to rotate")
			m.run()
			m.destroy()

	def reporterrorblock(self, sim):
		#TODO: Separate
		self.errortext = "Canvasmodel could NOT be solved \n Blocks which couldnot be solved:"

		def checkifsolved(sim_inst, name):
			if sim_inst.getType().isRefinedSolverVar():
				try:
					#if sim_inst.getStatus() == 2 :
					#	print name + " -> " + str(sim_inst.getStatus())
					#	self.errorvars.append(name)
					if sim_inst.getStatus() == 3:
						self.errorvars.append(name)

				except Exception, e:
					print e
			elif sim_inst.isRelation():
				pass
			else:
				for i in sim_inst.getChildren():
					k = name + "." + str(i.getName())
					checkifsolved(i, k)

		self.errorvars = []
		self.errorblocks = []
		self.activevar = []
		for i in sim.getChildren()[0].getChildren():
			print i.getName()
			checkifsolved(i, str(i.getName()))

		print self.errorvars
		for i in sim.getChildren()[0].getChildren():
			for j in self.errorvars:
				if str(i.getName()) in j:
					if str(i.getName()) in self.errorblocks:
						pass
					else:
						self.errortext += '\n'
						self.errortext += str(i.getName())  # +' --> ' + ' could not be solved'
						self.errorblocks.append(str(i.getName()))
		print self.errorblocks
		self.fill_blocks()

	def fill_blocks(self):
		cr = self.view.canvas._obtain_cairo_context()
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				bi.color_r = 1
				bi.color_g = 1
				bi.color_b = 1
				flag = 0
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				for i in self.errorblocks:
					if str(bi.name) == str(i):
						bi.color_r = .5
						bi.color_g = 0
						bi.color_b = 0
						flag = 1

		print "updating canvas"
		self.view.canvas.update_now()
		if flag == 1:
			self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#F88'))
			flag = 0
		else:
			self.view.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#AFA'))

	def load_library_dialog(self, widget):
		# TODO: separate
		# Warning Dialog
		if self.view.canvas.get_all_items():
			m = Gtk.MessageDialog(self, Gtk.DialogFlags.MODAL, Gtk.MessageType.WARNING, Gtk.ButtonsType.OK_CANCEL,
								  "Load New Library Will Clear Current Canvas, Continue?")
			m.set_title('Warning!')
			response = m.run()
			m.destroy()
			if response == Gtk.ResponseType.CANCEL:
				return
			else:
				for item in self.view.canvas.get_all_items():
					self.view.canvas.remove(item)
				self.reporter.reportNote('Canvas cleared for new Model')

		dialog = Gtk.FileChooserDialog('Load Library...',self,Gtk.FileChooserAction.OPEN,
									   (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
		dialog.set_default_response(Gtk.ResponseType.OK)
		dialog.set_current_folder(os.path.join(self.ascwrap.defaultlibraryfolder))

		filter = Gtk.FileFilter()
		filter.set_name("Canvas Files")
		filter.add_mime_type("Canvas Files/a4c")
		filter.add_pattern("*.a4c")
		dialog.add_filter(filter)

		#filter = Gtk.FileFilter()
		#filter.set_name("All files")
		#filter.add_pattern("*")
		#dialog.add_filter(filter)

		res = dialog.run()
		if res == Gtk.ResponseType.OK:
			result = dialog.get_filename()
			self.loadlib(lib_name = os.path.basename(result))
		dialog.destroy()

	def loadlib(self, widget=None, lib_name=None):
	# TODO: Separate
	#if loadcondition == 1:
	#if self.view.canvas.model_library == lib_name:
	#m = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.WARNING, Gtk.ButtonsType.CLOSE, "The selected Library is already loaded. ")
	#m.run()
	#m.destroy()
	#return
	#if self.view.canvas.get_all_items():
	#m = Gtk.MessageDialog(self, Gtk.DialogFlags.DESTROY_WITH_PARENT, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Unable to switch Library ! The canvas contains models from present Model Library. ")
	#m.run()
	#m.destroy()
	#return
	#print lib_path
		try:
			self.ascwrap.load_library(lib_name)

			self.view.canvas.model_library = lib_name

			self.scroll.remove(self.blockiconview)
			self.blockiconview = BlockIconView(self.ascwrap.canvas_blocks, self)
			self.scroll.add(self.blockiconview)
			self.show_all()
			self.reporter.reportSuccess("Library %s successfully loaded: Found %d block types. " %(lib_name, (len(self.ascwrap.canvas_blocks))))
			self.status.push(0, "Library %s Loaded." % lib_name)
		except Exception, e:
			self.reporter.reportError("Error occured while attempting to load the library: %s." % lib_name)
			self.status.push(0, "Error occured when loading the library: %s." % lib_name)
			print e

	def get_libraries_from_folder(self, path=None):
		'''Returns a dictionary of library names with their paths'''
		if path == None:
			return
		libs = {}
		locs = glob.glob(os.path.join(path, '*.a4c'))
		for loc in locs:
			name = loc.strip(path)
			libs[name] = loc
		return libs

	def zoom(self, widget):
		zoom = {'ZoomIn':1.2, 'ZoomOut':0.8, 'BestFit':1.0}
		x = zoom[widget.get_name()]
		self.act = self.act * x
		self.view.zoom(x)

	def fullscrn(self, widget):
		try:
			if self.flag == 1:
				self.unfullscreen()
				self.flag = 0
			else:
				self.fullscreen()
				self.flag = 1
		except:
			self.fullscreen()
			self.flag = 1

	def on_contents_click(self, widget):
		_help = help.Help(url="http://ascendwiki.cheme.cmu.edu/Canvas-based_modeller_for_ASCEND ")
		_help.run()

	def on_get_help_online_click(self, widget):
		_help = help.Help(url="http://ascend4.org/Canvas_Development")
		_help.run()

	def on_report_a_bug_click(self, widget):
		_help = help.Help(url="http://bugs.ascend4.org/my_view_page.php")
		_help.run()

	def quit_confirm(self, widget, event=None):
		# TODO: check whether current canvas has been saved or not.
		# Warning Dialog
		if self.view.canvas.get_all_items():
			m = Gtk.MessageDialog(self, Gtk.DialogFlags.MODAL, Gtk.MessageType.WARNING, Gtk.ButtonsType.YES_NO,
								  "Current Canvas is not Saved, Continue Exit?")
			m.set_title('Warning!')
			response = m.run()
			m.destroy()
			if response == Gtk.ResponseType.NO:
				return True
		del self.prefs
		Gtk.main_quit()
		# self.destroy()


