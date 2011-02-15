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
				if platform.system()=="Windows":
					import _winreg
					x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
					y= _winreg.OpenKey(x,r"SOFTWARE\ASCEND")
					_regpath,t = _winreg.QueryValueEx(y,"Install_Dir")
					_winreg.CloseKey(y)
					_winreg.CloseKey(x)
					self.helproot = os.path.join(_regpath,"book.pdf")		
				else:
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
		_b.open(_u,autoraise=1);
		print "BACK FROM WEB CALL"

if __name__ == "__main__":
	_h = Help()
	_h.run()
