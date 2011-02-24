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
			self.instance_box = xml.get_widget('instance')
			self.instance_model = modeltree.TreeView(parent.M)
			self.instance_box.add(self.instance_model.treeview)
			self.instance_model.treeview.show()
		except Exception as e:
			self.instance_box = xml.get_widget('instance')
			self.instance_label = gtk.Label()
			self.instance_box.add(self.instance_label)
			self.instance_label.set_text('Instance not Built, Solve the Canvas Model first!')
			self.instance_label.show()
		
		OK_button = xml.get_widget('ok')
		OK_button.connect('clicked',self.save_changes)
		OK_button.grab_default()
	
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