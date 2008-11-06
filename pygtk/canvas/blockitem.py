from gaphas.item import Element
from gaphas.constraint import LineConstraint, LessThanConstraint, EqualsConstraint, Constraint, _update, BalanceConstraint
from gaphas.item import Line, SW, NE, NW, SE, Element, Handle
from gaphas.util import *

from port import *

class BlockItem(Element):
    """
    This is an ASCEND 'block' in the canvas-based modeller. The block will have
    sets of input and output ports to which connector lines can be 'glued'.
    The block will also have a corresponding ASCEND MODEL type, and a name
    which will be used in ASCEND to refer to this block. Each of the ports will
    be special visual elements, but note that these are not 'handles', because
    they can not be used to resize/modify the element.
    """

    def __init__(self, width=64, height=64):

        self.ports = []
        super(BlockItem, self).__init__(width, height)

    def draw(self, context):
		#print 'Box.draw', self
		c = context.cairo
	
		phalfsize = 3
		for p in self.ports:
			c.rectangle(p.x - phalfsize, p.y - phalfsize, 2*phalfsize, 2*phalfsize)
			if p.connected_to is None:
				c.set_source_rgba(0.8,0.8,1, 0.8)
			else:
				c.set_source_rgba(1,0,0,1)
			c.fill_preserve()
			c.set_source_rgb(0.8,0.8,0)
			c.stroke()

    def glue(self,item, handle, ix, iy):
        gluerange = 10
        mindist = -1;
        minport = None
        for p in self.ports:
            dist = math.sqrt((ix-p.x)**2 + (iy-p.y)**2)
            if dist < gluerange:
                if not minport or dist<mindist:
                    mindist = dist
                    minport = p
        return mindist, minport

    def pre_update(self,context):
        #print "PRE-UPDATE BLOCK"
		pass

class DefaultBlockItem(BlockItem):
	"""
	This is a 'default block' with a certain number of input and output ports
	shown depending on the values sent to __init__. It is drawn as a simple
	box with the input ports on the left and the output ports on the right.
	"""

	def __init__(self, blockinstance):

		self.blockinstance = blockinstance
		inputs = len(blockinstance.blocktype.inputs)
		outputs = len(blockinstance.blocktype.outputs)
		super(DefaultBlockItem, self).__init__(64, 64)

		eq = EqualsConstraint
		bal = BalanceConstraint
		handles = self._handles
		h_nw = handles[NW]
		h_ne = handles[NE]
		h_sw = handles[SW]
		h_se = handles[SE]

		for i in range(inputs):
			p = Port(self)
			self.ports.append(p)
			self._constraints.append(eq(p.x, h_nw.x))
			self._constraints.append(bal(band=(h_nw.y, h_sw.y),v=p.y, balance=(0.5 + i)/inputs))

		for i in range(outputs):
			p = Port(self)
			self.ports.append(p)
			self._constraints.append(eq(p.x, h_ne.x))
			self._constraints.append(bal(band=(h_ne.y,h_se.y),v=p.y, balance=(0.5 + i)/outputs))

	def draw(self, context):
		# draw the box itself
		c = context.cairo
		nw = self._handles[NW]
		c.rectangle(nw.x, nw.y, self.width, self.height)
		if context.hovered:
		    c.set_source_rgba(.8,.8,1, .8)
		else:
		    c.set_source_rgba(1,1,1, .8)
		c.fill_preserve()
		c.set_source_rgb(0,0,0.8)
		c.stroke()

		text_center(c,self.width/2,self.height/2,self.blockinstance.name)

		# now the draw the ports using the base class
		super(DefaultBlockItem, self).draw(context)
