import os, os.path, platform, subprocess
from SCons.Script import *
from SCons.Util import WhereIs
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

def winpath(path):
	"""
	Convert a MSYS path to a native Windows path, so that we can pass values correctly to GCC
	"""
	import subprocess
	import os
	#print "path = %s"%path
	fn = "scons%d" % os.getpid()
	while os.path.exists(fn):
		fn = fn + "0"
	try:
		f = file(fn,"w")
		f.write("#!python\nimport sys\nprint sys.argv[1]")
		f.close()
		#print "FILE %s FOUND? %d"%(fn,os.path.exists(fn))
		p1 = subprocess.Popen(["sh.exe","-c","%s %s"%(fn,path)], stdout=subprocess.PIPE)
		#p1 = subprocess.Popen(["sh.exe","-c","echo hello"], stdout=subprocess.PIPE)
		out = p1.communicate()[0].strip()
		#print "NEW PATH IS '%s'" % out
	except Exception,e:
		print "FAILED: %s"%str(e)
	finally:
		#print "Deleting %s" % fn
		os.unlink(fn)
	return out

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
				sundialsconfig = find_sundials_config(env)
				if not sundialsconfig:
					raise RuntimeError("Unable to locate sundials-config in Windows PATH")
					# if someone has installed sundials with ./configure --prefix=/MinGW using MSYS, then
				# this should work, but we would like to make this a lot more robust!
				cmd = ['sh.exe',sundialsconfig,'-mida','-ts','-lc']
				env1 = env.Clone()
				env1['CPPPATH'] = None
				env1['LIBPATH'] = None
				env1['LIBS'] = None
				#print "RUNNING sundials-config"
				env1.ParseConfig(cmd)
				env['SUNDIALS_CPPPATH'] = [munge(winpath(p)) for p in env1.get('CPPPATH')]
				env['SUNDIALS_LIBPATH'] = [munge(winpath(p)) for p in env1.get('LIBPATH')]
				env['SUNDIALS_LIBS'] = env1.get('LIBS')
				env['HAVE_SUNDIALS'] = True		

			env['HAVE_SUNDIALS'] = True
									
		else:
			sundialsconfig = env.WhereIs("sundials-config")
			if not sundialsconfig:
				raise RuntimeError("Unable to locate 'sundials-config' in PATH")
			cmd = ['sundials-config','-mida','-ts','-lc']
			env1 = env.Clone()
			env1['CPPPATH'] = None
			env1['LIBPATH'] = None
			env1['LIBS'] = None
			env1.ParseConfig(cmd)

			# tricky stuff to detect the necessary extra 'lapack' linkage if required
			if os.path.exists("/etc/lsb-release"):
				print "CHECKING SUNDIALS"
				s = env.WhereIs('sundials-config')
				if s == "/usr/bin/sundials-config":
					print "STANDARD CONFIG"
					# With Ubuntu 11.10 onwards, we need to explicitly add lapack (and blas?)
					f = file("/etc/lsb-release")
					v = {}
					for l in f:
						x = l.strip().split("=")
						v[x[0]] = x[1]
					print v
					if v['DISTRIB_ID']=="Ubuntu" and float(v['DISTRIB_RELEASE'])>=11.10:
						print "ADDING LAPACK"
						env1['LIBS'].append("lapack")

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

def find_sundials_config(env):
  """
  Try and figure out if sundials-config is installed on this machine, and if so, where.
  """
  if SCons.Util.can_read_reg:
    # If we can read the registry, get the NSIS command from it
    try:
		# 0x20019 is KEY_READ, 
		k = SCons.Util.RegOpenKeyEx(SCons.Util.hkey_mod.HKEY_LOCAL_MACHINE,'SOFTWARE\\NSIS',0,0x20019)
		val, tok = SCons.Util.RegQueryValueEx(k,None)
		ret = val + os.path.sep + 'makensis.exe'
		if os.path.exists(ret):
			return '"' + ret + '"'
		else:
			return None
    except:
		pass # Couldn't find the key, just act like we can't read the registry
  # Hope it's on the path, but note that we have to be careful with PATHEXT since sundials-config doesn't have an 
  # an executable-signifying suffix (seems like a weakness with env.WhereIs in SCons??
  return WhereIs('sundials-config',path=os.environ['PATH'],pathext="")	

def exists(env):
	if find_sundials_config(env) != None:
		return 1
	return 0

