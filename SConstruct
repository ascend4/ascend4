import os, commands, platform, distutils.sysconfig, os.path

version = "0.9.5.92"

#------------------------------------------------------
# OPTIONS
#
# Note that if you set the options via the command line, they will be
# remembered in the file 'options.cache'. It's a feature ;-)

opts = Options(['options.cache', 'config.py'])
#print "PLATFORM = ",platform.system()

if platform.system()=="Windows":
	default_tcl_lib = "tcl83"
	default_tk_lib = "tk83"
	default_tktable_lib = "Tktable28"
	default_install_assets = "glade/"
	icon_extension = '.png'
	default_tcl = "c:\\Tcl"
	default_tcl_libpath = "$TCL\\bin"
	default_rel_distdir = '.'
else:
	default_tcl_lib = "tcl8.3"
	default_tk_lib = "tk8.3"
	default_tktable_lib = "Tktable2.8"
	default_install_assets = "$INSTALL_SHARE/glade/"
	icon_extension = '.svg'
	default_tcl = os.path.expanduser("~/activetcl")
	default_tcl_libpath = "$TCL/lib"	
	default_rel_distdir = '../share/ascend'

	if not os.path.isdir(default_tcl):
		default_tcl = '/usr'

opts.Add(
	'CC'
	,'C Compiler command'
	,None
)

opts.Add(
	'CXX'
	,'C++ Compiler command'
	,None
)

opts.Add(BoolOption(
	'GCOV'
	, 'Whether to enable coverage testing in object code'
	, False
))

# Package linking option
opts.Add(EnumOption(
	'PACKAGE_LINKING'
	, 'Style of linking for external libraries'
	, 'DYNAMIC_PACKAGES'
    , ['DYNAMIC_PACKAGES', 'STATIC_PACKAGES', 'NO_PACKAGES']
))

opts.Add(BoolOption(
	'WITH_GCCVISIBILITY'
	,"Whether to use GCC Visibility features (only applicable if available)"
	,True
))

# You can turn off building of Tcl/Tk interface
opts.Add(BoolOption(
	'WITH_TCLTK'
	,"Set to True if you don't want to build the original Tcl/Tk GUI."
	, True
))

# You can turn off the building of the Python interface
opts.Add(BoolOption(
	'WITH_PYTHON'
	,"Set to True if you don't want to build Python wrappers."
	, True
))

# Which solvers will we allow?
opts.Add(ListOption(
	'WITH_SOLVERS'
	,"List of the solvers you want to build. The default is the minimum that"	
		+" works."
	,["QRSLV","CMSLV"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSOD','OPTSQP'
	 ]
))

# Where will the local copy of the help files be kept?
opts.Add(PackageOption(
	'WITH_LOCAL_HELP'
	, "Directory containing the local copy of the help files (optional)"
	, "no"
))

# Will bintoken support be enabled?
opts.Add(BoolOption(
	'WITH_BINTOKEN'
	,"Enable bintoken support? This means compiling models as C-code before"
		+" running them, to increase solving speed for large models."
	,False
))

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
opts.Add(
	'DEFAULT_ASCENDLIBRARY'
	,"Set the default value of the ASCENDLIBRARY -- the location where"
		+" ASCEND will look for models when running ASCEND"
	,"$INSTALL_SHARE/models"
)

# Where is SWIG?
opts.Add(
	'SWIG'
	,"SWIG location, probably only required for MinGW and MSVC users."
		+" Enter the location as a Windows-style path, for example"
		+" 'c:\\msys\\1.0\\home\\john\\swigwin-1.3.29\\swig.exe'."
)

# Build the test suite?
opts.Add(BoolOption(
	'WITH_CUNIT'
	,"Whether to build the CUnit tests. Default is off. If set to on,"
		+" you must have CUnit installed somewhere that SCons can"
		+" find it, or else use the CUNIT_* options to specify."
	,False
))

# Where are the CUnit includes?
opts.Add(PackageOption(
	'CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,'off'
))

# Where are the CUnit libraries?
opts.Add(PackageOption(
	'CUNIT_LIBPATH'
	,"Where are your CUnit libraries?"
	,'off'
))


opts.Add(
	'TCL'
	,'Base of Tcl distribution'
	,default_tcl
)

# Where are the Tcl includes?
opts.Add(
	'TCL_CPPPATH'
	,"Where are your Tcl include files?"
	,"$TCL/include"
)

# Where are the Tcl libs?
opts.Add(
	'TCL_LIBPATH'
	,"Where are your Tcl libraries?"
	,default_tcl_libpath
)

# What is the name of the Tcl lib?
opts.Add(
	'TCL_LIB'
	,"Name of Tcl lib (eg 'tcl' or 'tcl83'), for full path to static library (if STATIC_TCLTK is set)"
	,default_tcl_lib
)

# Where are the Tk includes?
opts.Add(
	'TK_CPPPATH'
	,"Where are your Tk include files?"
	,'$TCL_CPPPATH'
)

# Where are the Tk libs?
opts.Add(
	'TK_LIBPATH'
	,"Where are your Tk libraries?"
	,'$TCL_LIBPATH'
)

# What is the name of the Tk lib?
opts.Add(
	'TK_LIB'
	,"Name of Tk lib (eg 'tk' or 'tk83'), or full path to static library"
	,default_tk_lib
)	

# Static linking to TkTable

opts.Add(BoolOption(
	'STATIC_TCLTK'
	,'Set true for static linking for Tcl/Tk and TkTable. EXPERIMENTAL'
	,False
))

opts.Add(
	'TKTABLE_LIBPATH'
	,'Location of TkTable static library'
	,'$TCL_LIBPATH/Tktable2.8'
)

opts.Add(
	'TKTABLE_LIB'
	,'Stem name of TkTable (eg tktable2.8, no ".so" or "lib") shared library, or full path of static tktable (/usr/lib/...)'
	,default_tktable_lib
)

opts.Add(
	'TKTABLE_CPPPATH'
	,'Location of TkTable header file'
	,'$TCL_CPPPATH'
)

opts.Add(
	'X11'
	,'Base X11 directory. Only used when STATIC_TCLTK is turned on. EXPERIMENTAL'
	,'/usr/X11R6'
)

opts.Add(
	'X11_LIBPATH'
	,'Location of X11 lib. EXPERIMENTAL'
	,'$X11/lib'
)

opts.Add(
	'X11_CPPPATH'
	,'Location of X11 includes. EXPERIMENTAL'
	,'$X11/include'
)

opts.Add(
	'X11_LIB'
	,'Name of X11 lib. EXPERIMENTAL'
	,'X11'
)

opts.Add(
	'INSTALL_PREFIX'
	,'Root location for installed files'
	,'/usr/local'
)

opts.Add(
	'INSTALL_BIN'
	,'Location to put binaries during installation'
	,"$INSTALL_PREFIX/bin"
)

opts.Add(
	'INSTALL_LIB'
	,'Location to put binaries during installation'
	,"$INSTALL_PREFIX/lib"
)

opts.Add(
	'INSTALL_SHARE'
	,'Location to put data files during installation'
	,"$INSTALL_PREFIX/share/ascend"
)

opts.Add(
	'INSTALL_INCLUDE'
	,'Location to put header files during installation'
	,"$INSTALL_PREFIX/include"
)

opts.Add(
	'PYGTK_ASSETS'
	,'Default location for Glade assets (placed in pygtk/config.py)'
	,default_install_assets
)

opts.Add(BoolOption(
	'DEBUG'
	,"Compile source with debugger symbols, eg for use with 'gdb'"
	,False
))

opts.Add(BoolOption(
	'MALLOC_DEBUG'
	,"Compile with debugging version of MALLOC. Required for full CUnit testing"
	,False
))

opts.Add(
	'INSTALL_ROOT'
	,'For use by RPM only: location of %{buildroot} during rpmbuild'
	,""
)

opts.Add(
	'DISTTAR_NAME'
	,"Stem name of the tarball created by 'scons dist'. So for 'ascend-aaa.tar.bz2', set this to 'ascend-aaa'."
	,"ascend-"+version
)

opts.Add(BoolOption(
	'RELATIVE_PATHS'
	,"Whether to use absolute or relative paths in the installed Tcl/Tk interface. If you want to build an RPM, set this to false."
	,True
))

opts.Add(
	'WIN_INSTALLER_NAME'
	,"Name of the installer .exe to create under Windows (minus the '.exe')"
	,"ascend-"+version
)

opts.Add(BoolOption(
	'WITH_XTERM_COLORS'
	,"Set to 0 if you don't want xterm colour codes in the console output"
	,True
))

if platform.system()!="Windows":
	opts.Add(BoolOption(
		'WITH_GCCVISIBILITY'
		, 'Whether to use GCC Visibility extensions when building with GCC 4.0'
		, True
	))

if platform.system()=="Windows":
	opts.Add(BoolOption(
		'WITH_INSTALLER'
		,'Build the Windows Installer (setup program) using NSIS'
		,False
	))

# TODO: OTHER OPTIONS?
# TODO: flags for optimisation
# TODO: turning on/off bintoken functionality
# TODO: Where will the 'Makefile.bt' file be installed?

# Import the outside environment

if os.environ.get('OSTYPE')=='msys':
	env = Environment(
		ENV=os.environ
		, tools=['mingw','lex','yacc','fortran','swig','disttar','nsis']
		, toolpath=['scons']
	)
	env['IS_MINGW']=True
else:
	env = Environment(
		ENV=os.environ
		,tools=['default','lex','yacc','fortran','swig','disttar','nsis']
		, toolpath=['scons']
	)

if platform.system()=='Windows' and env.has_key('MSVS'):
	print "INCLUDE =",env['ENV']['INCLUDE']
	print "LIB =",env['ENV']['LIB']
	print "PATH =",env['ENV']['PATH']
	env.Append(CPPPATH=env['ENV']['INCLUDE'])
	env.Append(LIBPATH=env['ENV']['LIB'])

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

with_tcltk = env.get('WITH_TCLTK')
without_tcltk_reason = "disabled by options/config.py"

with_python = env.get('WITH_PYTHON')
without_python_reason = "disabled by options/config.py"

with_cunit = env.get('WITH_CUNIT')
without_cunit_reason = "not requested"

#print "SOLVERS:",env['WITH_SOLVERS']
#print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
#print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

can_install = True
if platform.system()=='Windows':
	can_install = False

env['CAN_INSTALL']=can_install

print "TCL_CPPPATH =",env['TCL_CPPPATH']
print "TCL_LIBPATH =",env['TCL_LIBPATH']
print "TCL_LIB =",env['TCL_LIB']
print "CC =",env['CC']
print "CXX =",env['CXX']

#------------------------------------------------------
# SPECIAL CONFIGURATION TESTS

need_fortran = False

#----------------
# SWIG

import os,re

def get_swig_version(env):
	cmd = env['SWIG']+' -version'
	(cin,coutcerr) = os.popen4(cmd)
	output = coutcerr.read()
	
	restr = "SWIG\\s+Version\\s+(?P<maj>[0-9]+)\\.(?P<min>[0-9]+)\\.(?P<pat>[0-9]+)\\s*$"
	expr = re.compile(restr,re.M);
	m = expr.search(output);
	if not m:
		return None
	maj = int(m.group('maj'))
	min = int(m.group('min'))
	pat = int(m.group('pat'))

	return (maj,min,pat)
	

def CheckSwigVersion(context):
	
	try:
		context.Message("Checking version of SWIG... ")
		maj,min,pat = get_swig_version(context.env)
	except:
		context.Result("Failed to detect version, or failed to run SWIG")
		return 0;
	
	context.env['SWIGVERSION']=tuple([maj,min,pat])
	
	if maj == 1 and (
			min > 3
			or (min == 3 and pat >= 24)
		):
		context.Result("ok, %d.%d.%d" % (maj,min,pat))
		return 1;
	else:
		context.Result("too old, %d.%d.%d" % (maj,min,pat))
		return 0;

#----------------
# General purpose library-and-header test

class KeepContext:
	def __init__(self,context,varprefix,static=False):
		self.keep = {}
		for k in ['LIBS','LIBPATH','CPPPATH','LINKFLAGS']:
			if context.env.has_key(k):
				self.keep[k] = context.env[k]
			else:
				self.keep[k] = None
		
		if context.env.has_key(varprefix+'_CPPPATH'):
			context.env.Append(CPPPATH=[env[varprefix+'_CPPPATH']])
			#print "Adding '"+str(cpppath_add)+"' to cpp path"

		if static:
			staticlib=env[varprefix+'_LIB']
			#print "STATIC LIB = ",staticlib
			context.env.Append(
				LINKFLAGS=[staticlib]
			)
		else:
			if context.env.has_key(varprefix+'_LIBPATH'):
				context.env.Append(LIBPATH=[env[varprefix+'_LIBPATH']])
				#print "Adding '"+str(libpath_add)+"' to lib path"

			if context.env.has_key(varprefix+'_LIB'):
				context.env.Append(LIBS=[env[varprefix+'_LIB']])
				#print "Adding '"+str(libs_add)+"' to libs"	

	def restore(self,context):
		#print "RESTORING CONTEXT"
		#print self.keep
		#print "..."
		for k in self.keep:
			if self.keep[k]==None:
				if context.env.has_key(k):
					#print "Clearing "+str(k)
					del context.env[k];
			else:
				#print "Restoring "+str(k)+" to '"+self.keep[k]+"'"				
				context.env[k]=self.keep[k];

def CheckExtLib(context,libname,text,ext='.c',varprefix=None,static=False):
	"""This method will check for variables LIBNAME_LIBPATH
	and LIBNAME_CPPPATH and try to compile and link the 
	file with the provided text, linking with the 
	library libname."""

	if static:
		context.Message( 'Checking for static '+libname+'... ' )
	else:
		context.Message( 'Checking for '+libname+'... ' )
		
	if varprefix==None:
		varprefix = libname.upper()
	
	keep = KeepContext(context,varprefix,static)

	if not context.env.has_key(varprefix+'_LIB'):
		# if varprefix_LIB were in env, KeepContext would 
		# have appended it already
		context.env.Append(LIBS=libname)

	is_ok = context.TryLink(text,ext)
	
	#print "Link success? ",(is_ok != 0)

	keep.restore(context)

#	print "Restored CPPPATH="+str(context.env['CPPPATH'])
#	print "Restored LIBS="+libname
#	print "Restored LIBPATH="+str(context.env['LIBPATH'])

	context.Result(is_ok)
	return is_ok

#----------------
# GCC

gcc_test_text = """
#ifndef __GNUC__
# error "Not using GCC"
#endif

int main(void){
	return __GNUC__;
}
"""

def CheckGcc(context):
	context.Message("Checking for GCC... ")
	is_ok = context.TryCompile(gcc_test_text,".c")
	context.Result(is_ok)
	return is_ok

#----------------
# GCC VISIBILITY feature

gccvisibility_test_text = """
#if __GNUC__ < 4
# error "Require GCC version 4 or newer"
#endif

__attribute__ ((visibility("default"))) int x;

int main(void){
	extern int x;
	x = 4;
}
"""

def CheckGccVisibility(context):
	context.Message("Checking for GCC 'visibility' capability... ")
	if not context.env.has_key('WITH_GCCVISIBILITY') or not env['WITH_GCCVISIBILITY']:
		context.Result("disabled")
		return 0
	is_ok = context.TryCompile(gccvisibility_test_text,".c")
	context.Result(is_ok)
	return is_ok

#----------------
# YACC

yacc_test_text = """
%start ROOT
   %token MSG
   %%

   ROOT:
     MSG { print("HELLO"); } 
   ;
"""

def CheckYacc(context):
	context.Message("Checking for Yacc... ")
	is_ok = context.TryCompile(yacc_test_text,".y")
	context.Result(is_ok)
	return is_ok

#----------------
# CUnit test

cunit_test_text = """
#include <CUnit/CUnit.h>
int maxi(int i1, int i2){
	return (i1 > i2) ? i1 : i2;
}

void test_maxi(void){
	CU_ASSERT(maxi(0,2) == 2);
	CU_ASSERT(maxi(0,-2) == 0);
	CU_ASSERT(maxi(2,2) == 2);

}
int main(void){
/* 	CU_initialize_registry() */
	return 0;
}
"""

def CheckCUnit(context):
	return CheckExtLib(context,'cunit',cunit_test_text)

#----------------
# Tcl test

# TCL and TK required version 8.1, 8.2, 8.3, or 8.4:
tcltk_minor_newest_acceptable = 4
tcltk_major_required = 8

tcl_check_text = r"""
#include <tcl.h>
#include <stdio.h>
int main(void){
    printf("%s",TCL_PATCH_LEVEL);
	return 0;
}
"""

def CheckTcl(context):
	return CheckExtLib(context,'tcl',tcl_check_text,static=env['STATIC_TCLTK'])

def CheckTclVersion(context):
	keep = KeepContext(context,'TCL',static=env['STATIC_TCLTK'])
	context.Message("Checking Tcl version... ")
	(is_ok,output) = context.TryRun(tcl_check_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result("failed to run check")
		return 0

	major,minor,patch = tuple([int(i) for i in output.split(".")])
	if major != tcltk_major_required or minor > tcltk_minor_newest_acceptable:
		context.Result(output+" (bad version)")
		# bad version
		return 0
		
	# good version
	context.Result(output+", good")
	return 1

#----------------
# Tk test

tk_check_text = r"""
#include <tk.h>
#include <stdio.h>
int main(void){
    printf("%s",TK_PATCH_LEVEL);
	return 0;
}
"""
def CheckTk(context):
	return CheckExtLib(context,'tk',tcl_check_text,static=env['STATIC_TCLTK'])


def CheckTkVersion(context):
	keep = KeepContext(context,'TK',static=context.env['STATIC_TCLTK'])
	context.Message("Checking Tk version... ")
	#print "LINKFLAGS =",context.env['LINKFLAGS']
	(is_ok,output) = context.TryRun(tk_check_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result("failed to run check")
		return 0

	major,minor,patch = tuple([int(i) for i in output.split(".")])
	if major != tcltk_major_required or minor > tcltk_minor_newest_acceptable:
		# bad version
		context.Result(output+" (bad version)")
		return 0
		
	# good version
	context.Result(output+" (good)")
	return 1

#----------------
# Tktable test

tktable_check_text = r"""
#include <tkTable.h>
#include <stdio.h>
int main(void){
	Table mytable;
	return 0;
}
"""

def CheckTkTable(context):
	return CheckExtLib(context,'tktable',tktable_check_text,static=env['STATIC_TCLTK'])

#---------------
# X11 test

x11_check_text = r"""
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/ObjectP.h>
#include <X11/Object.h>
int main(void){
	Object mything;
	return 0;
}
"""

def CheckX11(context):
	return CheckExtLib(context,'X11',x11_check_text)

#----------------
# GCC Version sniffing

# TODO FIXME

gcc_version4 = False

#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
		'CheckSwigVersion' : CheckSwigVersion
		, 'CheckCUnit' : CheckCUnit
		, 'CheckTcl' : CheckTcl
		, 'CheckTclVersion' : CheckTclVersion
		, 'CheckTk' : CheckTk
		, 'CheckTkVersion' : CheckTkVersion
		, 'CheckGcc' : CheckGcc
		, 'CheckGccVisibility' : CheckGccVisibility
		, 'CheckYacc' : CheckYacc
		, 'CheckTkTable' : CheckTkTable
		, 'CheckX11' : CheckX11
#		, 'CheckIsNan' : CheckIsNan
#		, 'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
#	, config_h = "config.h"
)


# Math library

#if not conf.CheckFunc('sinh') and not conf.CheckLibWithHeader(['m','c','libc'], 'math.h', 'C'):
#	print 'Did not find math library, exiting!'
#	Exit(1)

# Where is 'isnan'?

if not conf.CheckFunc('isnan'):
	print "Didn't find isnan"
#	Exit(1)

# GCC visibility

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env['WITH_GCCVISIBILITY'] and conf.CheckGccVisibility():
		conf.env['HAVE_GCCVISIBILITY']=True;
		conf.env.Append(CCFLAGS=['-fvisibility=hidden'])
		conf.env.Append(CPPDEFINES=['HAVE_GCCVISIBILITY'])

# YACC

if not conf.CheckYacc():
	print "YACC NOT FOUND OR NOT WORKING"
else:
	conf.env['HAVE_YACC']=True

conf.env['HAVE_LEX']=True

# Tcl/Tk

if with_tcltk:
	if conf.CheckTcl():
		if conf.CheckTclVersion():
			if conf.CheckTk():
				if with_tcltk and conf.CheckTkVersion():
					if env['STATIC_TCLTK']:
						if conf.CheckTkTable():
							pass
						else:
							without_tcltk_reason = "TkTable not found"
							with_tcltk = False
				else:
					without_tcltk_reason = "Require Tk version <= 8.4. See 'scons -h'"
					with_tcltk = False
			else:
				without_tcltk_reason = "Tk not found."
				with_tcltk = False
		else:
			without_tcltk_reason = "Require Tcl <= 8.4 Tcl."
			with_tcltk = False

	else:
		without_tcltk_reason = "Tcl not found."
		with_tcltk = False

if env['STATIC_TCLTK']:
	conf.CheckX11()

# Python... obviously we're already running python, so we just need to
# check that we can link to the python library OK:

if platform.system()=="Windows":
	python_lib='python24'
else:
	python_lib='python2.4'

# SWIG version

if not conf.CheckSwigVersion():
	without_python_reason = 'SWIG >= 1.3.24 is required'
	with_python = False

# CUnit

if with_cunit:
	if not conf.CheckCUnit():
		without_cunit_reason = 'CUnit not found'

# BLAS

need_blas=False
if with_tcltk:
	need_blas=True
if need_blas:
	if conf.CheckLib('blas'):
		with_local_blas = False
		without_local_blas_reason = "Found BLAS installed on system"
	else:
		with_local_blas = True
		need_fortran = True

# FORTRAN

if need_fortran:
	conf.env.Tool('f77')
	detect_fortran = conf.env.Detect(['g77','f77'])
	if detect_fortran:
		# For some reason, g77 doesn't get detected properly on MinGW
		if not env.has_key('F77'):
			conf.env.Replace(F77=detect_fortran)
			conf.env.Replace(F77COM='$F77 $F77FLAGS -c -o $TARGET $SOURCE')
			conf.env.Replace(F77FLAGS='')
			#print "F77:",conf.env['F77']
			#print "F77COM:",conf.env['F77COM']
			#print "F77FLAGS:",conf.env['F77FLAGS']
			fortran_builder = Builder(
				action='$F77COM'
				, suffix='.o'
				, src_suffix='.f'
			)
			conf.env.Append(BUILDERS={'Fortran':fortran_builder})
	else:
		print "FORTRAN-77 required but not found"
		Exit(1)
#else:
#	print "FORTRAN not required"

# TODO: -D_HPUX_SOURCE is needed

# TODO: check size of void*

# TODO: detect if dynamic libraries are possible or not

if platform.system()=="Windows" and env.has_key('MSVS'):
	if not conf.CheckHeader('windows.h') and env['PACKAGE_LINKING']=='DYNAMIC_PACKAGES':
		print "Reverting to STATIC_PACKAGES since windows.h is not available. Probably you "\
			+"need to install the Microsoft Windows Server 2003 Platform SDK, or similar."
		env['PACKAGE_LINKING']='STATIC_PACKAGES'
		
	if with_python and not conf.CheckHeader(['basetsd.h','BaseTsd.h']):
		with_python = 0;
		without_python_reason = "Header file 'basetsd.h' not found. Install the MS Platform SDK."

conf.env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

conf.Finish()

env.Append(PYTHON_LIBPATH=[distutils.sysconfig.PREFIX+"/libs"])
env.Append(PYTHON_LIB=[python_lib])
env.Append(PYTHON_CPPPATH=[distutils.sysconfig.get_python_inc()])

#---------------------------------------
# SUBSTITUTION DICTIONARY for .in files

subst_dict = {
	'@DEFAULT_ASCENDLIBRARY@':env['DEFAULT_ASCENDLIBRARY']
	, '@GLADE_FILE@':'ascend.glade'
	, '@HELP_ROOT@':''
	, '@ICON_EXTENSION@':icon_extension
	, '@INSTALL_SHARE@':env['INSTALL_SHARE']
	, '@INSTALL_BIN@':env['INSTALL_BIN']
	, '@INSTALL_INCLUDE@':env['INSTALL_INCLUDE']
	, '@PYGTK_ASSETS@':env['PYGTK_ASSETS']
	, '@VERSION@':version
	, '@DISTTAR_NAME@':env['DISTTAR_NAME']
	, '@WEBHELPROOT@':'http://pye.dyndns.org/ascend/manual/'
	, '@ASC_SHLIBSUFFIX@':env['SHLIBSUFFIX']
	, '@ASC_SHLIBPREFIX@':env['SHLIBPREFIX']
	, '@ASC_ENV_TK_DEFAULT@' : '$$ASCENDDIST/tcltk'
	, '@ASC_DISTDIR_REL_BIN@' : default_rel_distdir
}

if env.get('WITH_LOCAL_HELP'):
	print "WITH_LOCAL_HELP:",env['WITH_LOCAL_HELP']
	subst_dict['@HELP_ROOT@']=env['WITH_LOCAL_HELP']

# bool options...
for k,v in { \
		'RELATIVE_PATHS' : 'ASC_RELATIVE_PATHS', \
		'WITH_XTERM_COLORS' : 'ASC_XTERM_COLORS', \
		'MALLOC_DEBUG' : 'MALLOC_DEBUG' \
}.iteritems():
	if env.get(k):
		subst_dict['ifdef '+v]="if 1"

if with_python:
	subst_dict['@ASCXX_USE_PYTHON@']="1"

if env.has_key('HAVE_GCCVISIBILITY'):
	subst_dict['@HAVE_GCCVISIBILITY@'] = "1"

env.Append(SUBST_DICT=subst_dict)

#------------------------------------------------------
# RECIPE: SWIG scanner

import SCons.Script

SWIGScanner = SCons.Scanner.ClassicCPP(
	"SWIGScan"
	, ".i"
	, "CPPPATH"
	, '^[ \t]*[%,#][ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")'
)

env.Append(SCANNERS=[SWIGScanner])

#------------------------------------------------------
# RECIPE: 'SubstInFile', used in pygtk SConscript

import re
from SCons.Script import *  # the usual scons stuff you get in a SConscript

def TOOL_SUBST(env):
    """Adds SubstInFile builder, which substitutes the keys->values of SUBST_DICT
    from the source to the target.
    The values of SUBST_DICT first have any construction variables expanded
    (its keys are not expanded).
    If a value of SUBST_DICT is a python callable function, it is called and
    the result is expanded as the value.
    If there's more than one source and more than one target, each target gets
    substituted from the corresponding source.
    """
    env.Append(TOOLS = 'SUBST')
    def do_subst_in_file(targetfile, sourcefile, dict):
        """Replace all instances of the keys of dict with their values.
        For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
        then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
        """
        try:
            f = open(sourcefile, 'rb')
            contents = f.read()
            f.close()
        except:
            raise SCons.Errors.UserError, "Can't read source file %s"%sourcefile
        for (k,v) in dict.items():
            contents = re.sub(k, v, contents)
        try:
            f = open(targetfile, 'wb')
            f.write(contents)
            f.close()
        except:
            raise SCons.Errors.UserError, "Can't write target file %s"%targetfile
        return 0 # success

    def subst_in_file(target, source, env):
        if not env.has_key('SUBST_DICT'):
            raise SCons.Errors.UserError, "SubstInFile requires SUBST_DICT to be set."
        d = dict(env['SUBST_DICT']) # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
            else:
                raise SCons.Errors.UserError, "SubstInFile: key %s: %s must be a string or callable"%(k, repr(v))
        for (t,s) in zip(target, source):
            return do_subst_in_file(str(t), str(s), d)

    def subst_in_file_string(target, source, env):
        """This is what gets printed on the console."""
        return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
                          for (t,s) in zip(target, source)])

    def subst_emitter(target, source, env):
        """Add dependency from substituted SUBST_DICT to target.
        Returns original target, source tuple unchanged.
        """
        d = env['SUBST_DICT'].copy() # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
        Depends(target, SCons.Node.Python.Value(d))
        return target, source

    subst_action=SCons.Action.Action(subst_in_file, subst_in_file_string)
    env['BUILDERS']['SubstInFile'] = Builder(action=subst_action, emitter=subst_emitter)

TOOL_SUBST(env)

#------------------------------------------------------
# Recipe for 'CHMOD' ACTION 	 
  	 
import SCons 	 
from SCons.Script.SConscript import SConsEnvironment 	 
SConsEnvironment.Chmod = SCons.Action.ActionFactory(os.chmod, 	 
	lambda dest, mode: 'Chmod("%s", 0%o)' % (dest, mode)) 	 
  	 
def InstallPerm(env, dest, files, perm): 	 
	obj = env.Install(dest, files) 	 
	for i in obj: 	 
		env.AddPostAction(i, env.Chmod(str(i), perm)) 	 
  	 
SConsEnvironment.InstallPerm = InstallPerm 	 
  	 
# define wrappers 	 
SConsEnvironment.InstallProgram = lambda env, dest, files: InstallPerm(env, dest, files, 0755) 	 
SConsEnvironment.InstallHeader = lambda env, dest, files: InstallPerm(env, dest, files, 0644)

#------------------------------------------------------
# BUILD...

# so that #include <modulename/headername.h> works across all modules...
env.Append(CPPPATH=['#base/generic'])

if gcc_version4:
	env.Append(CCFLAGS=['-fvisibility=hidden'])

if env['DEBUG']:
	env.Append(CCFLAGS=['-g'])

if env['GCOV']:
	env.Append(
		CPPFLAGS=['-g','-fprofile-arcs','-ftest-coverage']
		, LIBS=['gcov']
		, LINKFLAGS=['-fprofile-arcs','-ftest-coverage']
	)

#-------------
# TCL/TK GUI

if with_tcltk:
	if with_local_blas:
		env.SConscript(['blas/SConscript'],'env')
	else:
		print "Skipping... BLAS won't be build:", without_local_blas_reason

	env.SConscript(['lsod/SConscript'],'env')		

	env.SConscript(['linpack/SConscript'],'env')
	env.SConscript(['tcltk/generic/interface/SConscript'],'env')
else:
	print "Skipping... Tcl/Tk GUI isn't being built:",without_tcltk_reason

#-------------
# PYTHON INTERFACE

if with_python:
	env.SConscript(['pygtk/SConscript'],'env')
else:
	print "Skipping... Python GUI isn't being built:",without_python_reason

#------------
# BASE/GENERIC SUBDIRECTORIES

dirs = ['general','utilities','compiler','solver','packages']

srcs = []
for d in dirs:
	heresrcs = env.SConscript('base/generic/'+d+'/SConscript','env')
	srcs += heresrcs

#-------------
# LIBASCEND -- all base/generic functionality

libascend = env.SharedLibrary('ascend',srcs)

#-------------
# UNIT TESTS

if with_cunit:
	testdirs = ['general','solver','utilities']
	testsrcs = []
	for testdir in testdirs:
		path = 'base/generic/'+testdir+'/test/'
		env.SConscript([path+'SConscript'],'env')
		testsrcs += [i.path for i in env['TESTSRCS_'+testdir.upper()]]

	#print "TESTSRCS =",testsrcs
		
	env.SConscript(['test/SConscript'],'env')
	env.SConscript(['base/generic/test/SConscript'],'env')

	env.Alias('test',[env.Dir('test'),env.Dir('base/generic/test')])
	
else:
	print "Skipping... CUnit tests aren't being built:",without_cunit_reason


#------------------------------------------------------
# INSTALLATION

if env.get('CAN_INSTALL'):
	# the models directory only needs to be processed for installation, no other processing required.
	env.SConscript(['models/SConscript'],'env')

	dirs = ['INSTALL_BIN','INSTALL_SHARE','INSTALL_LIB']
	install_dirs = [env['INSTALL_ROOT']+env[d] for d in dirs]

	# TODO: add install options
	env.Alias('install',install_dirs)

	env.Install(env['INSTALL_ROOT']+env['INSTALL_LIB'],libascend)

#------------------------------------------------------
# CREATE the SPEC file for generation of RPM packages

if platform.system()=="Linux":
	env.SubstInFile('ascend.spec.in')

#------------------------------------------------------
# DISTRIBUTION TAR FILE

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cc','.cache','.pyc','.cvsignore','.dblite','.log','.pl']
	, DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist']
)

tar = env.DistTar("dist/"+env['DISTTAR_NAME']
	, [env.Dir('#')]
)

#------------------------------------------------------
# RPM BUILD

#if platform.system()=="Linux":
#	pass

#------------------------------------------------------
# DEFAULT TARGETS

env.Default(['pygtk','tcltk'])

