from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint,LineAlignConstraint, EquationConstraint
from gaphas.item import Line, SW, NE, NW, SE, Item, Handle, Element
from gaphas.util import *
from gaphas.connector import Position
from gaphas.solver import solvable, WEAK, NORMAL, STRONG, VERY_STRONG, Variable, REQUIRED
from gaphas.state import observed, reversible_method, reversible_pair, reversible_property
from gaphas.geometry import distance_rectangle_point
from gaphas.examples import Circle
from gaphas.canvas import Canvas
from gaphas.matrix import Matrix
from numpy import *
import math
import cairo

from blockport import BlockPort
from blockinstance import PORT_IN, PORT_OUT, PORT_INOUT

class ElementNoPorts(Element):
	"""
	This is a copy of the Element class, but without the declaration
	of the LinePorts in the __init__ method. It will be proposed to the
	Gaphor team that the Element class be modified like this, because
	there is quite a lot of useful code aside from the LinePort definition.
	"""

	min_width = solvable(strength=REQUIRED, varname='_min_width')
	min_height = solvable(strength=REQUIRED, varname='_min_height')

	def __init__(self, width=10, height=10):
		super(Element, self).__init__()
		self._handles = [ h(strength=VERY_STRONG) for h in [Handle]*4 ]

		handles = self._handles
		h_nw = handles[NW]
		h_ne = handles[NE]
		h_sw = handles[SW]
		h_se = handles[SE]

		# Share variables
		h_sw.pos.set_x(h_nw.pos.x)
		h_se.pos.set_x(h_ne.pos.x)
		h_ne.pos.set_y(h_nw.pos.y)
		h_se.pos.set_y(h_sw.pos.y)

		# No ports by default
		self._ports = []

		# initialize min_x variables
		self.min_width, self.min_height = 10, 10
		factor = 7
		delta_w = len(self.blockinstance.name)*factor
		delta_h = 10
		self.min_width = delta_w
		self.min_height = delta_h

		# create minimal size constraints
		self.constraint(left_of=(h_nw.pos, h_se.pos), delta=self._min_width)
		self.constraint(above=(h_nw.pos, h_se.pos), delta=self._min_height)

		self.width = width
		self.height = height

		# TODO: constraints that calculate width and height based on handle pos
		#self.constraints.append(EqualsConstraint(p1[1], p2[1], delta))


class BlockItem(ElementNoPorts):
	"""
	This is an ASCEND 'block' in the canvas-based modeller. The block will have
	sets of input and output ports to which connector lines can be 'glued'.
	The block will also have a corresponding ASCEND MODEL type, and a name
	which will be used in ASCEND to refer to this block. Each of the ports will
	be special visual elements, but note that these are not 'handles', because
	they can not be used to resize/modify the element.
	"""

	def __init__(self,blockinstance, width=64, height=64):
		self.blockinstance = blockinstance
		super(BlockItem, self).__init__(width, height)
		self.port_in = blockinstance.blocktype.port_in
		self.port_out = blockinstance.blocktype.port_out
		self.h = self._handles
		self.wide = self.h[SE].pos.x - self.h[NW].pos.x
		self.height = self.h[SE].pos.y - self.h[NW].pos.y
		self.normx = self.wide*0.1
		self.normy = self.height*0.1

	def up1(self):
		''' this is a update method used to update variables before drawing them again'''

		self.wide = self.h[SE].pos.x - self.h[NW].pos.x
		self.height = self.h[SE].pos.y - self.h[NW].pos.y
		self.normx = self.wide*0.1
		self.normy = self.height*0.1
		if not (len(self.port_in)==0 and len(self.port_out)==0):
			for w in self._ports:
				if(w.get_portinstance().io == PORT_IN):
					w.point._set_pos(Position(((float(self.port_in[w.portinstance.name][0]) * self.normx),(float(self.port_in[w.portinstance.name][1]) * self.normy))))
				else:
					w.point._set_pos(Position(((float(self.port_out[w.portinstance.name][0]) * self.normx),(float(self.port_out[w.portinstance.name][1]) * self.normy))))

	#Here combination of translate(x,y),rotate(angle),translate(-x,-y) is used to perform
	#rotation and flip about centre(x,y)

	def rotate_clock(self):
		self.matrix.translate(self.h[SE].pos.x/2,self.h[SE].pos.y/2)
		self.matrix.rotate(math.pi/2)
		self.matrix.translate(-1*self.h[SE].pos.x/2,-1*self.h[SE].pos.y/2)
		self.request_update()

	def rotate_anti(self):
		self.matrix.translate(self.h[SE].pos.x/2,self.h[SE].pos.y/2)
		self.matrix.rotate(-1*math.pi/2)
		self.matrix.translate(-1*self.h[SE].pos.x/2,-1*self.h[SE].pos.y/2)
		self.request_update()

	def flip(self):
		self.matrix.translate(self.h[SE].pos.x/2,self.h[SE].pos.y/2)
		self.matrix.rotate(math.pi)
		self.matrix.translate(-1*self.h[SE].pos.x/2,-1*self.h[SE].pos.y/2)
		self.request_update()

	def draw(self, context):
		"""
		  We want all ports within ASCEND to have a common appearance, so we
		  implement the drawing of ports here, and allow sub-classes of BlockItem
		  to implement the drawing of the other parts of the block, including the
		  outline etc.

		  Connected ports will be coloured red, other ports will be pale blue.
		  """
		from blockconnecttool import SET_CONNECTION_FLAG

		self.up1()
		c = context.cairo
		phalfsize = 3
		if SET_CONNECTION_FLAG[0]:
			for p in self._ports:
				if hasattr(p,"point") and checkportscanconnect(p.portinstance, SET_CONNECTION_FLAG[1]):
					c.rectangle(p.point.x - phalfsize, p.point.y - phalfsize, 2*phalfsize, 2*phalfsize)
					c.set_source_rgba(0.9,0.9,0.9, 0.8)
					c.fill_preserve()
					c.set_source_rgb(0,0,1) # blue when connect able
					c.stroke()

				elif hasattr(p,"point") and not checkportscanconnect(p.portinstance, SET_CONNECTION_FLAG[1]):
					c.rectangle(p.point.x - phalfsize, p.point.y - phalfsize, 2*phalfsize, 2*phalfsize)
					c.set_source_rgba(0,0,0, 0.8)
					c.fill_preserve()
					c.set_source_rgb(0,0,0) # black when not connect able
					c.stroke()
		else:
			for p in self._ports:
				if hasattr(p,"point"):
					c.rectangle(p.point.x - phalfsize, p.point.y - phalfsize, 2*phalfsize, 2*phalfsize)
					c.set_source_rgba(0,0,0, 0.8)
					c.fill_preserve()
					c.set_source_rgb(0,0,0)
					c.stroke()

	#port-labels will be displayed when mouse is hovered over the item's context
	#port-labels are numbers initially, but will be name of variables concerned with it later
	#when it will be linked with the solvers and parameter window.

		if context.focused:
			for w in self._ports:
				if(w.portinstance.io is PORT_IN):
					if(w.point.y/self.normy==0):
						text_align(c,w.point.x,w.point.y-10,str(w.get_portname()))
					elif (w.point.y/self.normy==10):
						text_align(c,w.point.x,w.point.y+10,str(w.get_portname()))
					else:
						text_align(c,w.point.x-3.5*len(str(w.get_portname())),w.point.y,str(w.get_portname()))
				else:
					if(w.point.y/self.normy==0):
						text_align(c,w.point.x,w.point.y-10,str(w.get_portname()))
					elif (w.point.y/self.normy==10):
						text_align(c,w.point.x,w.point.y+10,str(w.get_portname()))
					else:
						text_align(c,w.point.x+3.5*len(str(w.get_portname())),w.point.y,str(w.get_portname()))

		c.set_source_rgb(0,0,0)
		text_center(c,self.h[SE].pos.x/2,self.h[SE].y/0.9,self.blockinstance.name)

	def pre_update(self,context):
	#print "PRE-UPDATE BLOCK"
		pass

class GraphicalBlockItem(BlockItem):
	"""This class draws the custom blocks using graphic string from the notes section of model
	file. The notes section will be like this:
	MODEL tee REFINES equipment;
	NOTES
		'block' SELF {Tee piece}
		'icon' SELF {tee.svg}
		'graphic' SELF {0,0-0,10
				0,0-10,5
				10,5-0,10}
		'port_in' SELF {0,5}
		'port_out' SELF {10,4 10,6}
	END NOTES;
	side "out:" IS_A stream;
	inlet.mdot = outlet.mdot + side.mdot;
	END tee;
	So it will draw the custom icon for a model using the co-ordinates present in the graphic
	string.
	 """

	def __init__(self, blockinstance):

		self.blockinstance = blockinstance
		super(GraphicalBlockItem, self).__init__(blockinstance)

		eq = EqualsConstraint
		bal = BalanceConstraint
		handles = self._handles
		self.rad =1
		self.graphical_properties = blockinstance.blocktype.gr
		self.port_in = blockinstance.blocktype.port_in
		self.port_out = blockinstance.blocktype.port_out
		self.h_nw = handles[NW]
		self.h_ne = handles[NE]
		self.h_sw = handles[SW]
		self.h_se = handles[SE]
		self.wide = self.h_se.pos.x - self.h_nw.pos.x
		self.height = self.h_se.pos.y - self.h_nw.pos.y
		self.normx = self.wide*0.1 #normx is normalization factor for x-co-ordinate
		self.normy = self.height*0.1 #normy is normalization factor for y-co-ordinate
		ninputs = len(blockinstance.blocktype.inputs)
		noutputs = len(blockinstance.blocktype.outputs)
		ii, oi = (0,0) # input and output index counters
		_ports = []
		try:
			if not (len(self.port_in)==0 and len(self.port_out)==0):
				for i in self.blockinstance.ports:
					if self.blockinstance.ports[i].io is PORT_IN:
						p = BlockPort(blockinstance, i,self.port_in[i],ii)
						ii += 1
					elif self.blockinstance.ports[i].io is PORT_OUT:
						p = BlockPort(blockinstance, i,self.port_out[i],oi)
						oi += 1
					else:
						raise RuntimeError("Unknown port type")
					_ports.append(p)
			else:
				for i in self.blockinstance.ports:
					if self.blockinstance.ports[i].io is PORT_IN:
						p = BlockPort(blockinstance, i,[0,0],ii)
						self._constraints.append(eq(p.point.x, self.h_nw.x))
						self._constraints.append(bal(band=(self.h_nw.y, self.h_sw.y),v=p.point.y, balance=(0.5 + ii)/ninputs))
						ii += 1
					elif self.blockinstance.ports[i].io is PORT_OUT:
						p = BlockPort(blockinstance, i,[0,0],oi)
						self._constraints.append(eq(p.point.x, self.h_ne.x))
						self._constraints.append(bal(band=(self.h_ne.y,self.h_se.y),v=p.point.y, balance=(0.5 + oi)/noutputs))
						oi += 1
					else:
						raise RuntimeError("Unknown port type")
					_ports.append(p)
		except KeyError:
			print "Error Reporter to be called as it is a user error"
			print "Syntax error while defining ports or some ports-location are left to be added"
		self._ports = _ports

	def up(self):
		self.wide = self.h[SE].pos.x - self.h[NW].pos.x
		self.height = self.h[SE].pos.y - self.h[NW].pos.y
		#self.height = 1 * self.wide
		self.normx = self.wide*0.1
		self.normy = self.height*0.1

	def draw(self, context):
	# drawing the custom icons using the co-ordinates from model file(Graphical Representation)
		c = context.cairo
		self.up()
		for m in self.graphical_properties:
			c.move_to(float(m[0][0])*self.normx,float(m[0][1])*self.normy) #normalization
			for mm in m:
				if(len(mm)==2):
					c.line_to(float(mm[0])*self.normx,float(mm[1])*self.normy)
				else:
					c.move_to(float(mm[0])*self.normx + float(mm[2])*self.normx,float(mm[1])*self.normy)
					self.min_height = self.wide ## minimum ratio for custom icons having arcs......
					c.arc_negative(float(mm[0])*self.normx,float(mm[1])*self.normy,float(mm[2])*self.normx,math.pi*float(mm[3]),float(mm[4]))
		c.stroke()
		c.fill_preserve()
		super(GraphicalBlockItem,self).draw(context)

class DefaultBlockItem(BlockItem):
	"""
	 This is a 'default block' with a certain number of input and output ports
	 shown depending on the values sent to __init__. It is drawn as a simple
	 box with the input ports on the left and the output ports on the right.

	 @TODO Not clear yet whether blocks with 'custom' representations should have
	 as their parent class: this class or BlockItem.
	 """

	def __init__(self, blockinstance):

		self.blockinstance = blockinstance
		super(DefaultBlockItem, self).__init__(blockinstance)

		eq = EqualsConstraint
		bal = BalanceConstraint
		handles = self._handles
		self.graphical_properties = blockinstance.blocktype.gr
		self.port_in = blockinstance.blocktype.port_in
		self.port_out = blockinstance.blocktype.port_out
		self.h_nw = handles[NW]
		self.h_ne = handles[NE]
		self.h_sw = handles[SW]
		self.h_se = handles[SE]
		self.wide = self.h_se.pos.x - self.h_nw.pos.x
		self.height = self.h_se.pos.y - self.h_nw.pos.y
		self.normx = self.wide*0.1
		self.normy = self.height*0.1
		ninputs = len(blockinstance.blocktype.inputs)
		noutputs = len(blockinstance.blocktype.outputs)
		ii, oi = (0,0) # input and output index counters
		_ports = []

		try:
		# check to ensure if there is no port_in and port_out string
		# it will draw ports at default location in that case
			if not (len(self.port_in)==0 and len(self.port_out)==0):
				for i in self.blockinstance.ports:
					if self.blockinstance.ports[i].io is PORT_IN:
						p = BlockPort(blockinstance, i,self.port_in[i],ii)
						#print self.port_in[i],i
						ii += 1
					elif self.blockinstance.ports[i].io is PORT_OUT:
						p = BlockPort(blockinstance, i,self.port_out[i],oi)
						oi += 1
					else:
						raise RuntimeError("Unknown port type")
					_ports.append(p)
			else:
				for i in self.blockinstance.ports:
					if self.blockinstance.ports[i].io is PORT_IN:
						p = BlockPort(blockinstance, i,[0,0],ii)
						self._constraints.append(eq(p.point.x, self.h_nw.x))
						self._constraints.append(bal(band=(self.h_nw.y, self.h_sw.y),v=p.point.y, balance=(0.5 + ii)/ninputs))
						ii += 1
					elif self.blockinstance.ports[i].io is PORT_OUT:
						p = BlockPort(blockinstance, i,[0,0],oi)
						self._constraints.append(eq(p.point.x, self.h_ne.x))
						self._constraints.append(bal(band=(self.h_ne.y,self.h_se.y),v=p.point.y, balance=(0.5 + oi)/noutputs))
						oi += 1
					else:
						raise RuntimeError("Unknown port type")
					_ports.append(p)
		except KeyError:
			print "Error Reporter to be called as it is a user error"
			print "Syntax error while defining ports or some ports-location are left to be added"
		self._ports = _ports   

	def draw(self, context):
	# draw the default box itself

		c = context.cairo
		nw = self._handles[NW]
		c.rectangle(nw.x, nw.y, self.width, self.height)
		c.set_source_rgb(0,0,0)
		c.stroke()

		c.fill_preserve()
		super(DefaultBlockItem,self).draw(context)

class CustomBlockItem_pump(BlockItem):
	''' This is a hard-coded class for custom block item which will
	resemble the pump icon displayed in the icon palette '''
	''' It was the first ever custom block on canvas and will not be
	used later- it was just meant to test cairo context and gaphas'''

	def __init__(self, blockinstance):
		self.blockinstance = blockinstance
		super(CustomBlockItem_pump, self).__init__(self.blockinstance,64, 64)
		self.f = 1.05 # h = f * w
		self.h = self._handles
		self.wid = self.h[SE].pos.x - self.h[NW].pos.x
		self.r = self.wid/2
		self.xc = self.h[NW].pos.x + self.r
		self.yc = self.h[NW].pos.y + self.r
		self.th = math.atan2(self.f*self.wid - self.r, self.r)
		self.min_height = self.f*self.wid
		self._ports = [] # no ports yet... need to add them though

	def update(self):
		self.wid = self.h[SE].pos.x - self.h[NW].pos.x
		self.r = self.wid/2
		self.xc = self.h[NW].pos.x + self.r
		self.yc = self.h[NW].pos.y + self.r
		self.th = math.atan2(self.f*self.wid - self.r, self.r)
		self.min_height = self.f*self.wid

	def draw(self, context):
		self.update()
		cc = context.cairo
		cc.set_source_rgb(0,0,0)
		cc.new_sub_path()
		cc.arc_negative(self.xc,self.yc,self.r,2*math.pi,0)
		cc.move_to(self.xc + self.r*math.sin(self.th),self.yc + self.r*math.cos(self.th))
		cc.line_to(self.h[SE].pos.x, self.h[SE].pos.y)
		cc.line_to(self.h[SW].pos.x, self.h[SW].pos.y)
		cc.line_to(self.xc - self.r*math.sin(self.th),self.yc + self.r*math.cos(self.th))
		cc.stroke()
		text_center(cc,self.wid/2,self.f*self.wid/2,self.blockinstance.name)

class CustomBlockItem_turbine(BlockItem):
	''' This is a also a hard-coded class for custom block item which will
	resemble the turbine displayed in the icon palette '''
	''' It will be replaced by graphical representation later'''

	def __init__(self,blockinstance):
		self.blockinstance = blockinstance
		super(CustomBlockItem_turbine,self).__init__(self.blockinstance,width=64,height=64)
		self._ports=[]
		self.graphical_properties = blockinstance.blocktype.gr
		self.theta = math.pi/4
		self.min_inlet = 20
		self.h = self._handles
		self.wide = self.h[SE].pos.x - self.h[NW].pos.x
		self.height = self.h[SE].pos.y - self.h[NW].pos.y
		self.min_height = self.wide*2 + self.min_inlet
		self.normx = self.wide*0.1
		self.normy = self.height*0.1

	def update(self):
		self.wide = self.h[SE].pos.x - self.h[NW].pos.x
		self.height = self.h[SE].pos.y - self.h[NW].pos.y
		self.min_height = self.wide*2 + self.min_inlet
		self.normx = self.wide*0.1
		self.normy = self.height*0.1

	def draw(self, context):
		self.update()
		cc = context.cairo
		cc.move_to(self.h[SE].pos.x,self.h[SE].pos.y)
		cc.line_to(self.h[NE].pos.x,self.h[NE].pos.y)
		cc.line_to(self.h[NW].pos.x,self.h[NW].pos.y+self.wide/math.tan(self.theta))
		cc.line_to(self.h[SW].pos.x,self.h[SW].pos.y-self.wide/math.tan(self.theta))
		cc.line_to(self.h[SE].pos.x,self.h[SE].pos.y)
		cc.stroke()

		#super(CustomBlockItem_turbine,self).draw(context)

def checkportscanconnect(port1,port2):
	'''Checks if the two portinstances can connect
	 Returns: True, if Connectable
			  False, if not Connectable'''

	#TODO: Right now port Type checking is a simple str(Type) checking, have a complex procedure to check for
	#connectable ports!!

	if str(port1.type) == str(port2.type):
		if port1.io == PORT_IN or port1.io == PORT_INOUT:
			if port2.io == PORT_OUT or port2.io == PORT_INOUT:
				return True
		elif port2.io == PORT_IN or port2.io == PORT_INOUT:
			if port1.io == PORT_OUT or port1.io == PORT_INOUT:
				return True
	return False
