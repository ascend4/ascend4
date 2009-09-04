from blocktype import *

blocknameindex = {}

PORT_IN = 0
PORT_OUT = 1
PORT_INOUT = 11

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
		#blockproperties tab
		self.tab = 0
		# ASCEND reference:
		self.color_r = 1
		self.color_g = 1
		self.color_b = 1
		self.instance = 0
		self.ports = {}
		for n in self.blocktype.inputs:
			t = n.getId()
			# TODO record that it's an input port
			self.ports[t] = PortInstance(self,t, PORT_IN)
		for n in self.blocktype.outputs:
			t = n.getId()
			# TODO record that it's an output port
			self.ports[t] = PortInstance(self,t, PORT_OUT)
		#for n in self.blocktype.duals:
		#	t = n.getId()
		#	# TODO record that it's a bidirectional  port
		#	self.ports[t] = PortInstance(self,t, PORT_INOUT)

		print "CREATING PARAMETERS"
		self.params = {}
		#self.pfix = {}
		for n in self.blocktype.params:
			print n
			t = n.getId()
			self.params[t] = ParamInstance(self,t)
			#self.pfix[t] = 0
		print self.params	
			
		self.usercode = ""
			
	def get_default_name(self):
		n = str(self.blocktype.type.getName())
		print blocknameindex
		print "the name is:",n
		if not blocknameindex.has_key(n):
			print "the key '%s' is not in blocknameindex" % n

		return "%s%s" % (n, blocknameindex[n])

	def __str__(self):
		return "\t%s IS_A %s;\n" % (self.name, self.blocktype.type.getName())
	
	def __getstate__(self):
		#Return state values to pickle without  blockinstance.instance
		return (self.blocktype, self.name, self.tab, self.ports, self.params, self.usercode)
	
	def __setstate__(self, state):
		#Restore state values from pickle
		self.blocktype, self.name, self.tab, self.ports, self.params, self.usercode = state
		self.instance = None
		
				
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

class ParamInstance:
	"""
	Application-layer representation of a Parameter, which is a variable inside a 
	MODEL the value for which should be settable using the Canvas GUI. Currently
	we have no information about the type of the parameter (its units etc)
	because that data is still difficult to extract from the ASCEND API.
	"""
	def __init__(self,blockinstance,name):
		self.blockinstance = blockinstance
		self.name = name
		self.value = 0
		self.fix = 0

	def get_description(self):
		"""
		The parameter description (if it exists) is stored in the NOTE from the
		original MODEL file. This will go and get it (and saves us from storing
		it in the Pickle.
		"""
		# find the correct parameter
		desc = ""
		for p in self.blockinstance.blocktype.params:
			if p.getId() == self.name:
				desc = p.getText()
		return desc.split(":",2)[1].strip()
		
class LineInstance:
	def __init__(self,fromport=None,toport=None):
		self.fromport = fromport
		self.toport = toport

	def __str__(self):
		"""
		Create a string for use in MODEL export.
		"""
		if self.fromport and self.toport:	
			fromname = "%s.%s" % (self.fromport.blockinstance.name, self.fromport.name)
			toname = "%s.%s" % (self.toport.blockinstance.name, self.toport.name)
			return "\t%s, %s ARE_THE_SAME;\n" % (fromname, toname)
		return ""

# TODO set up reversible properties...?



