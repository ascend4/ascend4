import webbrowser
import os.path
import platform
import config

class Help:
	def __init__(self,helproot=None,url=None):
		print "HELPROOT =",config.HELPROOT
		self.goonline=False

		if url is not None:
			self.webhelproot = url
			self.goonline = True
		else:
			self.webhelproot = config.WEBHELPROOT
			
			if helproot==None:
				self.helproot = os.path.expanduser(config.HELPROOT)
			else:
				self.helproot = helproot
			
			if not os.path.exists(self.helproot):
				print "LOCAL HELP FILES NOT FOUND, WILL USE ONLINE COPY"
				self.goonline = True

	def run(self,topic=None):
		_b = webbrowser.get()

		if self.goonline:
			_u = self.webhelproot
		else:
			_p = os.path.join(self.helproot)
			_u = "file://"+_p
		
		print "OPENING WEB PAGE: %s..." % _u
		_b.open(_u);
		print "BACK FROM WEB CALL"

if __name__ == "__main__":
	_h = Help()
	_h.run()
