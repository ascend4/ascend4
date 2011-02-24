
from gaphas.tool import Tool
import pygtk
pygtk.require('2.0') 
import gtk
import blockinstance
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
		
		menu = gtk.Menu()
		menu.connect("deactivate",self.deactivate,context)
		
		menurename = gtk.MenuItem("Re_name",True)
		window = context.view.parent.parent.parent.parent.parent
		menurename.connect("activate",self.rename,context.view.hovered_item,window)
		menu.add(menurename)

		menudelete = gtk.MenuItem("_Delete",True)
		menudelete.connect("activate",self.delete,context.view.hovered_item,context.view)
		menu.add(menudelete)

		menu.add(gtk.SeparatorMenuItem())
		
		menublockproperties = gtk.MenuItem("_Properties")
		menublockproperties.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 0)
		menu.add(menublockproperties)
		
		menublockparams = gtk.MenuItem("_Parameters")
		menublockparams.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 1)
		menu.add(menublockparams)
		
		#menublockmethod = gtk.MenuItem("_Custom Method(s)")
		#menublockmethod.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 2)
		#menu.add(menublockmethod)
		
		menublockinstance = gtk.MenuItem("_Instance")
		menublockinstance.connect("activate",self.blockproperties, window, context, context.view.hovered_item, 3)
		menu.add(menublockinstance)

		#menuinfo = gtk.MenuItem("_Info",True)
		#menuinfo.connect("activate",self.info,window,context,context.view.hovered_item)	
		#menu.add(menuinfo)
		
		menu.add(gtk.SeparatorMenuItem())
		
		menucanvas = gtk.MenuItem("_Canvas Properties",True)
		menucanvas.connect("activate",self.canvasproperties,window ,context)
		menu.add(menucanvas)
		
		if not context.view.hovered_item:
			menurename.set_sensitive(False)
			#menuinfo.set_sensitive(False)
			menudelete.set_sensitive(False)
			menublockproperties.set_sensitive(False)
			menublockinstance.set_sensitive(False)
			#menublockmethod.set_sensitive(False)
			menublockparams.set_sensitive(False)

			
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
			dia = gtk.Dialog("Rename %s '%s'" % (bi.blocktype.type.getName(),bi.name), window,gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_CANCEL, 0, gtk.STOCK_OK, gtk.RESPONSE_OK))
			dia.set_default_response(gtk.RESPONSE_OK)
			ent = gtk.Entry()
			ent.set_text(bi.name)
			def key_press(ent,event):
				key = gtk.gdk.keyval_name(event.keyval)
				if key == 'Return':
					dia.response(gtk.RESPONSE_OK)
					return True
				return False
			ent.connect("key-press-event",key_press)
			dia.vbox.add(ent)
			dia.show_all()
			res = dia.run()
			if res == gtk.RESPONSE_OK:
				bi.name = ent.get_text()
			dia.destroy()
			
	@undo.block_observed
	def delete(self,widget,item,view):
		view.canvas.remove(item)

	def blockproperties(self, widget = None, window = None, context = None, item = None, tab = None):
		blockproperties.BlockProperties(window, item, tab = tab).run()
		
	def canvasproperties(self, widget, window, context):
		canvasproperties.CanvasProperties(window).run()
