import os, os.path, platform, subprocess
from SCons.Script import *

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
	Detect IPOPT settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			pkgconfig = ['c:\\MSYS\\1.0\\bin\\sh.exe','/MinGW/bin/pkg-config']
		else:
			pkgconfig = ['/usr/bin/pkg-config']
	
		cmd = pkgconfig + ['ipopt','--libs','--cflags']
		env1 = env.Clone()
		env1['CPPPATH'] = None
		env1['LIBPATH'] = None
		env1['LIBS'] = None
		env1.ParseConfig(cmd)
		env['IPOPT_CPPPATH'] = env1.get('CPPPATH')
		env['IPOPT_LIBPATH'] = env1.get('LIBPATH')
		env['IPOPT_LIBS'] = env1.get('LIBS')
		env['HAVE_IPOPT'] = True

		print "IPOPT_LIBS =",env.get('IPOPT_LIBS')
		print "IPOPT_LIBPATH =",env.get('IPOPT_LIBPATH')
		print "IPOPT_CPPPATH =",env.get('IPOPT_CPPPATH')

	except Exception, e:
		print "FAILED IPOPT DETECTION:",e.__class__,str(e)
		env['HAVE_IPOPT'] = False

def exists(env):
	"""
	Make sure this tool exists.
	"""
	if not subprocess.call('pkg-config --exists ipopt'):
		return True
	return False

