from gaphas.tool import Tool
import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import blockproperties
import canvasproperties
import undo


class ContextMenuTool(Tool):
	"""
	Context menu for blocks and connectors on the canvas, intended to be
	the main mouse-based way by which interaction with blocks occurs (blocks
	renamed, parameters specified, values examined, etc).
	Code for performing these tasks should not be here; this should just
	hook into the appropriate code in the Application layer.
	"""
	def __init__(self,view=None):
		super(ContextMenuTool, self).__init__(view)

	def on_button_press(self, event):
		context = self.view.tool
		if event.button != 3:
			context.ungrab(self.view.tool)
			return False

		menu = Gtk.Menu()
		menu.connect("deactivate",self.deactivate,context)
		'''
		menublockstreams = Gtk.MenuItem("_Streams")
		menublockstreams.connect("activate",self.setstream, window, context, context.view.hovered_item)
		menu.add(menublockstreams)
		'''
		menurename = Gtk.MenuItem("Re_name",True)
		window = context.view.parent.parent.parent.parent.parent
		menurename.connect("activate",self.rename,context.view.hovered_item,window)
		menu.add(menurename)
		menudefault.set_sensitive(False)
		'''menublockstreams.set_sensitive(False)'''

		menudelete = Gtk.MenuItem("_Delete",True)
		menudelete.connect("activate",self.delete,context.view.hovered_item,context.view)
		menu.add(menudelete)

		menu.add(Gtk.SeparatorMenuItem())

		menurotate_clock = Gtk.MenuItem("_Rotate_clockwise",True)
		window = context.view.parent.parent.parent.parent.parent
		menurotate_clock.connect("activate",self.blockrotate_clock,context.view.hovered_item,window)
		menu.add(menurotate_clock)

		menurotate_anti = Gtk.MenuItem("_Rotate_anti_clockwise",True)
		window = context.view.parent.parent.parent.parent.parent
		menurotate_anti.connect("activate",self.blockrotate_anti,context.view.hovered_item,window)
		menu.add(menurotate_anti)

		menuflip = Gtk.MenuItem("Flip",True)
		window = context.view.parent.parent.parent.parent.parent
		menuflip.connect("activate",self.blockflip,context.view.hovered_item,window)
		menu.add(menuflip)

		menu.add(Gtk.SeparatorMenuItem())

		menublockproperties = Gtk.MenuItem("_Properties")
		menublockproperties.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 0)
		menu.add(menublockproperties)

		menublockparams = Gtk.MenuItem("_Parameters")
		menublockparams.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 1)
		menu.add(menublockparams)

		menudefault = Gtk.MenuItem("_Set Default Values")
		menudefault.connect("activate",self.defaultvalues,window ,context,context.view.hovered_item)
		menu.add(menudefault)

		#menublockmethod = Gtk.MenuItem("_Custom Method(s)")
		#menublockmethod.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 2)
		#menu.add(menublockmethod)

		menublockinstance = Gtk.MenuItem("_Instance")
		menublockinstance.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 3)
		menu.add(menublockinstance)
		'''
		menublockstreams = Gtk.MenuItem("_Streams")
		menublockstreams.connect("activate",self.setstream, window, context, context.view.hovered_item)
		menu.add(menublockstreams)
			'''
		#menuinfo = Gtk.MenuItem("_Info",True)
		#menuinfo.connect("activate",self.info,window,context,context.view.hovered_item)
		#menu.add(menuinfo)

		menu.add(Gtk.SeparatorMenuItem())

		menucanvas = Gtk.MenuItem("_Canvas Properties",True)
		menucanvas.connect("activate",self.canvasproperties,window ,context)
		menu.add(menucanvas)

		if not context.view.hovered_item:
			menurename.set_sensitive(False)
			#menuinfo.set_sensitive(False)
			menudelete.set_sensitive(False)
			menurotate_clock.set_sensitive(False)
			menurotate_anti.set_sensitive(False)
			menuflip.set_sensitive(False)
			menublockproperties.set_sensitive(False)
			menublockinstance.set_sensitive(False)
			#menublockmethod.set_sensitive(False)
			menublockparams.set_sensitive(False)
			menudefault.set_sensitive(False)
			'''menublockstreams.set_sensitive(False)'''


		if not hasattr(context.view.hovered_item,'blockinstance'):
			menurename.set_sensitive(False)
			#menuinfo.set_sensitive(False)

		if context.view.hovered_item:
			if not context.view.hovered_item.blockinstance.instance:
				menublockinstance.set_sensitive(False)

		menu.show_all()
		menu.popup( None, None, None, event.button, event.time)
		#self.view.tool.ungrab(self.view.tool)
		return False

	#def on_button_release(self, event):
		#print event
		#self.view.hovered_item = None
		#return False

	def on_double_click(self,event):
		context = self.view.tool
		if event.button != 1 or not context.view.hovered_item:
			context.ungrab(self.view.tool)
			return False

		window = context.view.parent.parent.parent.parent.parent
		self.blockproperties(None , window, context, context.view.hovered_item)

	def deactivate(self,widget,context):
		#print 'Hovered Item Set To None'
		context.view.hovered_item = None

	def rename(self,widget,item,window):
		if hasattr(item,'blockinstance'):
			bi = item.blockinstance
			dia = Gtk.Dialog("Rename %s '%s'" % (bi.blocktype.type.getName(),bi.name), window,Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,(Gtk.STOCK_CANCEL, 0, Gtk.STOCK_OK, Gtk.ResponseType.OK))
			dia.set_default_response(Gtk.ResponseType.OK)
			ent = Gtk.Entry()
			ent.set_text(bi.name)
			def key_press(ent,event):
				key = Gdk.keyval_name(event.keyval)
				if key == 'Return':
					dia.response(Gtk.ResponseType.OK)
					return True
				return False
			ent.connect("key-press-event",key_press)
			dia.vbox.add(ent)
			dia.show_all()
			res = dia.run()
			if res == Gtk.ResponseType.OK:
				bi.name = ent.get_text()
			dia.destroy()

	@undo.block_observed
	def delete(self,widget,item,view):
		 view.canvas.remove(item)

	def blockproperties(self, widget = None, window = None, context = None, item = None, tab = None):
		 blockproperties.BlockProperties(window, item, tab = tab).run()

	def blockrotate_clock(self, widget, item, window):
		''' handler for rotating an item '''
		item.rotate_clock()

	def blockrotate_anti(self,widget, item, window):
		item.rotate_anti()

	def blockflip(self,widget, item, window):
		item.flip()

	def canvasproperties(self, widget, window, context):
		canvasproperties.CanvasProperties(window).run()
	'''
	def setstream(self, widget, window, context, item):
		print window.ascwrap.streams
	'''
	def defaultvalues(self,widget,window,context,item):
		print widget,window,context,item.blockinstance

		model = str(self.view.canvas)
		#print model
		self.view.canvas.saved_model = model
		self.view.canvas.saved_data = {}

		window.ascwrap.library.loadString(model,"canvasmodel")
		t = window.ascwrap.library.findType(str(item.blockinstance.blocktype.type.getName()));
		try:
			m =t.getMethod('default_self');
		except:
			return
		i = t.getSimulation('sim',False);
		i.build()
		i.run(m);
		fv = i.getFixedVariables();
		for i in fv:
			for param in item.blockinstance.params:
				if param == i.getName():
					item.blockinstance.params[param].value = i.getValue();
					item.blockinstance.params[param].fix = True
