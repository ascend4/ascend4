from gaphas.tool import Tool
import pygtk
pygtk.require('2.0') 
import gtk

class ContextMenuTool(Tool):
	"""
	Context menu for blocks and connectors on the canvas, intended to be
	the main mouse-based way by which interaction with blocks occurs (blocks
	renamed, parameters specified, values examined, etc).
	Code for performing these tasks should not be here; this should just
	hook into the appropriate code in the Application layer.
	"""
	def __init__(self):
		pass

	def on_button_press(self, context, event):
		if event.button != 3:
			return False
		if context.view.hovered_item:
			menu = gtk.Menu()
			menurename = gtk.MenuItem("Re_name",True);
			window = context.view.parent.parent.parent.parent
			print window.__class__
			menurename.connect("activate",self.rename,context.view.hovered_item,window)
			menu.add(menurename)
			menudelete = gtk.MenuItem("_Delete",True);
			menudelete.connect("activate",self.delete,context.view.hovered_item,context.view)
			menu.add(menudelete)
			menu.show_all()		
			menu.popup( None, None, None, event.button, event.time)

	def rename(self,widget,item,window):
		if hasattr(item,'blockinstance'):
			bi = item.blockinstance
			dia = gtk.Dialog("Rename %s '%s'" 
				% (
					bi.blocktype.type.getName()
					,bi.name
				), window,gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT
				,(gtk.STOCK_CANCEL, 0, gtk.STOCK_OK, gtk.RESPONSE_OK)
			)
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

	def delete(self,widget,item,view):
		print "DELETING OBJECT"
		# TODO: add undo handler
		view.canvas.remove(item)

