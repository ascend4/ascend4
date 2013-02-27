import pygtk
import re

pygtk.require('2.0')
import gtk
import ascpy
import os.path
import cairo

class BlockType():
	"""
	All data associated with the MODEL type that is represented by a block.
	This includes the actual ASCEND TypeDescription as well as the NOTES that
	are found to represent the inputs and outputs for this block, as well as
	some kind of graphical representation(s) for the block. In the canvas-
	based GUI, there will need to be a Cairo-based represention, for drawing
	on the canvas, as well as some other form, for creating the icon in the
	block palette.
	"""

	def __init__(self, typedesc, notesdb):
		self.type = typedesc
		self.notesdb = notesdb
		self.arrays = []
		self.gr = [] # this data structure is for Graphical Representation for custom icons
		self.port_in = {} # this list is for location of input ports in custom icons
		self.port_out = {} # this list is for location of output ports in custom icons
		self.arrays = []

		# FIXME BlockType should know what .a4c file to load in order to access
		# its type definition, for use in unpickling.
		self.sourcefile = None

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))

		self.inputs = []
		self.outputs = []
		self.params = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [[n.getId(),self.type.findMember(n.getId()),str(t)]]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [[n.getId(),self.type.findMember(n.getId()),str(t)]]
			elif t[0:min(len(t),6)]=="param:":
				self.params += [[n.getId(),self.type.findMember(n.getId()),str(t)]]

		self.iconfile = None
		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("icon"))
		if nn:
			n = nn[0].getText()
			if os.path.exists(n):
				self.iconfile = n

		self.name = None
		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("block"))
		if nn:
			self.name = nn[0].getText()


		#fetching the graphic string from model file, string manipulating it and storing it in
		#list of list
		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("graphic"))
		if nn:
			t = nn[0].getText().split("\n")
			for n in t:
				temp = n.split("-")
				ll = []
				for k in temp:
					tt = k.split(",")
					pp = []
					for q in tt:
						q = q.strip("\t")
						pp.append(q)
					ll.append(pp)
				self.gr.append(ll)

		self.iconfile = self.create_icon(48,48)

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("port_in"))
		if nn:
			n = nn[0].getText().split(" ")
			for m in n:
				tt = m.split("-")
				for k in tt:
					tpp = k.split(":")
					loc = tpp[1].split(",")
					xy = []
					xy.append(loc[0])
					xy.append(loc[1])
					self.port_in[str(tpp[0])] = xy

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("port_out"))
		if nn:
			n = nn[0].getText().split(" ")
			for m in n:
				tt = m.split("-")
				for k in tt:
					tpp = k.split(":")
					loc = tpp[1].split(",")
					xy = []
					xy.append(loc[0])
					xy.append(loc[1])
					self.port_out[str(tpp[0])] = xy

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("array"))
		for n in nn:
			if n:
				t = n.getText()
				self.arrays.append([n.getText(),self.type.findMember(n.getText())])
		#print self.arrays

	def get_icon(self, width, height):
		"""
		Get a pixbuf representation of the block for use in the block palette
		(or possibly elsewhere)
		"""
		f = self.iconfile
		if self.iconfile is None:
			f = "defaultblock.svg"
		return gtk.gdk.pixbuf_new_from_file_at_size(str(f),width,height)

	def create_icon(self,width,height):
		properties = self.gr
		if len(properties) == 0:
			return None
		fo = file("%s.svg"%self.name,'w')
		## Prepare a destination surface -> out to an SVG file!
		surface = cairo.SVGSurface (fo,width,height)
		c = cairo.Context (surface)
		for m in properties:
			c.move_to(float(m[0][0])*width*0.1,float(m[0][1])*height*0.1)
			for mm in m:
				c.line_to(float(mm[0])*width*0.1,float(mm[1])*height*0.1)
		c.stroke()
		surface.finish()
		return fo.name

	def __getstate__(self):
		state = self.__dict__.copy()
		state['type'] = str(self.type)
		state['notesdb'] = None
		state['inputs'] = []
		state['outputs'] = []
		state['params'] =  []
		#state['inputs'] = [[str(x) for x in self.inputs[i]] for i in range(len(self.inputs))]
		#state['outputs'] = [[str(x) for x in self.outputs[i]] for i in range(len(self.outputs))]
		#state['params'] =  [[str(x) for x in self.params[i]] for i in range(len(self.params))]
		return(state)

	def __setstate__(self, state):
		self.__dict__ = state

	def reattach_ascend(self,library, notesdb):
		self.type = library.findType(self.type)

		nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))

		self.inputs = []
		self.outputs = []
		self.params = []
		for n in nn:
			t = n.getText()
			if t[0:min(len(t),3)]=="in:":
				self.inputs += [[n.getId(),self.type.findMember(n.getId()),str(t)]]
			elif t[0:min(len(t),4)]=="out:":
				self.outputs += [[n.getId(),self.type.findMember(n.getId()),str(t)]]
			elif t[0:min(len(t),6)]=="param:":
				self.params += [[n.getId(),self.type.findMember(n.getId()),str(t)]]

		print "Reattached type '%s', with %d inputs, %d outputs" % (self.type.getName(), len(self.inputs), len(self.outputs))

	def get_input_name(self, index):
		return self.inputs[index].getText()

	def get_output_name(self, index):
		return self.outputs[index].getText()
