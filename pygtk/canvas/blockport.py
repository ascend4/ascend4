from gaphas.connector import PointPort
from blockinstance import PortInstance
from gaphas.connector import Position
from gaphas.solver import WEAK

class BlockPort(PointPort):
	"""
	Port object with additional property to hold application-layer
	representation of the port, including the reference to the block to which
	it belongs (this data is not stored in the Gaphas Port object)
	"""
	
	def __init__(self, blockinstance, portname):
		super(BlockPort,self).__init__(Position((0,0)))
		self.portinstance = blockinstance.ports[portname]

	def get_portinstance(self):
		return self.portinstance