##### CELSIUS TEMPERATURE WORKAROUND
from gi.repository import Pango


class CelsiusUnits:

	@staticmethod
	def get_units_row(selected):
		weight = Pango.Weight.NORMAL
		if selected:
			weight = Pango.Weight.BOLD
		return [selected, CelsiusUnits.get_celsius_sign(), "1 K (offset -273.15 K)", weight]

	@staticmethod
	def get_celsius_sign():
		return "degC"

	@staticmethod
	def convert_celsius_to_kelvin(value, instype):
		if instype.startswith("delta"):
			return value

		temp = float(value)
		return str(temp + 273.15)

	@staticmethod
	def convert_kelvin_to_celsius(value, instype):
		if instype.startswith("delta"):
			return value

		temp = float(value)
		return str(temp - 273.15)
##### CELSIUS TEMPERATURE WORKAROUND