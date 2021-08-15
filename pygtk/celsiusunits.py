##### CELSIUS TEMPERATURE WORKAROUND
from gi.repository import Pango

from preferences import Preferences


class CelsiusUnits:

	pref = Preferences()

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
		try:
			temp = float(value)
		except ValueError:
			return value

		return str(temp + 273.15)

	@staticmethod
	def convert_kelvin_to_celsius(value, instype):
		if instype.startswith("delta"):
			return value
		try:
			temp = float(value)
		except ValueError:
			return value

		return str(temp - 273.15)

	@staticmethod
	def convert_edit(instance, text, save_units):
		if instance.getType().isRefinedReal() and str(instance.getType().getDimensions()) == 'TMP':
			units = CelsiusUnits.pref.getPreferredUnitsOrigin(str(instance.getType().getName()))
			if units == CelsiusUnits.get_celsius_sign() and len(text.split(" ")) == 1 or text.find(CelsiusUnits.get_celsius_sign()) != -1:
				if text.find(CelsiusUnits.get_celsius_sign()) == -1:
					text += CelsiusUnits.get_celsius_sign()
				text = CelsiusUnits.convert_celsius_to_kelvin(text[:text.find(CelsiusUnits.get_celsius_sign())], str(instance.getType()))
				if save_units:
					CelsiusUnits.pref.setPreferredUnits(str(instance.getType().getName()), CelsiusUnits.get_celsius_sign())
		return text

	@staticmethod
	def convert_show(instance, value, add_sign, default=None):
		if instance.getType().isRefinedReal() and str(instance.getType().getDimensions()) == 'TMP':
			units = Preferences().getPreferredUnitsOrigin(str(instance.getType().getName()))
			if units == CelsiusUnits.get_celsius_sign():
				temp = value.split(" ")[0]
				value = CelsiusUnits.convert_kelvin_to_celsius(temp, str(instance.getType()))
				if add_sign:
					value += " " + CelsiusUnits.get_celsius_sign()
				return value

		if default is not None:
			return default

		return value
##### CELSIUS TEMPERATURE WORKAROUND