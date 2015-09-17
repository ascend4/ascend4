import urllib.request, urllib.error, urllib.parse, configparser, platform
import socket

class VersionCheck:
	def __init__(self):
		self.url = "http://www.ascend4.org/versioncheck/version.ini"
		self.download = None
		self.latest = None
		self.info = None
	def check(self):
		socket.setdefaulttimeout(1)
		auth_handler = urllib.request.HTTPBasicAuthHandler()
		opener = urllib.request.build_opener(auth_handler)
		urllib.request.install_opener(opener)
		fp = urllib.request.urlopen(self.url)

		cp = configparser.SafeConfigParser() 
		cp.readfp(fp,self.url)

		opersys = platform.system()

		if opersys=="Windows":
			self.vertype = "Windows"
			self.latest = cp.get('Windows','version').strip()
			self.info = cp.get('Windows','info').strip()
			try:
				self.download = cp.get('Windows','download').strip()
			except:
				pass
			return True
		else:
			self.vertype = "Source code"
			self.latest = cp.get('Generic','version').strip()
			self.info = cp.get('Generic','info').strip()
			try:
				self.download = cp.get('Generic','download').strip()
			except:
				pass
			return True

		raise RuntimeError("No version info available for this operating system")

if __name__=="__main__":
	v = VersionCheck()
	if v.check():
		print(v.latest)
		print("Info at %s" % v.info)
		if v.download:
			print("Download from %s" % v.download)
