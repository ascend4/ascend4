##### CELSIUS TEMPERATURE WORKAROUND
from gi.repository import Pango

from preferences import Preferences


class CelsiusUnits:

	pref = Preferences()

	@staticmethod
	def get_units_row(selected, units):
		weight = Pango.Weight.NORMAL
		if selected:
			weight = Pango.Weight.BOLD
		return [selected, CelsiusUnits.get_display_symbol(units), CelsiusUnits.get_conversion_text(units), weight]

	@staticmethod
	def get_celsius_sign():
		return "degC"

	@staticmethod
	def get_fahrenheit_sign():
		return "degF"

	@staticmethod
	def get_celsius_symbol():
		return "\N{DEGREE SIGN}C"

	@staticmethod
	def get_fahrenheit_symbol():
		return "\N{DEGREE SIGN}F"

	@staticmethod
	def get_temperature_units():
		return [CelsiusUnits.get_celsius_sign(), CelsiusUnits.get_fahrenheit_sign()]

	@staticmethod
	def get_display_symbol(units):
		if units == CelsiusUnits.get_celsius_sign():
			return CelsiusUnits.get_celsius_symbol()
		if units == CelsiusUnits.get_fahrenheit_sign():
			return CelsiusUnits.get_fahrenheit_symbol()
		return units

	@staticmethod
	def get_conversion_text(units):
		if units == CelsiusUnits.get_celsius_sign():
			return "1 K (offset -273.15 K)"
		if units == CelsiusUnits.get_fahrenheit_sign():
			return "5/9 K (offset -459.67 degF)"
		return ""

	@staticmethod
	def normalize_units_name(units):
		if units is None:
			return None
		units = units.strip()
		if units == CelsiusUnits.get_celsius_symbol():
			return CelsiusUnits.get_celsius_sign()
		if units == CelsiusUnits.get_fahrenheit_symbol():
			return CelsiusUnits.get_fahrenheit_sign()
		return units

	@staticmethod
	def get_preferred_temperature_units(instance):
		if instance.getType().isRefinedReal() and str(instance.getType().getDimensions()) == 'TMP':
			return CelsiusUnits.normalize_units_name(Preferences().getPreferredUnitsOrigin(str(instance.getType().getName())))
		return None

	@staticmethod
	def get_display_unit_name(instance, default=None):
		units = CelsiusUnits.get_preferred_temperature_units(instance)
		if units in CelsiusUnits.get_temperature_units():
			return CelsiusUnits.get_display_symbol(units)
		return default

	@staticmethod
	def convert_show_value(instance, value):
		units = CelsiusUnits.get_preferred_temperature_units(instance)
		if units == CelsiusUnits.get_celsius_sign():
			try:
				return float(value) - 273.15
			except (TypeError, ValueError):
				return value
		if units == CelsiusUnits.get_fahrenheit_sign():
			try:
				return (float(value) - 273.15) * 9.0 / 5.0 + 32.0
			except (TypeError, ValueError):
				return value
		return value

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
	def convert_fahrenheit_to_kelvin(value, instype):
		if instype.startswith("delta"):
			try:
				temp = float(value)
			except ValueError:
				return value
			return str(temp * 5.0 / 9.0)
		try:
			temp = float(value)
		except ValueError:
			return value
		return str((temp - 32.0) * 5.0 / 9.0 + 273.15)

	@staticmethod
	def convert_kelvin_to_fahrenheit(value, instype):
		if instype.startswith("delta"):
			try:
				temp = float(value)
			except ValueError:
				return value
			return str(temp * 9.0 / 5.0)
		try:
			temp = float(value)
		except ValueError:
			return value
		return str((temp - 273.15) * 9.0 / 5.0 + 32.0)

	@staticmethod
	def split_temp_input(text):
		s = text.strip()
		for units in [
			CelsiusUnits.get_celsius_symbol(),
			CelsiusUnits.get_fahrenheit_symbol(),
			CelsiusUnits.get_celsius_sign(),
			CelsiusUnits.get_fahrenheit_sign(),
		]:
			if s.endswith(units):
				return s[:-len(units)].strip(), CelsiusUnits.normalize_units_name(units)
		return s, None

	@staticmethod
	def convert_edit(instance, text, save_units):
		if instance.getType().isRefinedReal() and str(instance.getType().getDimensions()) == 'TMP':
			preferred = CelsiusUnits.get_preferred_temperature_units(instance)
			value_text, explicit_units = CelsiusUnits.split_temp_input(text)
			units = explicit_units
			if units is None and len(text.split()) == 1 and preferred in CelsiusUnits.get_temperature_units():
				units = preferred
			if units == CelsiusUnits.get_celsius_sign():
				text = CelsiusUnits.convert_celsius_to_kelvin(value_text, str(instance.getType()))
				if save_units:
					CelsiusUnits.pref.setPreferredUnits(str(instance.getType().getName()), CelsiusUnits.get_celsius_sign())
			elif units == CelsiusUnits.get_fahrenheit_sign():
				text = CelsiusUnits.convert_fahrenheit_to_kelvin(value_text, str(instance.getType()))
				if save_units:
					CelsiusUnits.pref.setPreferredUnits(str(instance.getType().getName()), CelsiusUnits.get_fahrenheit_sign())
		return text

	@staticmethod
	def convert_show(instance, value, add_sign, default=None):
		units = CelsiusUnits.get_preferred_temperature_units(instance)
		if units == CelsiusUnits.get_celsius_sign():
			temp = value.split(" ")[0]
			value = CelsiusUnits.convert_kelvin_to_celsius(temp, str(instance.getType()))
			if add_sign:
				value += " " + CelsiusUnits.get_celsius_symbol()
			return value
		if units == CelsiusUnits.get_fahrenheit_sign():
			temp = value.split(" ")[0]
			value = CelsiusUnits.convert_kelvin_to_fahrenheit(temp, str(instance.getType()))
			if add_sign:
				value += " " + CelsiusUnits.get_fahrenheit_symbol()
			return value

		if default is not None:
			return default

		return value
##### CELSIUS TEMPERATURE WORKAROUND
