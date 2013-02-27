'''
Canvas Properties Dialog Pop Up 
Author: Grivan Thapar, July 2010
'''
import gtk, gobject, os, pango,gtk.glade
import gtksourceview2 as gtksourceview
import modeltree

class CanvasProperties(object):
	'''
	Pop-Up window for viewing and editing general Canvas properties
	'''
	def __init__(self,parent):

		##Get the XML Config##
		glade_file_path = os.path.join('..','glade','canvasproperties.glade')
		xml = gtk.glade.XML(glade_file_path,root = None,domain="",typedict={})	
		
		self.dialog = xml.get_widget('dialog')
		self.parent = parent
		
		##Get the ASCEND lang##
		self.sourceviewLangman = gtksourceview.language_manager_get_default()
		op = self.sourceviewLangman.get_search_path()
		if os.path.join('..','..','tools','gtksourceview-2.0') not in op:
			op.append(os.path.join('..','..','tools','gtksourceview-2.0'))
			self.sourceviewLangman.set_search_path(op)
		self.sourceviewLang = self.sourceviewLangman.get_language('ascend')
		
		##Preview Tab##
		scrolledwindow = xml.get_widget('scrolledwindow1')
		self.sourceviewView = gtksourceview.View()
		self.sourceviewView.set_editable(False)
		#self.sourceviewView.set_auto_indent(True)
		self.sourceviewBuff = gtksourceview.Buffer()
		self.sourceviewBuff.set_language(self.sourceviewLang)
		self.sourceviewBuff.set_highlight_syntax(True)
		self.sourceviewBuff.set_text(str(self.parent.view.canvas))
		self.sourceviewView.set_buffer(self.sourceviewBuff)
		scrolledwindow.add(self.sourceviewView)
		self.sourceviewView.show()
		
		##Declaration Tab##
		scrolledwindow2 = xml.get_widget('scrolledwindow2')
		self.sourceviewView2 = gtksourceview.View()
		self.sourceviewView2.set_auto_indent(True)
		self.sourceviewBuffMethod = gtksourceview.Buffer()
		self.sourceviewBuffMethod.set_language(self.sourceviewLang)
		self.sourceviewBuffMethod.set_highlight_syntax(True)
		self.sourceviewBuffMethod.set_text(str(self.parent.view.canvas.user_code))
		self.sourceviewView2.set_buffer(self.sourceviewBuffMethod)
		scrolledwindow2.add(self.sourceviewView2)
		self.sourceviewView2.show()
	
		##Instance Tab##
		try:
			self.instance_box = xml.get_widget('instancescrolledwin')
			self.instance_model = modeltree.TreeView(parent.M)
			self.instance_box.add(self.instance_model.treeview)
			self.instance_model.treeview.show()
		except Exception as e:
			self.instance_box = xml.get_widget('instancescrolledwin')
			self.instance_label = gtk.Label()
			self.instance_box.add_with_viewport(self.instance_label)
			self.instance_label.set_text('Instance not Built, Solve the Canvas Model first!')
			self.instance_label.show()
		
		##Stream(s) tab##
		self.treeview = xml.get_widget('treeview1')
		self.stream_store = gtk.TreeStore(gobject.TYPE_PYOBJECT,gobject.TYPE_STRING,
		                                  gobject.TYPE_STRING)
		for stream in self.parent.ascwrap.streams:
			row = self.stream_store.append(None,[stream,str(stream),''])
			for prop in stream.stream_properties:
				self.stream_store.append(row,
				                         [stream,
				                          prop,
				                          stream.stream_properties[prop]])
		self.treeview.set_model(self.stream_store)
		self.draw_stream_view()
		
		OK_button = xml.get_widget('ok')
		OK_button.connect('clicked',self.save_changes)
		OK_button.grab_default()
	
	def draw_stream_view(self):
		self.name_render = gtk.CellRendererText()
		self.name_render.set_property('foreground-set',True)
		self.name_render.set_property('weight-set',True)
		
		self.prop_render = gtk.CellRendererText()
		self.prop_render.set_property('foreground-set',True)
		self.prop_render.set_property('weight-set',True)
		self.prop_render.set_property('editable',True)
		self.prop_render.connect('edited',self.set_stream_prop_callback)
		
		self.name_column = gtk.TreeViewColumn('Name',self.name_render,text=1,
		                                      foreground =4, weight=5)
		self.prop_column = gtk.TreeViewColumn('Value',self.prop_render,text=2,
		                                      foreground =4, weight=5)
		self.treeview.append_column(self.name_column)
		self.treeview.append_column(self.prop_column)
		
	def set_stream_prop_callback(self,combo,path,new_text):
		iter = self.stream_store.get_iter(path)
		stream = self.stream_store.get_value(iter,0)
		prop = self.stream_store.get_value(iter,1)
		if new_text == str(stream.stream_properties[prop]):
			return
		stream.stream_properties[prop]=new_text.split(',')
		self.stream_store.set_value(iter,2,new_text.split(','))
			
	def save_method_buffer(self,textbuffer):
		print textbuffer.get_modified()
	
	def run(self):
		self.dialog.run()
		self.dialog.hide()
		
	def save_changes(self,button):
		if self.sourceviewBuffMethod.get_modified():
			startiter = self.sourceviewBuffMethod.get_start_iter()
			enditer = self.sourceviewBuffMethod.get_end_iter()
			usertext = self.sourceviewBuffMethod.get_text(startiter, enditer)
			self.parent.view.canvas.user_code = usertext
			
if __name__ == '__main__':
	cp = CanvasProperties(None)
	cp.run()

# vim: set ts=4 noet:

