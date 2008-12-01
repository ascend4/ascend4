
# This class is not yet used, but should be.

class LineInstance:
	"""
	Class representation of the ASCEND 'ARE_THE_SAME' relationship
	signified by a ConnectorLine on the BlockCanvas.
	"""

	def __init__(self):
		self.fromblock = None
		self.toblock = None

	def __str__(self):
		s += ("\t%s, %s ARE_THE_SAME;\n" % (li.fromblock.name, li.toblock.name))
