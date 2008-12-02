from gaphas.item import Line

class LineInstance:
	"""
	Class representation of the ASCEND 'ARE_THE_SAME' relationship
	signified by a ConnectorLine on the BlockCanvas.

	There will be a reference to a LineInstance stored within a
	LineItem object.

	The intention is that it should be possible to encapsulate all the 
	non-graphical information about a canvas model using a number of
	BlockInstance and LineInstance objects, and no Gaphas objects. It
	should be possible to pickle this collection of objects and have everything
	that is necessary for passing the model to ASCEND -- this provides the
	clean separation of ASCEND-related and Gaphas-releated code, hopefully.
	"""

	def __init__(self):
		self.connectedblocks = []

	def __str__(self):
		"""
		Representation of this line requires the name object from the 
		BlockInstance as well as name associated with the connected Port
		(still a problem).
		"""
		try:
			return ("\t%s, %s ARE_THE_SAME;\n" % (li.fromblock.name, li.toblock.name))
		except:
			return ""

