from gaphas.connector import PointPort
from blockinstance import PortInstance
from gaphas.connector import Position
from gaphas.solver import Variable
from gaphas.solver import WEAK

class BlockPort(PointPort):
	"""
	Port object with additional property to hold application-layer
	representation of the port, including the reference to the block to which
	it belongs (this data is not stored in the Gaphas Port object)
	"""

	def __init__(self, blockinstance, portname,location, portlabel):
		super(BlockPort,self).__init__(Position((float(location[0]),float(location[1]))))
		self.portinstance = blockinstance.ports[portname]
		self.portname = portname
		self.portlabel= portlabel
		self.location = location

	def get_portinstance(self):
		return self.portinstance

	def get_portname(self):
		return self.portname

	def get_portlabel(self):
		return self.portlabel

	def get_location(self):
		return self.location
