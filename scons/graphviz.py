import os, platform, subprocess
from SCons.Script import *

def generate(env):
	"""
	Detect Graphviz settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\graphviz")
			LIB,t = _winreg.QueryValueEx(y,"INSTALL_LIB")
			BIN,t = _winreg.QueryValueEx(y,"INSTALL_BIN")
			INCLUDE,t = _winreg.QueryValueEx(y,"INSTALL_INCLUDE")

			env['GRAPHVIZ_CPPPATH'] = [INCLUDE]
			env['GRAPHVIZ_LIBPATH'] = [LIB]
			env['GRAPHVIZ_LIBS'] = ['graph']
			env['HAVE_GRAPHVIZ'] = True
									
		else:
			cmd = ['pkg-config','libgvc','libagraph','--cflags','--libs']
			env1 = env.Copy()
			env1['CPPPATH'] = None
			env1['LIBPATH'] = None
			env1['LIBS'] = None
			env1.ParseConfig(cmd)
			env['GRAPHVIZ_CPPPATH'] = env1.get('CPPPATH')
			env['GRAPHVIZ_LIBPATH'] = env1.get('LIBPATH')
			env['GRAPHVIZ_LIBS'] = env1.get('LIBS')
			env['HAVE_GRAPHVIZ'] = True

		print "GRAPHVIZ_LIBS =",env.get('GRAPHVIZ_LIBS')
		print "GRAPHVIZ_LIBPATH =",env.get('GRAPHVIZ_LIBPATH')
		print "GRAPHVIZ_CPPPATH =",env.get('GRAPHVIZ_CPPPATH')

	except:
		env['HAVE_GRAPHVIZ'] = False

def exists(env):
	"""
	Make sure this tool exists.
	"""
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\graphviz")
			INCLUDE,t = _winreg.QueryValueEx(y,'INSTALL_INCLUDE')
			return True
		except:
			return False
	else:
		if not subprocess.call('pkg-config libgvc libagraph --exists'):
			return True
		return False

