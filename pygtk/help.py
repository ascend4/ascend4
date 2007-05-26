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
			
		if platform.system()=='Windows':
			try:
				import win32api
				_b = webbrowser.get('windows-default')
			except:
				print "FAILED TO IMPORT 'win32', hoping that firefox is installed..."
				_b = webbrowser.GenericBrowser('\\Progra~1\\Mozill~1\\firefox %s &');
			
			print _b
		elif platform.system()=='Linux':
			_b = webbrowser.get()	
		else:
			print "PLATFORM IS ".platform.platform(True,True)

		if self.goonline:
			_u = self.webhelproot
		else:
			_p = os.path.join(self.helproot)
			_u = "file://"+_p
		
		#print "OPENING WEB PAGE:",_u
		_b.open(_u);

if __name__ == "__main__":
	_h = Help()
	_h.run()
