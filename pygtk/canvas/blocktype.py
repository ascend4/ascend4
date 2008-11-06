import pygtk
pygtk.require('2.0') 
import gtk
import ascpy

class BlockType:
	"""
	All data associated with the MODEL type that is represented by a block.
	This includes the actual ASCEND TypeDescription as well as the NOTES that
	are found to represent the inputs and outputs for this block, as well as
	some kind of graphical representation(s) for the block. In the canvas-
	based GUI, there will need to be a Cairo-based represention, for drawing
	on the canvas, as well as some other form, for creating the icon in the
	block palette.
	"""
	# FIXME there is no definition here of the canvas graphical representation,
	# but there should be.

	def __init__(self, typedesc, notesdb):
		self.type = typedesc
		self.notesdb = notesdb

		# FIXME BlockType should know what .a4c file to load in order to access
		# its type definition, for use in unpickling.
		self.sourcefile = None

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))

		self.inputs = []
		self.outputs = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [n]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [n]

	def get_icon(self, width, height):
		return gtk.gdk.pixbuf_new_from_file_at_size("defaultblock.svg",width,height)

