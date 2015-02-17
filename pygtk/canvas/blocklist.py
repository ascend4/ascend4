#!/usr/bin/env python
if __name__ == '__main__':
	print "ERROR: ASCEND Canvas should now be invoked using the file 'canvas.py' instead of 'blocklist.py'."
	exit(1)

import gtk
import os, os.path, glob
import cairo
import ascpy

class BlockIconView(gtk.IconView):
	"""
	IconView containing the palette of BlockTypes available for use in the
	canvas. The list of blocks is supplied currently as an initialisation
	parameter, but it is intended that this would be dynamic in a final system.

	It should be possible drag icons from the palette into the canvas, but
	that is not yet implemented.
	"""
	def __init__(self,blocks=None,app=None):
		self.model = gtk.ListStore(str, gtk.gdk.Pixbuf)
		self.app = app
		self.otank = {}
		try:
			for b in blocks:
				pixbuf = b.get_icon(48,48)
				iter = self.model.append([b.type.getName(), pixbuf])
				path = self.model.get_path(iter)
				self.otank[path] = b
		except Exception as e:
			pass

		gtk.IconView.__init__(self)
		self.set_model(self.model)
		self.set_text_column(0)
		self.set_pixbuf_column(1)
		self.set_columns(-1)
		self.set_size_request(180,100)
		self.connect("item-activated", self.item_activated)
		self.connect("selection-changed", self.selection_changed)

	def refresh_view():
		pass
	
	def selection_changed(self,iconview):
		s = self.get_selected_items()
		if len(s)==1:
			b = self.otank[s[0]]
			self.app.set_placement_tool(b)
			
	def item_activated(self,iconview, path):
		self.app.set_placement_tool(self.otank[path])


from gaphas import GtkView, View
from gaphas.tool import HoverTool, PlacementTool, HandleTool, ToolChain, DEBUG_TOOL_CHAIN
#from gaphas.tool import LineSegmentTool
from gaphas.tool import Tool, ItemTool, RubberbandTool
from gaphas.painter import ItemPainter
from blockconnecttool import BlockConnectTool
from blockline import BlockLine
from blockitem import DefaultBlockItem
from contextmenutool import ContextMenuTool
from connectortool import ConnectorTool
from blockcanvas import BlockCanvas
#from panzoom import ZoomTool
#from panzoom import PanTool
from gaphas.tool import PanTool, ZoomTool
from blockinstance import BlockInstance
from solverreporterforcanvas import PopupSolverReporter
import canvasproperties
import blockproperties
import undo
from undo import UndoMonitorTool
import errorreporter
import gaphas.view
import pickle as pickle
import gaphas.picklers
import obrowser
import urllib, help
from preferences import Preferences
		
def BlockToolChain():
	"""
	ToolChain for working with BlockCanvas, including several custom Tools.
	"""
	chain = ToolChain()
	chain.append(UndoMonitorTool())
	chain.append(HoverTool())
	chain.append(BlockConnectTool()) # for connect/disconnect of lines
	chain.append(ConnectorTool()) # for creating new lines by drag from Port
	chain.append(ContextMenuTool()) # right-click
#	chain.append(LineSegmentTool()) # for moving line 'elbows'
	chain.append(ItemTool())
	chain.append(PanTool())
	chain.append(ZoomTool())
	chain.append(RubberbandTool())
	return chain

class mainWindow(gtk.Window):
	
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
	    <menuitem action='Library' />
	    <separator />
	    <menuitem action='Quit' />
	  </menu>
	  <menu action='Edit'>
	    <menuitem action='Undo' />
	    <menuitem action='Redo' />
	    <separator />
	    <menuitem action='BlockProperties' />
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
	<toolbar name='ToolBar'>
	</toolbar>
	</ui>
	'''
	
	def __init__(self,library,options=None):
		"""
		Initialise the application -- create all the widgets, and populate
		the icon palette.
		TODO: separate the icon palette into a separate method.
		"""
		self.ascwrap= library
		self.errorvars = []
		self.errorblocks = []
		self.status = gtk.Statusbar()
		self.temp = []
		self.act = 1 #to be used for storing canvas zoom in/out related data.
		# the Gaphas canvas
		canvas = BlockCanvas()

		# the main window
		gtk.Window.__init__(self)
		self.iconok = self.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconinfo = self.render_icon(gtk.STOCK_DIALOG_INFO,gtk.ICON_SIZE_MENU)
		self.iconwarning = self.render_icon(gtk.STOCK_DIALOG_WARNING,gtk.ICON_SIZE_MENU)
		self.iconerror = self.render_icon(gtk.STOCK_DIALOG_ERROR,gtk.ICON_SIZE_MENU)

		self.set_title("ASCEND Canvas Modeller")
		self.set_default_size(650,650)
		self.connect("destroy", gtk.main_quit)
		
		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join('..','glade','ascend.svg'))
		self.set_icon(windowicon.get_pixbuf())
		
		vbox = gtk.VBox()

		ui_manager = gtk.UIManager()
		accelgroup = ui_manager.get_accel_group()
		self.add_accel_group(accelgroup)
		
		actiongroup = gtk.ActionGroup('CanvasActionGroup')
		
		self.prefs = Preferences()
		
		actions = [('File', None, '_File')
		           ,('Quit', gtk.STOCK_QUIT, '_Quit', None,'Quit the Program', self.quit)
		           ,('New', gtk.STOCK_NEW,'_New',None,'Start a new Simulation', self.new)
		           ,('Open', gtk.STOCK_OPEN,'_Open',None,'Open a saved Canvas file', self.fileopen)
		           ,('Save', gtk.STOCK_SAVE,'_Save',None,'Open a saved Canvas file', self.save_canvas)
		           ,('SaveAs', gtk.STOCK_SAVE_AS,'_Save As...',None,'Open a saved Canvas file', self.filesave)
		           ,('Export', gtk.STOCK_PRINT, '_Export SVG', None,'Quit the Program', self.export_svg_as)
		           ,('Library', gtk.STOCK_OPEN, '_Load Library...', '<Control>l','Load Library', self.load_library_dialog)
		           ,('Edit', None, '_Edit')
		           ,('Undo', gtk.STOCK_UNDO, '_Undo', '<Control>z', 'Undo Previous Action', self.undo_canvas)
		           ,('Redo',gtk.STOCK_REDO, '_Redo', '<Control>y', 'Redo Previous Undo', self.redo_canvas)
		           ,('BlockProperties',gtk.STOCK_PROPERTIES, '_Block Properties', None, 'Edit Block Properties', self.bp)
		           ,('Delete', gtk.STOCK_DELETE, '_Delete', 'Delete', 'Delete Selected Item', self.delblock)
		           ,('View', None, '_View')
		           ,('Fullscreen', gtk.STOCK_FULLSCREEN, '_Full Screen', 'F11', 'Toggle Full Screen', self.fullscrn)
		           ,('ZoomIn', gtk.STOCK_ZOOM_IN, '_Zoom In', None, 'Zoom In Canvas', self.zoom)
		           ,('ZoomOut', gtk.STOCK_ZOOM_OUT, '_Zoom Out', None, 'Zoom Out Canvas', self.zoom)
		           #,('BestFit', gtk.STOCK_ZOOM_FIT, '_Best Fit', None, 'Best Fit Canvas',self.zoom)
		           ,('Tools', None, '_Tools')
		           ,('Debug', None, '_Debug', None, 'View Instance Browser',self.debug_canvas)
		           ,('Run', gtk.STOCK_EXECUTE, '_Run', None, 'Solve Canvas', self.run_canvas)
		           ,('Preview', gtk.STOCK_PRINT_PREVIEW, '_Preview', None, 'Preview Generated Code', self.preview_canvas)
		           ,('Help', None, '_Help')
		           ,('Development', gtk.STOCK_INFO, '_Development', None, 'Check Development', self.on_get_help_online_click)
		           ,('ReportBug', None, '_Report Bug', None, 'Report a Bug!', self.on_report_a_bug_click)
		           ,('About', gtk.STOCK_ABOUT, '_About', None, 'About Us', self.about)
		           ]
		
		actiongroup.add_actions(actions)
		
		ui_manager.insert_action_group(actiongroup,0)
		ui_manager.add_ui_from_string(self.menu_xml)
		
		#Creating Menu Bar
		menubar = ui_manager.get_widget('/MenuBar')
		vbox.pack_start(menubar,False,False)
		
		#Creating Tool Bar
		toolbar = ui_manager.get_widget('/ToolBar')
		vbox.pack_start(toolbar,False,False)
		
		'''The Toolbar Definations start here'''
		
		tb = gtk.Toolbar()	
		#Load Button
		loadbutton = gtk.ToolButton(gtk.STOCK_OPEN)
		loadbutton.connect("clicked",self.fileopen)
		tb.insert(loadbutton,0)
		
		#Save Button
		savebutton = gtk.ToolButton(gtk.STOCK_SAVE)
		savebutton.connect("clicked",self.save_canvas)
		tb.insert(savebutton,1)
		
		#Debug Button
		debugbutton = gtk.ToolButton(gtk.STOCK_PROPERTIES)
		debugbutton.set_label("Debug")
		debugbutton.connect("clicked",self.debug_canvas)
		tb.insert(debugbutton,2)
		
		#Preview Button
		previewb = gtk.ToolButton(gtk.STOCK_PRINT_PREVIEW)
		previewb.set_label("Preview")
		previewb.connect("clicked",self.preview_canvas)
		tb.insert(previewb,3)
		
		#Export Button
		exportbutton = gtk.ToolButton(gtk.STOCK_CONVERT)
		exportbutton.set_label("Export SVG")
		exportbutton.connect("clicked",self.export_svg_as)
		tb.insert(exportbutton,2)
		
		#Run Button
		runb = gtk.ToolButton(gtk.STOCK_EXECUTE)
		runb.set_label("Run")
		runb.connect("clicked",self.run_canvas)
		tb.insert(runb, 4)
		
		##Custom Entry
		#m_entry = gtk.ToolButton(gtk.STOCK_SAVE_AS)
		#m_entry.set_label("Custom METHOD")
		#m_entry.connect("clicked",self.custommethod)
		#tb.insert(m_entry,5)
		
		vbox.pack_start(tb, False, False)

		# hbox occupies top part of vbox, with icons on left & canvas on right.
		paned = gtk.HPaned()
		
		# the 'view' widget implemented by Gaphas
		#gaphas.view.DEBUG_DRAW_BOUNDING_BOX = True
		self.view = GtkView()	
		self.view.tool =  BlockToolChain()

		# table containing scrollbars and main canvas 
		t = gtk.Table(2,2)
		self.view.canvas = canvas
		self.view.zoom(1)
		self.view.set_size_request(600, 450)
		hs = gtk.HScrollbar(self.view.hadjustment)
		vs = gtk.VScrollbar(self.view.vadjustment)
		t.attach(self.view, 0, 1, 0, 1)
		t.attach(hs, 0, 1, 1, 2, xoptions=gtk.FILL, yoptions=gtk.FILL)
		t.attach(vs, 1, 2, 0, 1, xoptions=gtk.FILL, yoptions=gtk.FILL)

		# a scrolling window to contain the icon palette
		self.scroll = gtk.ScrolledWindow()
		self.scroll.set_border_width(2)
		self.scroll.set_shadow_type(gtk.SHADOW_ETCHED_IN)

		self.scroll.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
	
		self.blockiconview = BlockIconView(blocks=self.ascwrap.canvas_blocks,app=self)
		self.scroll.add(self.blockiconview)
		self.reporter = self.ascwrap.reporter
		
		paned.pack1(self.scroll, True, True)
		paned.pack2(t, True, True)
		vbox.pack_start(paned, True, True)
		vpane = gtk.VPaned()
		vpane.pack1(vbox)
		lower_vbox = gtk.VBox()
		
		self.ET = errorreporter.ErrorReporter( self.reporter,self.iconok,self.iconinfo,self.iconwarning,self.iconerror)
		self.notebook = gtk.Notebook()
		self.notebook.set_tab_pos(gtk.POS_TOP)
		label = gtk.Label('Error / Status Reporter Console')
		scrolledwindow = gtk.ScrolledWindow()
		scrolledwindow.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		scrolledwindow.add(self.ET.errorview)
		self.notebook.append_page(scrolledwindow, label)
		lower_vbox.pack_start(self.notebook,True, True)
		lower_vbox.pack_start(self.status, False, False)
		vpane.pack2(lower_vbox)
		
		self.undo_manager = undo.undoManager(self)
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
		label = blocktype.type.getName()
		def my_block_factory():
			def wrapper():
				b = BlockInstance(blocktype)
				bi = DefaultBlockItem(b)
				self.view.canvas.add(bi)
				return bi
			return wrapper
		self.view.unselect_all()
		self.view.tool.grab(PlacementTool(self.view,my_block_factory(), HandleTool(), 2))
		self.status.push(0,"Selected '%s'..." % blocktype.type.getName())

	def set_connector_tool(self,foobar):
		"""
		Prepare the canvas for drawing a connecting line (note that one can
		alternatively just drag from a port to another port).
		"""
		
		def my_line_factory():
			def wrapper():
				l =  BlockLine()
				self.view.canvas.add(l)
				return l
			return wrapper
		self.view.tool.grab(PlacementTool(self.view,my_line_factory(), HandleTool(), 1))
		
	@undo.block_observed
	def delblock(self, widget = None):
		if self.view.focused_item:
			self.view.canvas.remove(self.view.focused_item)
			self.status.push(0,"Item deleted.")
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
			
	@undo.block_observed		
	def new(self, widget):
		for item in self.view.canvas.get_all_items():
			self.view.canvas.remove(item)
		self.reporter.reportNote('Canvas cleared for new Model')
		self.view.canvas.filestate = 0
		self.view.canvas.filename = None	
		self.view.canvas.canvasmodelstate = 'Unsolved'
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)
		self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
		
	def debug_canvas(self,widget):
		"""
		Display an 'object browser' to allow inspection of the objects
		in the canvas.
		"""

		b = obrowser.Browser("canvas",self.view.canvas, False)
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)

	def save_canvas(self,widget):
		"""
		Save the canvas in 'pickle' format. Currently saving is jointly handled by both self.save_canvas and self.filesave methods
		"""
		if not self.view.canvas.filestate:
			self.filesave(widget)
			return
		else:	
			f = file(self.view.canvas.filename,"w")
		try:
			pickle.dump(self.view.canvas,f)
			self.status.push(0,"Canvasmodel saved...")		
		except Exception,e:
			print "ERROR:",str(e)
			self.status.push(0,"Canvasmodel could not be saved : " + str(e))
			b = obrowser.Browser("canvas",self.view.canvas)
			d = gtk.Dialog("Error",self,gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
			d.vbox.add(gtk.Label(str(e)))
			d.show_all()
			d.run()
			d.hide()
		finally:
			f.close()
	
	def load_canvas(self,widget):
		"""
		Restore a saved canvas in 'pickle' format. Currently not in use as loading now handled by self.fileopen().
		"""
		f = file("./test.a4b","r")
		try:
			self.view.canvas = pickle.load(f)
			if self.view.canvas.model_library is not None:
				print "Loading Library...."
				self.loadlib(self, self.view.canvas.model_library,0)
			self.view.canvas.reattach_ascend(self.ascwrap.library,self.ascwrap.annodb)
			self.view.canvas.update_now()
			self.status.push(0,"Canvasmodel sucessfully loaded...")
		except Exception,e:
			self.status.push(0,"Canvasmodel could not be loaded : " + str(e))
			self.reporter.ReportError("Canvasmodel could not be loaded : " + str(e))	
			self.reporter.ReportNote(" Error occured while attempting to 'Load' the file")
		finally:
			f.close()
		self.load_presaved_canvas(None)			
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)	
	

	def temp_canvas(self,widget):
		pass
		"""
		NOTE: 	re-implementation needed	
		"""	
	
	def undo_canvas(self,widget):
		"""
		Undo

		"""
		self.undo_manager.undo()
		#pass
		
	def redo_canvas(self,widget):
		"""
		NOTE: 	re-implementation needed	

		"""
		self.undo_manager.redo()
		#pass
		
	def preview_canvas(self,widget):
		"""
		Output an ASCEND representation of the canvas on the commandline.
		Under development.
		"""
		self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)
		#info.Info(self.view.parent.parent.parent.parent.parent,str(self.view.canvas),"Canvas Preview").run()
		canvasproperties.CanvasProperties(self).run()
		
	def export_svg_as(self,widget):
		
		f = None
		dialog = gtk.FileChooserDialog("Export Canvas As...", None,
                                       gtk.FILE_CHOOSER_ACTION_SAVE, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)
		filter = gtk.FileFilter()
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
		
		if response == gtk.RESPONSE_OK:
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
			surface = cairo.SVGSurface(fn , w, h)
			cr = cairo.Context(surface)
			svgview.matrix.translate(-svgview.bounding_box.x, -svgview.bounding_box.y)
			svgview.paint(cr)
			cr.show_page()
			surface.flush()
			surface.finish()
			self.reporter.reportNote(" File ' %s ' saved successfully." % name )
			self.status.push(0,"Wrote SVG file '%s'." % fn)
		dialog.destroy()
		
		
	def export_svg(self,widget):	
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

		self.status.push(0,"Wrote SVG file '%s'." % fn)
		
	#def run_presaved_canvas(self,widget):
		##TODO: Separate
		#if self.view.canvas.saved_model is not None:
			#model = self.view.canvas.saved_model 
			#self.ascwrap.library.loadString(model,"canvasmodel")
			
		T = self.ascwrap.library.findType("canvasmodel")
		M = T.getSimulation('canvassim',True)
		M.setSolver(ascpy.Solver("QRSlv"))
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())

			#for item in self.view.canvas.get_all_items():
				#if hasattr(item, 'blockinstance'):
					#bi = item.blockinstance
					#for i in self.ascwrap.library.modules.getChildren()[0].getChildren():
						#if str(bi.name) == str(i.getName()):
							#bi.instance = i
							
	def load_presaved_canvas(self,widget):
		#TODO: Separate
		print self.view.canvas.saved_data
		try:
			if self.view.canvas.saved_data is not None:
				model = str(self.view.canvas)
				self.ascwrap.library.loadString(model,"canvasmodel")
				T = self.ascwrap.library.findType("canvasmodel")
				M = T.getSimulation("canvassim",True)
				
				def assignval(sim_inst, name):	
					if sim_inst.isAtom():
						try:
							sim_inst.setRealValue(self.view.canvas.saved_data[name])
						except Exception,e:
							print e	
					elif sim_inst.isRelation():
						pass	
					else:
						for i in sim_inst.getChildren():
							k = name+"."+str(i.getName())
							assignval(i,k)
							
				for i in M.getChildren()[0].getChildren():
					assignval(i, str(i.getName()))
					
				for item in self.view.canvas.get_all_items():
					if hasattr(item, 'blockinstance'):
						bi = item.blockinstance
						for i in M.getChildren()[0].getChildren():
							if str(bi.name) == str(i.getName()):
								bi.instance = i
				self.status.push(0,"Canvasmodel state : %s"% self.view.canvas.canvasmodelstate)	
		except AttributeError:					
			self.status.push(0,"Canvasmodel state : Unsolved")
			
	def run_canvas(self,widget):
		#TODO: Separate
		"""
		Exports canvas to ASCEND solver and attempts to solve it. Also provides realtime feedback.
		"""
		self.undo_manager.reset()
		model = str(self.view.canvas)
		#print model
		self.view.canvas.saved_model = model
		self.view.canvas.saved_data = {}
	
		self.ascwrap.library.loadString(model,"canvasmodel")
		T = self.ascwrap.library.findType("canvasmodel")
		
		
		self.M = T.getSimulation("canvassim",True)
		self.M.setSolver(ascpy.Solver("QRSlv"))
		self.reporter = ascpy.getReporter()
		reporter = PopupSolverReporter(self,self.M.getNumVars())
		
		try:
			self.M.build()
		except RuntimeError,e:
			print "Couldn't build system: %s" % str(e)
			self.status.push(0,"Couldn't build system: %s" % str(e));
			return
		#try:
			#met=T.getMethod('on_load')
			#self.M.run(met)
		#except Exception,e:
			#print "Couldn't run on_load method: %s"% str(e)
			#return
		
		self.status.push(0,"Solving with 'QRSlv'. Please Wait...")
		
		try:
			self.M.solve(ascpy.Solver("QRSlv"),reporter)
			self.reporterrorblock(self.M)
		except RuntimeError:
			self.reporterrorblock(self.M)
			
		for item in self.view.canvas.get_all_items():
			if hasattr(item, 'blockinstance'):
				bi = item.blockinstance
				for i in self.M.getChildren()[0].getChildren():
					if str(bi.name) == str(i.getName()):
						bi.instance = i

		#Storing solved variables into a dictionary which will be pickled
		for i in self.M.getallVariables():
			self.view.canvas.saved_data[str(i.getName())] = i.getValue()						
			
	def about(self, widget):
        	about = gtk.AboutDialog()
        	about.set_program_name("ASCEND CANVAS")
        	about.set_version("0.9.6x alpha")
        	about.set_copyright("Carnegie Mellon University")
        	about.set_comments("Canvas - Based GUI Modeller for Energy Systems")
        	about.set_website("http://www.ascend.cheme.cmu.edu")
		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
        	about.set_icon(windowicon.get_pixbuf())
		about.set_logo(gtk.gdk.pixbuf_new_from_file("../glade/ascend.png"))
        	about.run()
        	about.destroy()			   
			   
	def dummy(self, widget):
		dum = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "Sorry ! This fuctionality is not implemented yet.")
        	dum.run()
        	dum.destroy() 
	
	def fileopen(self, widget):
		dialog = gtk.FileChooserDialog("Open..",self,gtk.FILE_CHOOSER_ACTION_OPEN, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))
     		dialog.set_default_response(gtk.RESPONSE_OK)
		filter = gtk.FileFilter()
		filter.set_name("Canvas Files")
		filter.add_mime_type("Canvas Files/a4b")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		filter = gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		res = dialog.run()
		if res == gtk.RESPONSE_OK:
			result = dialog.get_filename()
			self.load_canvas_file(result)
		dialog.destroy()
				
	def load_canvas_file(self,filename):
		#TODO: Separate
		f = file(filename,"r")
		try:
			self.view.canvas = pickle.load(f)
			if self.view.canvas.model_library is not None:
				self.loadlib(self, self.view.canvas.model_library)
			self.view.canvas.reattach_ascend(self.ascwrap.library,self.ascwrap.annodb)
			self.view.canvas.update_now()
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))
			self.load_presaved_canvas(None)
			self.reporter.reportError(" File %s successfully loaded." % filename)
			self.status.push(0,"File %s Loaded." % filename)
			self.view.canvas.filename = filename
		except Exception,e:
				self.reporter.reportError(" Error occured while attempting to 'Load' the file. File could be loaded properly.")
				print e
		finally:
			f.close()
			
	def filesave(self, widget):
	 	f = None
		dialog = gtk.FileChooserDialog("Save..", self, gtk.FILE_CHOOSER_ACTION_SAVE, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)
		if self.view.canvas.filestate == 0:
			dialog.set_filename("untitled")
		filter = gtk.FileFilter()
		filter.set_name("Canvas File")
		filter.add_pattern("*.a4b")
		dialog.add_filter(filter)
		dialog.show()
		dialog.set_do_overwrite_confirmation(True)
		#dialog.connect("confirm-overwrite", self.confirm_overwrite_callback)
		response = dialog.run()
		if response == gtk.RESPONSE_OK:
			name = dialog.get_filename()
			if '.a4b' not in name:
				name += '.a4b'
			if f == None and f != name:
				f = open(name, 'w')
			try:
				pickle.dump(self.view.canvas,f)
				self.reporter.reportNote(" File ' %s ' saved successfully." % name )
				self.status.push(0,"CanvasModel Saved.")
				self.view.canvas.filestate = 1
				self.view.canvas.filename = name
			except Exception,e:
				print "ERROR:",str(e)
				b = obrowser.Browser("canvas",self.view.canvas)
				d = gtk.Dialog("Error",self,gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
				d.vbox.add(gtk.Label(str(e)))
				d.show_all()
				d.run()
				d.hide()
			finally:
				f.close()
		dialog.destroy()

	def confirm_overwrite_callback(self, widget):

		uri = widget.get_filename()
		if is_uri_read_only(uri):
			if user_wants_to_replace_read_only_file (uri):
				return gtk.FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME
			else:
				return gtk.FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
		else:
			return gtk.FILE_CHOOSER_CONFIRMATION_CONFIRM 
		return
		
		
	def bp(self, widget = None):
		if self.view.focused_item:
			blockproperties.BlockProperties(self, self.view.focused_item).run()
		else:
			m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "No Block was selected! Please select a Block to view its properties.")
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
							
					except Exception,e:
						print e	
				elif sim_inst.isRelation():
					pass	
				else:
					for i in sim_inst.getChildren():
						k = name+"."+str(i.getName())
						checkifsolved(i,k)
		
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
						self.errortext += str(i.getName()) #+' --> ' + ' couldnot be solved'
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
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#F88'))
			flag = 0
		else:
			self.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#AFA'))	 
	
	def load_library_dialog(self,widget):
		#TODO: separate
		dialog = gtk.FileChooserDialog('Load Library...',self, gtk.FILE_CHOOSER_ACTION_OPEN,(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)
		dialog.set_current_folder(os.path.join(self.ascwrap.defaultlibraryfolder))

		filter = gtk.FileFilter()
		filter.set_name("Canvas Files")
		filter.add_mime_type("Canvas Files/a4c")
		filter.add_pattern("*.a4c")
		dialog.add_filter(filter)
		
		filter = gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		res = dialog.run()
		if res == gtk.RESPONSE_OK:
			result = dialog.get_filename()
			self.loadlib(lib_name = os.path.basename(result))
		dialog.destroy()
		
	
	def loadlib(self, widget = None, lib_name = None):
		##TODO: Separate
		#if loadcondition == 1:
			#if self.view.canvas.model_library == lib_name:
				#m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_WARNING, gtk.BUTTONS_CLOSE, "The selected Library is already loaded. ")
				#m.run()	
				#m.destroy()
				#return
			#if self.view.canvas.get_all_items():
				#m = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "Unable to switch Library ! The canvas contains models from present Model Library. ")
				#m.run()	
				#m.destroy()
				#return
			
		#print lib_path
		self.ascwrap.load_library(lib_name)
		
		self.view.canvas.model_library = lib_name
		
		self.scroll.remove(self.blockiconview)
		self.blockiconview = BlockIconView(self.ascwrap.canvas_blocks, self)
		self.scroll.add(self.blockiconview)
		self.show_all()
			
		#self.status.push(0, " Library '%s' loaded :: Found %d block types." %(lib_name, (len(blocks))))
		
	def get_libraries_from_folder(self,path=None):
		'Returns a dictionary of library names with their paths'
		if path == None:
			return
		libs = {}
		locs = glob.glob(os.path.join(path,'*.a4c'))
		for loc in locs:
			name = loc.strip(path)
			libs[name] = loc
		return libs
					
	def zoom(self, widget):
		zoom = {'ZoomIn':1.2,'ZoomOut':0.8,'BestFit':1.0}
		x = zoom[widget.get_name()]
		self.act = self.act*x
		self.view.zoom(x)
	
		
	def fullscrn(self, widget):
		try:
			if  self.flag == 1:
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
		_help = help.Help(url="http://ascendwiki.cheme.cmu.edu/Canvas_Development")
		_help.run()				
					
	def on_report_a_bug_click(self, widget):
		_help = help.Help(url="http://ascendbugs.cheme.cmu.edu/report/")
		_help.run()	
		
	def quit(self,args):
		del(self.prefs)
		self.destroy()
