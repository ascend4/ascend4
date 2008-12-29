from blocktype import *

blocknameindex = {}

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
	def __init__(self,blockinstance,name=None):
		self.blockinstance = blockinstance
		self.name = name
		# ASCEND reference:
		self.instance = None

class ConnectionInstance:
	def __init__(self,fromport,toport):
		self.fromport = fromport
		self.toport = toport



