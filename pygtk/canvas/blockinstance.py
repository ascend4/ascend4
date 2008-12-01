from blocktype import *

blocknameindex = {}

class BlockInstance:
	"""
	Instance of a block of a particular type. This block will have 
	associated with it a particular name, and perhaps eventually some
	specified values of its defined parameters.
	"""

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

	def get_default_name(self):
		n = str(self.blocktype.type.getName())
		print blocknameindex
		print "the name is:",n
		if not blocknameindex.has_key(n):
			print "the key '%s' is not in blocknameindex" % n

		return "%s%s" % (n, blocknameindex[n])

	def __str__(self):
		return "\t%s IS_A %s;\n" % (self.name, self.blocktype.type.getName())

