from blocktype import *

blocknameindex = {}

PORT_IN = 0
PORT_OUT = 1

class BlockInstance:
	"""
	Application-layer representation of an instance of a Block on the Canvas.
	Includes a reference to the BlockType as well as the block's name.
	Eventually, the instance will include a reference to the corresponding
	ASCEND object, allowing results from the ASCEND solver to be inspected via
	the Canvas GUI.
	"""
	# FIXME still need some way of setting *parameters* associated with a block.

	def __init__(self,blocktype,name=None):
		self.blocktype = blocktype;
		n = str(blocktype.type.getName())
		if not blocknameindex.has_key(n):
			blocknameindex[n] = 0
		blocknameindex[n] += 1
		print "Block index for %s is %d" % (n, blocknameindex[n])
		if name is None:
			name = self.get_default_name()
		self.name = name
		# ASCEND reference:
		self.instance = None

		self.ports = {}
		for n in self.blocktype.inputs:
			t = n.getId()
			# TODO record that it's an input port
			self.ports[t] = PortInstance(self,t, PORT_IN)
		for n in self.blocktype.outputs:
			t = n.getId()
			# TODO record that it's an output port
			self.ports[t] = PortInstance(self,t, PORT_OUT)

	def get_default_name(self):
		n = str(self.blocktype.type.getName())
		print blocknameindex
		print "the name is:",n
		if not blocknameindex.has_key(n):
			print "the key '%s' is not in blocknameindex" % n

		return "%s%s" % (n, blocknameindex[n])

	def __str__(self):
		return "\t%s IS_A %s;\n" % (self.name, self.blocktype.type.getName())

class PortInstance:
	"""
	Application-layer representation of a Port, which is a variable inside a 
	MODEL which is connectable using the Canvas GUI. Class includes the name of
	the variable represented by the Port, but no type information, as that is
	currently difficult to extract from the ASCEND API.
	"""
	def __init__(self,blockinstance,name, type):
		self.blockinstance = blockinstance
		self.name = name
		self.type = type

class LineInstance:
	def __init__(self,fromport=None,toport=None):
		self.fromport = fromport
		self.toport = toport

	def __str__(self):
		"""
		Create a string for use in MODEL export.
		"""	
		fromname = "%s.%s" % (self.fromport.blockinstance.name, self.fromport.name)
		toname = "%s.%s" % (self.toport.blockinstance.name, self.toport.name)
		return "\t%s, %s ARE_THE_SAME;\n" % (fromname, toname)

# TODO set up reversible properties...?


