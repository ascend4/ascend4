from gaphas.connector import PointPort
from blockinstance import PortInstance
from gaphas.connector import VariablePoint
from gaphas.solver import WEAK

class BlockPort(PointPort):
	"""
	Port object with additional property to hold application-layer
	representation of the port, including the reference to the block to which
	it belongs (this data is not stored in the Gaphas Port object)
	"""
	
	def __init__(self, blockinstance, portname):
		super(BlockPort,self).__init__(VariablePoint((0,0),strength=WEAK))
		self.portinstance = blockinstance.ports[portname]

