#
# Handler for processing dimensioned value inputs
#

import re
import ascpy

# RE for units matching
UNITS_RE = re.compile(r"([-+]?(\d+(\.\d*)?|\d*\.\d+)([eE][-+]?\d+)?)\s*(.*)")

class InputError(Exception):
	def __init__(self,msg):
		self.msg = msg;
	def __str__(self):
		return "Input Error: %s" % self.msg;

class RealAtomEntry:
	def __init__(self,instance,newtext,default_units=None):
		self.instance = instance;
		self.newtext = newtext;
		self.units = None; # the string value of the entered units
		self.value = None;
		self.default_units = default_units;

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

		print("val = ",_val)
		print("units = ",self.units)

		# parse the units, throw an error if no good
		try:
			_val = float(_val)
		except RuntimeError:
			raise InputError("Unable to convert number part '%s' to float" % _val)

		# check the units
		if self.units.strip() == "":
			# if no units entered, assume display/default units supplied by caller.
			_u = self.default_units
			if _u is None:
				try:
					_u = self.instance.getDisplayUnits(False)
				except RuntimeError:
					_u = _instdim.getDefaultUnits()
			elif _u.__class__ == str:
				_u = ascpy.Units(_u)
			print("Assuming units '%s'" % _u.getName().toString())
		else:
			try:
				_u = ascpy.Units(self.units)
				print("Parsed units '%s'" % self.units)
			except RuntimeError:
				raise InputError("Unrecognisable units '%s'" % self.units)

			if _instdim != _u.getDimensions():

				if _u.getDimensions().isDimensionless():
					self.units = "[dimensionless]"

				_my_dims = _instdim.getDefaultUnits()
				if _instdim.isDimensionless():
					_my_dims = "[dimensionless]"
					raise InputError("Incompatible units '%s' (must be dimensionless)" 
							% (self.units) )
				else:
					raise InputError("Incompatible units '%s' (must fit with '%s')" 
							% (self.units, _my_dims.getName().toString()) )
	
		_conv = float(_u.getConversion())
		# self.reporter.reportNote("Converting: multiplying '%s %s' by factor %s to get SI units" % (_val, _units, _conv) )
		self.value = _val * _conv;

		print("Setting '%s' to '%f'" % (self.instance.getName().toString(), self.value))
		
	def setValue(self):
		if self.instance.getType().isRefinedSolverVar():
			# for solver vars, set the 'fixed' flag as well
			self.instance.setFixedValue(self.value)
		else:
			self.instance.setRealValue(self.value)

	def getValue(self):
		return self.value

	def exportPreferredUnits(self,prefs):
		return

	def applyUnitsOverride(self, browser, by_name=None, model_scope=None):
		_dim = self.instance.getDimensions()
		if self.units is None or self.units.strip() == "":
			return
		if _dim.isDimensionless() or _dim.isWild():
			return
		if by_name is None:
			by_name = browser.get_units_edit_override_by_name()
		if model_scope is None:
			model_scope = browser.get_units_edit_scope_model()
		if by_name:
			model_scope = True
		self.instance.setDisplayUnitsOverride(self.units, by_name, model_scope)
		rc = ascpy.saveDisplayUnitsOverrides()
		if rc != 0:
			raise RuntimeError("Failed to save units-overrides preferences")
		
