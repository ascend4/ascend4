import urllib2, ConfigParser, platform


class VersionCheck:
	def __init__(self):
		self.url = "http://ascend.cruncher2.dyndns.org/version.ini"
		self.download = None
		self.latest = None
		self.info = None
	def check(self):
		try:
			auth_handler = urllib2.HTTPBasicAuthHandler()
			opener = urllib2.build_opener(auth_handler)
			urllib2.install_opener(opener)
			fp = urllib2.urlopen(self.url)

			cp = ConfigParser.SafeConfigParser() 
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
		except Exception,e:
			print "Version check failed (%s)" % e

		return False

if __name__=="__main__":
	v = VersionCheck()
	if v.check():
		print v.latest
		print "Info at %s" % v.info
		if v.download:
			print "Download from %s" % v.download
