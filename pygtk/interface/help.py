import webbrowser
import os.path

class Help:
	def __init__(self,helproot=None):
		if helproot==None:
			self.helproot = os.path.expanduser("~/src/ascend-doc/trunk/html")
		else:
			self.helproot = helproot

	def run(self,topic=None):
		#_b = webbrowser.GenericBrowser('dillo %s &');
		_b = webbrowser.Konqueror();
		_p = os.path.join(self.helproot,"book.html");
		_b.open("file:"+_p);

if __name__ == "__main__":
	_h = Help()
	_h.run()
