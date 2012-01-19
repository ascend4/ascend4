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
	Detect SUNDIALS (IDA) settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			try:
				# one day, we'll provide a SUNDIALS installer so that people don't have to 
				# build their own SUNDIALS. In that case, look for the settings in the registry
				import _winreg
				x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
				y= _winreg.OpenKey(x,r"SOFTWARE\SUNDIALS")
				PATH,t = _winreg.QueryValueEx(y,"InstallPath")
				LIB = os.path.join(PATH,"lib")
				BIN = os.path.join(PATH,"bin")
				INCLUDE = os.path.join(PATH,"include")
				env['SUNDIALS_CPPPATH'] = [munge(INCLUDE)]
				env['SUNDIALS_LIBPATH'] = [munge(BIN)]
				env['SUNDIALS_LIBS'] = ['sundials_ida','sundials_nvecserial','m']
			except WindowsError:
				# if someone has installed sundials with ./configure --prefix=/MinGW using MSYS, then
				# this should work, but we would like to make this a lot more robust!
				cmd = ['sh.exe','/mingw/bin/sundials-config','-mida','-ts','-lc']
				env1 = env.Clone()
				env1['CPPPATH'] = None
				env1['LIBPATH'] = None
				env1['LIBS'] = None
				print "RUNNING sundials-config"
				env1.ParseConfig(cmd)
				env['SUNDIALS_CPPPATH'] = env1.get('CPPPATH')
				env['SUNDIALS_LIBPATH'] = env1.get('LIBPATH')
				env['SUNDIALS_LIBS'] = env1.get('LIBS')
				env['HAVE_SUNDIALS'] = True		

			env['HAVE_SUNDIALS'] = True
									
		else:
			cmd = ['sundials-config','-mida','-ts','-lc']
			env1 = env.Clone()
			env1['CPPPATH'] = None
			env1['LIBPATH'] = None
			env1['LIBS'] = None
			env1.ParseConfig(cmd)
			env['SUNDIALS_CPPPATH'] = env1.get('CPPPATH')
			env['SUNDIALS_LIBPATH'] = env1.get('LIBPATH')
			env['SUNDIALS_LIBS'] = env1.get('LIBS')
			env['HAVE_SUNDIALS'] = True

		print "SUNDIALS_LIBS =",env.get('SUNDIALS_LIBS')
		print "SUNDIALS_LIBPATH =",env.get('SUNDIALS_LIBPATH')
		print "SUNDIALS_CPPPATH =",env.get('SUNDIALS_CPPPATH')

	except Exception, e:
		print "FAILED SUNDIALS DETECTION (%s):" % platform.system(),e.__class__,str(e)
		env['HAVE_SUNDIALS'] = False

def exists(env):
	"""
	Make sure this tool exists.
	"""
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\SUNDIALS")
			INCLUDE,t = _winreg.QueryValueEx(y,'INSTALL_INCLUDE')
			return True
		except:
			return False
	else:
		if not subprocess.call('sundials-config -h'):
			return True
		return False

