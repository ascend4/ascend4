from gaphas import Canvas
from gaphas.item import Line
import re

UNITS_RE = re.compile("([-+]?(\d+(\.\d*)?|\d*\.d+)([eE][-+]?\d+)?)\s*(.*)");

saved_model = None

class BlockCanvas(Canvas):
	
	def __init__(self):
		super(BlockCanvas, self).__init__()
		self.filestate = 0
		self.canvasmodelstate = 'Unsolved'
		self.model_library = 'test/canvas/blocktypes.a4c'
		self.saved_data = None
		self.filename = None
	
	def update_constraints(self, items):
		"""
		Update constraints. Also variables may be marked as dirty before the
		constraint solver kicks in.
		"""
		# request solving of external constraints associated with dirty items
		request_resolve = self._solver.request_resolve
		for item in items:
			if hasattr(item,'ports'):
				for p in item._ports:
					if hasattr(p,"point"):
						request_resolve(p.point.x)
						request_resolve(p.point.y)

		super(BlockCanvas,self).update_constraints(items)

	def reattach_ascend(self, library, notesdb):
		"""
		After unpickling a canvas, this method gives a way of reattaching
		the model to an ASCEND library, by connecting the types named in the
		pickle with the typenames present in the Library.
		"""
		# FIXME need to set ASCEND to *load* the missing types if they're not
		# already in the Library.
		items = []
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			bi = item.blockinstance
			if bi.blocktype.type is None:
				bi.blocktype.reattach_ascend(library, notesdb)

	def __str__(self):
		"""
		Create ASCEND code corresponding to canvas model, return the
		code as a string.
		"""

		# FIXME it should be possible to perform this function at an
		# application layer, without using routines from Gaphas...
		s = "\n(* automatically generated model from blocklist.py *)\n";
		s += 'REQUIRE "blocktypes.a4c";\n'
		s += "MODEL canvasmodel;\n"
	
		# Add blocks to the model via IS_A statements
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			s += str(item.blockinstance)

		# Add LINES as ARE_THE_SAME statements
		for line in self.get_all_items():
			if not isinstance(line, Line):
				continue
			s += str(line.lineinstance)

		# Set PARAMETERS using the ON_LOAD METHOD.
		p = ""
		uc = ""
		s += "METHODS\nMETHOD on_load;\n";
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			n = item.blockinstance.name
			uc += item.blockinstance.usercode	
			for k,v in item.blockinstance.params.iteritems():
				if v.fix == 1:
					p += "\tFIX %s.%s;\n" % (n,v.name)
					_param_value = self.checkEntry(v.value)
					p += "\t%s.%s := %s;\n" % (n,v.name,_param_value)
						
		if not p == "":
			s += p
		#if ucflag == 1:
		#	s += "RUN user_code;\n"	
		#if ucflag == 1:
		#	s += usercode 
				
		if uc == "":
			s += "END on_load;\n"
		else:
			s += "RUN user_code;\n"
			s += "END on_load;\n"
			s += "METHOD user_code;\n"
			s += uc
			s += "END user_code;"		
		
		
		s += "\nEND canvasmodel;\n";
		return s

	def __getstate__(self):
		"""
		Placeholder for any special pickling stuff that we want to do with
		our canvas.
		"""

		return super(BlockCanvas,self).__getstate__()
	
	def __setstate__(self, state):
		"""
		Placeholder for any special pickling stuff that we want to do with
		our canvas.
		"""
		super(BlockCanvas,self).__setstate__(state)

	def checkEntry(self, _param_value):

		try:
			# match a float with option text afterwards, optionally separated by whitespace
			_match = re.match(UNITS_RE,_param_value)
			print "done"
			if not _match:
				print _param_val
				#raise InputError("Not a valid value-and-optional-units")
				#parent.reporter.reportError("Not a valid value-and-optional-units")
			_val = _match.group(1) # the numerical part of the input
			_units = _match.group(5) # the text entered for units
			return str(_val)+'{'+str(_units)+'}'
				
		except:
			print  "Unable to generate Model Code. Please re-check parameter values."
