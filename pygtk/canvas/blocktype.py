import pygtk
pygtk.require('2.0') 
import gtk
import ascpy
import os.path

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
		self.params = []
		#self.duals = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [n]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [n]
			#elif t[0:min(len(t),5)]=="dual:":
			#	self.duals += [n]
			elif t[0:min(len(t),6)]=="param:":
				self.params += [n]
				
		self.iconfile = None
		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("icon"))
		if nn:
			n = nn[0].getText()
			if os.path.exists(n):
				self.iconfile = n
		
	def get_icon(self, width, height):
		"""
		Get a pixbuf representation of the block for use in the block palette
		(or possibly elsewhere)
		"""
		f = self.iconfile
		if self.iconfile is None:
			f = "defaultblock.svg"
		return gtk.gdk.pixbuf_new_from_file_at_size(f,width,height)

	def __getstate__(self):
		print "GET STATE ON BLOCKTYPE %s" % self.type.getName()
		return (str(self.type.getName()),len(self.inputs), len(self.outputs))

	def __setstate__(self, state):
		print "SET STATE ON BLOCKTYPE"
		(typename,ninputs,noutputs) = state
		print "Recreating type '%s' with %d inputs, %d outputs" % (typename,ninputs,noutputs)
		self.type = None
		self.notesdb = None
		self._typename = typename
		self.inputs = range(ninputs)
		self.outputs = range(noutputs)
		#self.instance = instance
		#self.duals = range(nduals)
		print "outputs =", self.outputs

	def reattach_ascend(self,library,  notesdb):
		self.type = library.findType(self._typename)

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))

		self.inputs = []
		self.outputs = []
		self.params = []
		#self.duals = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [n]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [n]
		#	elif t[0:min(len(t),4)]=="dual:":
		#		self.duals += [n]
			elif t[0:min(len(t),6)]=="param:":
				self.params += [n]
	
		print "Reattached type '%s', with %d inputs, %d outputs" % (self.type.getName(), len(self.inputs), len(self.outputs))		

	def get_input_name(self, index):
		return self.inputs[index].getText()

	def get_output_name(self, index):
		return self.outputs[index].getText()

	#def get_dual_name(self, index):
	#	return self.duals[index].get_text()
	