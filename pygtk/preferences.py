# Preferences module for ASCPY.

import yaml
from pathlib import Path
import re
import platform

import json

class Preferences:
	_instance = None

	def __new__(cls, *args, **kwargs):
		if not cls._instance:
			cls._instance = super(Preferences, cls).__new__(cls)
			cls._instance.load_preferences()
		return cls._instance

	def load_preferences(self):
		print("LOADING PREFERENCES\n\n")
		try:
			if platform.system()=="Windows":
				self.fn = Path(os.environ['APPDATA']) / "ascend-config.yml"
			else:
				folder = Path.home()/".config"/"ascend";
				folder.mkdir(parents=True,exist_ok=True);
				self.fn = folder/"ascend-config.yml"
			with open(self.fn, 'r') as f:
				self.preferences = yaml.safe_load(f)
		except FileNotFoundError:
			self.preferences = {}  # Default preferences
		if self.preferences is None:
			self.preferences = {}
		if 'geometry' not in self.preferences:
			self.preferences['geometry'] = {}
		if 'preferredunits' not in self.preferences:
			self.preferences['preferredunits'] = {}
		self.mtime = self.fn.stat().st_mtime if self.fn.exists() else 0

	def get_preference(self, key, default=None):
		return self.preferences.get(key, default)

	def set_preference(self, key, value):
		self.preferences[key] = value

	def save_preferences(self):
		print("SAVING PREFERENCES\n\n")
		# we won't save preferences if another ascend has saved them since we loaded (not sure what's best here)
		latest_mtime = self.fn.stat().st_mtime if self.fn.exists() else 0
		if latest_mtime == self.mtime:
			with open(self.fn, 'w') as f:
				yaml.dump(self.preferences, f, default_flow_style=False)

	def getStringPref(self, sect, key, default=None):
		return self.preferences.get(sect, {}).get(key, default)

	def getRealPref(self, sect, key, default=None):
		return self.preferences.get(sect, {}).get(key, default)

	def getBoolPref(self, sect, key, default=None):
		return self.preferences.get(sect, {}).get(key, default)

	def setStringPref(self, sect, key, val):
		if sect not in self.preferences:
			self.preferences[sect] = {}
		self.preferences[sect][key] = str(val)

	def setRealPref(self, sect, key, val):
		if sect not in self.preferences:
			self.preferences[sect] = {}
		self.preferences[sect][key] = float(val)

	def setBoolPref(self, sect, key, val):
		if sect not in self.preferences:
			self.preferences[sect] = {}
		self.preferences[sect][key] = True if val else False

	def getGeometrySizePosition(self,displayname,winname):
		_g = self.preferences['geometry'].get(str(displayname),{}).get(winname,None)
		if _g is None:
			return None
		#print(f"GOT GEOM FOR {winname}: {_g}")
		_p = re.compile('^\s*(\d+)[Xx](\d+)\+(-?\d+)\+(-?\d+)\s*$');
		_m = _p.match(_g)
		#print(f"MATCH: {_m.groups()}")
		if not _m:
			return None
		#print(f"MATCH: {_m.groups()}")
		return [int(i) for i in _m.groups()]

	def setGeometrySizePosition(self,displayname,winname,width,height,left,top):
		print("setGeometrySizePosition:","%dx%d+%d+%d" % (width, height, left, top) )
		if str(displayname) not in self.preferences['geometry']:
			self.preferences['geometry'][str(displayname)] = {}
		self.preferences['geometry'][str(displayname)][winname] = "%dx%d+%d+%d" % (width, height, left, top);

	def getGeometryValue(self,displayname,key):
		return self.preferences['geometry'].get(str(displayname),{}).get(key,None)

	def setGeometryValue(self,displayname,key,value):
		if str(displayname) not in self.preferences['geometry']:
			self.preferences['geometry'][str(displayname)] = {}
		self.preferences['geometry'][str(displayname)][key] = value

	def getPreferredUnitsOrigin(self, key):
		return self.preferences['preferredunits'].get(key,None)

	def getPreferredUnits(self, key):
		_u = self.getPreferredUnitsOrigin(key)
		##### CELSIUS TEMPERATURE WORKAROUND (see celsiusunits.py)
		if _u == "degC":
			_u = "K"
		return _u

	def setPreferredUnits(self,key,val):
		self.preferences['preferredunits'][key] = val

# Test script:
def main():
	x = Preferences();
	print(x.getStringPref('Browser','auto_solve'))
	y = Preferences();

	print("Units for length: ",x.getPreferredUnits("length"));
	print("Units for time: ",x.getPreferredUnits("time"));

	x.setPreferredUnits("length","m");
	y.setPreferredUnits("time","hr");
	print("Units for length: ",y.getPreferredUnits("length"));
	print("Units for time: ",y.getPreferredUnits("time"));
	
	assert x is y

	print("About to delete x")
	del x;
	print("Deleted x")

	y.setPreferredUnits("length","cm");

	print("explicitly save preferences on y");
	
	y.save_preferences()
	del y
	print("Deleted y")


if __name__ == "__main__":
    main()
    
# vim: ts=4:sw=4:noet:syntax=python
