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
		if name is None:
			name = self.get_default_name()
		self.name = name
		self.tab = 0
		self.color_r = 1
		self.color_g = 1
		self.color_b = 1
		self.instance = 0
		self.ports = {}
		#self.stream = ''
		for n in self.blocktype.inputs:
			self.ports[n[0]] = PortInstance(self,n[0], n[1], PORT_IN)
		for n in self.blocktype.outputs:
			self.ports[n[0]] = PortInstance(self,n[0], n[1], PORT_OUT)
		self.params = {}
		for n in self.blocktype.params:
			self.params[n[0]] = (ParamInstance(self,n[0],n[1]))

		self.usercode = ""

	def get_default_name(self):
		n = str(self.blocktype.type.getName())
		if not blocknameindex.has_key(n):
			print "the key '%s' is not in blocknameindex" % n

		return "%s%s" % (n, blocknameindex[n])

	def __str__(self):
		return "\t%s IS_A %s;\n" % (self.name,self.blocktype.type.getName())

	def reattach_ascend(self,ascwrap,notesdb):
		if type(self.blocktype.type) == str:
			self.blocktype.reattach_ascend(ascwrap, notesdb)

		for port in self.ports:
			self.ports[port].type = ascwrap.findType(self.ports[port].type)

		for param in self.params:
			self.params[param].type = ascwrap.findType(self.params[param].type)

	def __getstate__(self):
		#Return state values to pickle without  blockinstance.instance
		state = self.__dict__.copy()
		return(state)

	def __setstate__(self, state):
		#Restore state values from pickle
		self.__dict__ = state


class PortInstance:
	"""
	Application-layer representation of a Port, which is a variable inside a
	MODEL which is connectable using the Canvas GUI. Class includes the name of
	the variable represented by the Port, but no type information, as that is
	currently difficult to extract from the ASCEND API.
	"""
	def __init__(self,blockinstance,name, type, io):
		self.blockinstance = blockinstance
		self.name = name
		self.type = type
		self.io = io

	def __getstate__(self):
		state = self.__dict__.copy()
		state['type'] = str(self.type)
		return(state)

	def __setstate__(self, state):
		self.__dict__ = state

class ParamInstance:
	"""
	Application-layer representation of a Parameter, which is a variable inside a
	MODEL the value for which should be settable using the Canvas GUI. Currently
	we have no information about the type of the parameter (its units etc)
	because that data is still difficult to extract from the ASCEND API.
	"""
	def __init__(self,blockinstance,name,type):
		self.blockinstance = blockinstance
		self.name = name
		self.type = type
		self.fix = False
		self.value = self.get_initial_value()
		self.fix = self.set_initial_state()
		if self.type.getPreferredUnits():
			self.units=str(self.type.getPreferredUnits().getName())
		else:
			self.units=str(self.type.getDimensions().getDefaultUnits().getName())
		if self.units == '1':
			self.units = ''

	def get_description(self):
		"""
		The parameter description (if it exists) is stored in the NOTE from the
		original MODEL file. This will go and get it (and saves us from storing
		it in the Pickle.
		"""
		# find the correct parameter
		desc = ""
		for p in self.blockinstance.blocktype.params:
			if p[0] == self.name:
				desc = p[2]
		return desc.split(":",2)[1].strip().split("=")[0].strip()

	def get_initial_value(self):
		'''
		This will get the initial parameter value
		given in the NOTE
		dp "param: pressure rise due to the pump = 10{atm}" IS_A delta_pressure;
		this methos will return 10{atm}
		'''

		desc = ""
		for p in self.blockinstance.blocktype.params:
			if p[0] == self.name:
				desc = p[2]

		temp = desc.split(":",2)[1].strip().split("=")


		if(len(temp)==1):
			return None	 # for no value

		temp2 = temp[1].split("[")

		# for free state
		# print len(temp2)
		if(len(temp2)>1):
			self.fix = False
		else:
			self.fix = True

		#print self.fix
		if(self.fix):
			return temp[1].strip().split("{")[0].strip() + temp[1].strip().split("{")[1].strip().split("}")[0].strip()# for fixed state
		else:
			return temp2[1].split("]")[0].strip()   # for free state

	def set_initial_state(self):
		'''
		This will return initial state of the parameter i.e. FIX or FREE
		'''
		self.get_initial_value()
		if (self.fix):
			return True  # for fix
		else:
			return False	# for free


	def getValue(self):
		if self.value:
			return (str(self.value))
		else:
			return(' '+str(self.units))

	def setValue(self,conv,units):
		if self.value:
			self.value=self.value*conv
			self.units=units
			return(str(self.value)+' '+str(self.units))
		else:
			self.units=units
			return(' '+str(self.units))

	def __getstate__(self):
		state = self.__dict__.copy()
		state['type'] = str(self.type)
		return(state)

	def __setstate__(self, state):
		self.__dict__ = state

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

# vim: set ts=4 noet:
