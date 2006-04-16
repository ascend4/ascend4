#
# Handler for processing dimensioned value inputs
#

import re
import ascpy

# RE for units matching
UNITS_RE = re.compile("([-+]?(\d+(\.\d*)?|\d*\.d+)([eE][-+]?\d+)?)\s*(.*)");

class InputError(Exception):
	def __init__(self,msg):
		self.msg = msg;
	def __str__(self):
		return "Input Error: %s" % self.msg;

class RealAtomEntry:
	def __init__(self,instance,newtext):
		self.instance = instance;
		self.newtext = newtext;
		self.units = None; # the string value of the entered units
		self.value = None;

	def checkEntry(self):
		_instdim = self.instance.getDimensions();
		_insttype = self.instance.getType()

		try:
			# match a float with option text afterwards, optionally separated by whitespace
			_match = re.match(UNITS_RE,self.newtext)
			if not _match:
				raise InputError("Not a valid value-and-optional-units")

			_val = _match.group(1) # the numerical part of the input
			self.units = _match.group(5) # the text entered for units
			#_val, _units = re.split("[ \t]+",newtext,2);
		except RuntimeError:
			raise InputError("Unable to split value and units")

		print "val = ",_val
		print "units = ",self.units

		# parse the units, throw an error if no good
		try:
			_val = float(_val)
		except RuntimeError:
			raise InputError("Unable to convert number part '%s' to float" % _val)

		# check the units
		if self.units.strip() == "":
			# if no units entered, assume the 'preferred' units
			_u = _insttype.getPreferredUnits()
			if _u == None:
				# no preferred units for this type, so assume default units
				_u = _instdim.getDefaultUnits()
			print "Assuming units '%s'" % _u.getName().toString()
		else:
			try:
				_u = ascpy.Units(self.units)
				print "Parsed units '%s'" % self.units
			except RuntimeError:
				raise InputError("Unrecognisable units '%s'" % self.units)

			if _instdim != _u.getDimensions():

				if _u.getDimensions().isDimensionless():
					self.units = "[dimensionless]"

				_my_dims = _instdim.getDefaultUnits()
				if _instdim.isDimensionless():
					_my_dims = "[dimensionless]"

				raise InputError("Incompatible units '%s' (must fit with '%s')" 
						% (self.units, _my_dims.getName().toString()) )
	
		_conv = float(_u.getConversion())
		# self.reporter.reportNote("Converting: multiplying '%s %s' by factor %s to get SI units" % (_val, _units, _conv) )
		self.value = _val * _conv;

		print "Setting '%s' to '%f'" % (self.instance.getName().toString(), self.value)
		
	def setValue(self):
		if self.instance.getType().isRefinedSolverVar():
			# for solver vars, set the 'fixed' flag as well
			self.instance.setFixedValue(self.value)
		else:
			self.instance.setRealValue(self.value)

	def getValue(self):
		return self.value

	def exportPreferredUnits(self,prefs):
		_typename = str( self.instance.getType().getName() )
		_dim = self.instance.getDimensions()

		if self.units.strip() != "" and not _dim.isDimensionless():
			prefs.setPreferredUnits(_typename,self.units);
		

