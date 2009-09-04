
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint
from gaphas.item import Line, SW, NE, NW, SE, Item, Handle
from gaphas.util import *
from gaphas.connector import PointPort, VariablePoint
from gaphas.solver import solvable, WEAK, NORMAL, STRONG, VERY_STRONG
from gaphas.state import observed, reversible_method, reversible_pair, reversible_property, disable_dispatching
from gaphas.geometry import distance_rectangle_point

from blockport import BlockPort
from blockinstance import PORT_IN, PORT_OUT

class ElementNoPorts(Item):
    """
    This is a copy of the Element class, but without the declaration
	of the LinePorts in the __init__ method. It will be proposed to the
	Gaphor team that the Element class be modified like this, because
	there is quite a lot of useful code aside from the LinePort definition.
    """

    def __init__(self, width=10, height=10):
        super(ElementNoPorts, self).__init__()
        self._handles = [ h(strength=VERY_STRONG) for h in [Handle]*4 ]

        handles = self._handles
        h_nw = handles[NW]
        h_ne = handles[NE]
        h_sw = handles[SW]
        h_se = handles[SE]

        # no element ports by default
        self._ports = []

        # setup constraints
        self.constraint(horizontal=(h_nw.pos, h_ne.pos))
        self.constraint(horizontal=(h_se.pos, h_sw.pos))
        self.constraint(vertical=(h_nw.pos, h_sw.pos))
        self.constraint(vertical=(h_se.pos, h_ne.pos))

        # create minimal size constraints
        self._c_min_w = self.constraint(left_of=(h_nw.pos, h_se.pos), delta=10)
        self._c_min_h = self.constraint(above=(h_nw.pos, h_se.pos), delta=10)

        # set width/height when minimal size constraints exist
        self.width = width
        self.height = height


    def setup_canvas(self):
        super(ElementNoPorts, self).setup_canvas()

        # Set width/height explicitly, so the element will maintain it
        self.width = self.width
        self.height = self.height

    def _set_width(self, width):
        """
        >>> b=ElementNoPorts()
        >>> b.width = 20
        >>> b.width
        20.0
        >>> b._handles[NW].x
        Variable(0, 40)
        >>> b._handles[SE].x
        Variable(20, 40)
        """
        if width < self.min_width:
            width = self.min_width
        h = self._handles
        h[SE].x = h[NW].x + width


    def _get_width(self):
        """
        Width of the box, calculated as the distance from the left and
        right handle.
        """
        h = self._handles
        return float(h[SE].x) - float(h[NW].x)

    width = property(_get_width, _set_width)

    def _set_height(self, height):
        """
        >>> b=ElementNoPorts()
        >>> b.height = 20
        >>> b.height
        20.0
        >>> b.height = 2
        >>> b.height
        10.0
        >>> b._handles[NW].y
        Variable(0, 40)
        >>> b._handles[SE].y
        Variable(10, 40)
        """
        if height < self.min_height:
            height = self.min_height
        h = self._handles
        h[SE].y = h[NW].y + height

    def _get_height(self):
        """
        Height.
        """
        h = self._handles
        return float(h[SE].y) - float(h[NW].y)

    height = property(_get_height, _set_height)

    @observed
    def _set_min_width(self, min_width):
        """
        Set minimal width.
        """
        if min_width < 0:
            raise ValueError, 'Minimal width cannot be less than 0'

        self._c_min_w.delta = min_width
        if self.canvas:
            self.canvas.solver.request_resolve_constraint(self._c_min_w)

    min_width = reversible_property(lambda s: s._c_min_w.delta, _set_min_width)

    @observed
    def _set_min_height(self, min_height):
        """
        Set minimal height.
        """
        if min_height < 0:
            raise ValueError, 'Minimal height cannot be less than 0'

        self._c_min_h.delta = min_height
        if self.canvas:
            self.canvas.solver.request_resolve_constraint(self._c_min_h)

    min_height = reversible_property(lambda s: s._c_min_h.delta, _set_min_height)

        
    def point(self, pos):
        """
        Distance from the point (x, y) to the item.
        """
        h = self._handles
        hnw, hse = h[NW], h[SE]
        return distance_rectangle_point(map(float, (hnw.x, hnw.y, hse.x, hse.y)), pos)


class BlockItem(ElementNoPorts):
    """
    This is an ASCEND 'block' in the canvas-based modeller. The block will have
    sets of input and output ports to which connector lines can be 'glued'.
    The block will also have a corresponding ASCEND MODEL type, and a name
    which will be used in ASCEND to refer to this block. Each of the ports will
    be special visual elements, but note that these are not 'handles', because
    they can not be used to resize/modify the element.
    """

    def __init__(self, width=64, height=64):
        super(BlockItem, self).__init__(width, height)

    def draw(self, context):
		"""
		We want all ports within ASCEND to have a common appearance, so we
		implement the drawing of ports here, and allow sub-classes of BlockItem
		to implement the drawing of the other parts of the block, including the
		outline etc.

		Connected ports will be coloured red, other ports will be pale blue.
		"""
		c = context.cairo	
		phalfsize = 3
		for p in self._ports:
			if hasattr(p,"point"):
				c.rectangle(p.point.x - phalfsize, p.point.y - phalfsize, 2*phalfsize, 2*phalfsize)
				#if p.connected_to is None:
				c.set_source_rgba(0.8,0.8,1, 0.8)
				#else:
				#	c.set_source_rgba(1,0,0,1)
				c.fill_preserve()
				c.set_source_rgb(0.8,0.8,0)
				c.stroke()

	# removing the 'glue' method now, as ports are now built in to gaphas.

    def pre_update(self,context):
        #print "PRE-UPDATE BLOCK"
		pass

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
		super(DefaultBlockItem, self).__init__(64, 64)

		eq = EqualsConstraint
		bal = BalanceConstraint
		handles = self._handles
		h_nw = handles[NW]
		h_ne = handles[NE]
		h_sw = handles[SW]
		h_se = handles[SE]

		ninputs = len(blockinstance.blocktype.inputs)
		noutputs = len(blockinstance.blocktype.outputs)
		ii, oi = (0,0) # input and output index counters
		_ports = []
		for i in self.blockinstance.ports:
			p = BlockPort(blockinstance, i)
			if self.blockinstance.ports[i].type is PORT_IN:
				self._constraints.append(eq(p.point.x, h_nw.x))
				self._constraints.append(bal(band=(h_nw.y, h_sw.y),v=p.point.y, balance=(0.5 + ii)/ninputs))
				ii += 1
			elif self.blockinstance.ports[i].type is PORT_OUT:
				self._constraints.append(eq(p.point.x, h_ne.x))
				self._constraints.append(bal(band=(h_ne.y,h_se.y),v=p.point.y, balance=(0.5 + oi)/noutputs))
				oi += 1
			else:
				raise RuntimeError("Unknown port type")
			_ports.append(p)

		self._ports = _ports

	def draw(self, context):
		# draw the box itself
		c = context.cairo
		nw = self._handles[NW]
		c.rectangle(nw.x, nw.y, self.width, self.height)
		if context.hovered:
			try:
		    		c.set_source_rgb(self.blockinstance.color_r,self.blockinstance.color_g,self.blockinstance.color_b)
			except AttributeError:
				c.set_source_rgb(1,1,1)
				
		    #c.set_source_rgba(.8,.8,1, .8)
		else:
		    #c.set_source_rgba(1,1,1, .8)
		    	try:
		    		c.set_source_rgb(2*self.blockinstance.color_r,2*self.blockinstance.color_g,2*self.blockinstance.color_b)
			except AttributeError:
				c.set_source_rgb(1,1,1)
				
		c.fill_preserve()
		c.set_source_rgb(0,0,0)
		c.stroke()

		text_center(c,self.width/2,self.height/2,self.blockinstance.name)

		# now the draw the ports using the base class
		super(DefaultBlockItem, self).draw(context)

