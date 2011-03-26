import os, os.path, platform, subprocess
from SCons.Script import *

HKLM_GRAPHVIZ = r"SOFTWARE\AT&T Research Labs\Graphviz"

munge = lambda s: s
try:
	# if we have access to GetShortPathName, we'll use it...
	import win32api
	def munge1(s):
		s1 = s
		try:
			# we can only munge the path if it actually exists
			s1 = win32api.GetShortPathName(s)
		except:
			# if it doesn't exist, we just return the un-munged path
			pass
		return s1
	munge = munge1 
except:
	pass

def generate(env):
	"""
	Detect Graphviz settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,HKLM_GRAPHVIZ)
			PATH,t = _winreg.QueryValueEx(y,"InstallPath")
			LIB = os.path.join(PATH,"lib")
			BIN = os.path.join(PATH,"bin")
			INCLUDE = os.path.join(PATH,"include","graphviz")

			env['GRAPHVIZ_CPPPATH'] = [munge(INCLUDE)]
			env['GRAPHVIZ_LIBPATH'] = [munge(BIN)]
			env['GRAPHVIZ_LIBS'] = ['gvc','graph']
			env['HAVE_GRAPHVIZ'] = True
									
		else:
			env1 = env.Clone()
			env1['CPPPATH'] = None
			env1['LIBPATH'] = None
			env1['LIBS'] = None
			try:
				cmd = ['pkg-config','libgvc','libcgraph','--cflags','--libs']
				env1.ParseConfig(cmd)
				# we might get an error from this with older versions of GraphViz, so failover to below...
				env['GRAPHVIZ_CPPPATH'] = env1.get('CPPPATH')
				env['GRAPHVIZ_LIBPATH'] = env1.get('LIBPATH')
				env['GRAPHVIZ_LIBS'] = env1.get('LIBS')
				env['HAVE_GRAPHVIZ'] = True
			except:
				cmd = ['pkg-config','libgvc','libagraph','--cflags','--libs']
				env1.ParseConfig(cmd)
				env['GRAPHVIZ_CPPPATH'] = env1.get('CPPPATH')
				env['GRAPHVIZ_LIBPATH'] = env1.get('LIBPATH')
				env['GRAPHVIZ_LIBS'] = env1.get('LIBS')
				env['HAVE_GRAPHVIZ'] = True
	
		print "GRAPHVIZ_LIBS =",env.get('GRAPHVIZ_LIBS')
		print "GRAPHVIZ_LIBPATH =",env.get('GRAPHVIZ_LIBPATH')
		print "GRAPHVIZ_CPPPATH =",env.get('GRAPHVIZ_CPPPATH')

	except Exception,e:
		print "NO GRAPHVIZ (%s)" % str(e)
		env['HAVE_GRAPHVIZ'] = False

def exists(env):
	"""
	Make sure this tool exists.
	"""
	print "CHECKING FOR GRAPHVIZ"
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,HKLM_GRAPHVIZ)
			INCLUDE,t = _winreg.QueryValueEx(y,'InstallPath')
			print "GRAPHVIZ EXISTS"
			return True
		except:
			print "GRAPHVIZ DOESN'T EXIST"
			return False
	else:
		print "NOT WINDOWS"
		if not subprocess.call('pkg-config libgvc libagraph --exists'):
			return True
		return False

