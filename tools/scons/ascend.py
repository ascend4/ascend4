# SCons tool to determine settings for use with ASCEND modelling environment.
# If you change this file, please provide a copy back to the ASCEND project;
# hopefully someone else will also find your changes useful:
# see http://ascend.cheme.cmu.edu/

import os, platform,sys,subprocess
from SCons.Script import *

def my_parse_config_win(env,cmd):
	env1 = env.Clone()
	proc = subprocess.Popen(cmd,stdout=subprocess.PIPE)
	out = proc.communicate()[0].strip()	
	
	env1.MergeFlags(out)

	env.Append(
		ASCEND_CPPPATH = env1.get('CPPPATH') or []
		,ASCEND_CPPDEFINES = env1.get('CPPDEFINES') or []
		,ASCEND_LIBPATH = env1.get('LIBPATH') or []
		,ASCEND_LIBS = env1.get('LIBS') or []
	)

def generate(env):
	"""
	Detect ASCEND settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			try:
				import win32api
			except:
				raise RuntimeError("PyWin32 is not installed. Install it first.")

			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\ASCEND")
			BIN,t = _winreg.QueryValueEx(y,'INSTALL_BIN')
			Path = os.path.join(BIN,"ascend-config")
			if not os.path.exists(Path):
				raise RuntimeError("Could not find 'ascend-config' in your PATH")
				
			cmd = [sys.executable,Path,"--cppflags","--libs"]
			my_parse_config_win(env,cmd)

			cmd = [sys.executable,Path]
			libext = ".dll"
			libpref = ""
			
		else:
			cmd = ['ascend-config','--libs','--cppflags','--libs']
			env1 = env.Clone()
			env1.ParseConfig(cmd)
			env.Append(
				ASCEND_CPPPATH = env1.get('CPPPATH') or []
				,ASCEND_CPPDEFINES = env1.get('CPPDEFINES') or []
				,ASCEND_LIBPATH = env1.get('LIBPATH') or []
				,ASCEND_LIBS = env1.get('LIBS') or []
			)
			cmd = ['ascend-config']
			libext = ".so"
			libpref = "lib"

		# Get the suffix and prefix used for external libraries
		
		proc = subprocess.Popen(cmd+['--extlib-prefix'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		out = proc.communicate()[0].strip()
		if proc.returncode is 0:
			env.Append(ASCEND_EXTLIB_PREFIX=out)
		else:
			# old ASCEND will not provide '--extlib-prefix' so make some assumptions...
			env.Append(ASCEND_EXTLIB_PREFIX=libpref)
		proc = subprocess.Popen(cmd+['--extlib-suffix'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		out = proc.communicate()[0].strip()
		if proc.returncode is 0:
			env.Append(ASCEND_EXTLIB_SUFFIX=out)
		else:
			env.Append(ASCEND_EXTLIB_SUFFIX="_ascend%s"%libext)
			
		# Get the ASCEND model library location (it will be assumed that
		# we can install files to that location)

		proc = subprocess.Popen(cmd+['--models'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		out = proc.communicate()[0].strip()
		if proc.returncode is 0:
			env.Append(ASCEND_MODELS=out)

		env.Append(HAVE_ASCEND=True)

		#print "ASCEND_LIBS =",env.get('ASCEND_LIBS')
		#print "ASCEND_LIBPATH =",env.get('ASCEND_LIBPATH')
		#print "ASCEND_CPPPATH =",env.get('ASCEND_CPPPATH')
		#print "ASCEND_EXTLIB_SUFFIX =",env.get('ASCEND_EXTLIB_SUFFIX')
		#print "ASCEND_EXTLIB_PREFIX =",env.get('ASCEND_EXTLIB_PREFIX')
		#print "ASCEND_MODELS =",env.get('ASCEND_MODELS')

	except Exception,e:
		print "Checking for ASCEND... not found! (%s)" % str(e)
		env.Append(HAVE_ASCEND=False)
		return False
		
	print "Checking for ASCEND... found (ASCEND_CPPPATH = %s)" % env.get('ASCEND_CPPPATH')
	return True
	
def exists(env):
	"""
	Make sure this tool exists.
	"""
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\ASCEND")
			BIN,t = _winreg.QueryValueEx(y,'INSTALL_BIN')
			return True
		except:
			return False
	else:
		if env.Execute(['ascend-config','--version']):
			return True
		return False

