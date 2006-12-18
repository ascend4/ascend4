import sys, os, commands, platform, distutils.sysconfig, os.path, re

version = "0.9.5.103"

#------------------------------------------------------
# OPTIONS
#
# Note that if you set the options via the command line, they will be
# remembered in the file 'options.cache'. It's a feature ;-)

opts = Options(['options.cache', 'config.py'])
#print "PLATFORM = ",platform.system()

if platform.system()=="Windows":
	default_tcl_lib = "tcl84"
	default_tk_lib = "tk84"
	default_tktable_lib = "Tktable28"
	default_install_assets = "glade/"
	icon_extension = '.png'
	default_tcl = "c:\\Tcl"
	if os.environ.get('MSYSTEM')=="MINGW32":
		default_tcl_libpath="$TCL\\bin"
	else:
		default_tcl_libpath="$TCL\\lib"
	default_rel_distdir = '.'
	default_absolute_paths = False
	
	default_ida_prefix = "c:\\MinGW"
	if not os.path.exists(default_ida_prefix):
		default_ida_prefix = None

	default_conopt_prefix = "c:\\Program Files\\CONOPT"
	default_conopt_libpath="$CONOPT_PREFIX"
	default_conopt_cpppath="$CONOPT_PREFIX"
	default_conopt_lib="conopt3"
	default_conopt_envvar="CONOPT_PATH"

	default_prefix="c:\\MinGW"
	default_libpath="$DEFAULT_PREFIX\\lib"
	default_cpppath="$DEFAULT_PREFIX\\include"
	
	if not os.path.exists(default_conopt_prefix):
		default_conopt_prefix = None
		
	need_libm = False
	python_exe = "c:\\Python24\\python.exe"
	default_with_scrollkeeper=False
else:
	default_tcl_lib = "tcl8.4"
	default_tk_lib = "tk8.4"
	default_tktable_lib = "Tktable2.8"
	default_install_assets = "$INSTALL_ASCDATA/glade/"
	icon_extension = '.svg'
	default_tcl = '/usr'
	default_tcl_libpath = "$TCL/lib"	
	default_rel_distdir = '../share/ascend'
	default_absolute_paths = True
	default_ida_prefix="/usr/local"
	default_conopt_prefix="/usr"
	default_conopt_libpath="$CONOPT_PREFIX/lib"
	default_conopt_cpppath="$CONOPT_PREFIX/include"
	default_conopt_lib="consub3"
	default_conopt_envvar="CONOPT_PATH"

	default_prefix="/usr"
	default_libpath="$DEFAULT_PREFIX/lib"
	default_cpppath="$DEFAULT_PREFIX/include"

	need_libm = True
	if not os.path.isdir(default_tcl):
		default_tcl = '/usr'
	python_exe = distutils.sysconfig.EXEC_PREFIX+"/bin/python"
	default_with_scrollkeeper=False

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
	,"Set to False if you don't want to build the original Tcl/Tk GUI."
	, True
))

# You can turn off the building of the Python interface
opts.Add(BoolOption(
	'WITH_PYTHON'
	,"Set to False if you don't want to build Python wrappers."
	, True
))

# Which solvers will we allow?
opts.Add(ListOption(
	'WITH_SOLVERS'
	,"List of the solvers you want to build. The default is the minimum that"	
		+" works."
	,["QRSLV","CMSLV","LSOD","IDA","CONOPT","LRSLV"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSOD','OPTSQP',"IDA"
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
	,"$INSTALL_ASCDATA/models"
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
	,"You can disable CUnit tests with this option. This will basically stop"
		+" SCons from parsing the SConscript files relating to the 'test'"
        +" target, which just might make things marginally faster. Probably"
		+" you can just ignore this option though. SCons will sniff for Cunit"
		+" but build the tests only if you specify the 'test' target."
	,True
))

#----- default paths -----
opts.Add(PackageOption(
	'DEFAULT_PREFIX'
	,"Where are most of the shared libraries located on your system?"
	,default_prefix
))

#------ install location for python extensions ------

# (removed for the moment)

#------ cunit --------
# Where was CUNIT installed?
opts.Add(PackageOption(
	'CUNIT_PREFIX'
	,"Where are your CUnit files?"
	,"$DEFAULT_PREFIX"
))

# Where are the CUnit includes?
opts.Add(PackageOption(
	'CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,"$CUNIT_PREFIX/include"
))

# Where are the CUnit libraries?
opts.Add(PackageOption(
	'CUNIT_LIBPATH'
	,"Where are your CUnit libraries?"
	,"$CUNIT_PREFIX/lib"
))

#-------- ida -------

opts.Add(PackageOption(
	"IDA_PREFIX"
	,"Prefix for your IDA install (IDA ./configure --prefix)"
	,default_ida_prefix
))

opts.Add(
	"IDA_LIB"
	,"Libraries linked to for IDA"
	,['sundials_nvecserial','sundials_ida','m']
)

opts.Add(
	'IDA_CPPPATH'
	,"Where is your ida.h?"
	,"$IDA_PREFIX/include"
)

opts.Add(
	'IDA_LIBPATH'
	,"Where are your SUNDIALS libraries installed?"
	,"$IDA_PREFIX/lib"
)

# ----- conopt-----

opts.Add(PackageOption(
	"CONOPT_PREFIX"
	,"Prefix for your CONOPT install (CONOPT ./configure --prefix)"
	,default_conopt_prefix
))

opts.Add(
	"CONOPT_LIB"
	,"Library linked to for CONOPT"
	,default_conopt_lib
)

opts.Add(
	'CONOPT_CPPPATH'
	,"Where is your conopt.h?"
	,default_conopt_cpppath
)

opts.Add(
	'CONOPT_LIBPATH'
	,"Where is your CONOPT libraries installed?"
	,default_conopt_libpath
)

opts.Add(
	'CONOPT_ENVVAR'
	,"What environment variable should be used at runtime to override the default search location for CONOPT DLL/SO?"
	,default_conopt_envvar
)

#-------- f2c ------

opts.Add(
	"F2C_LIB"
	,"F2C library (eg. g2c, gfortran, f2c)"
	,"g2c"
)

opts.Add(PackageOption(
	"F2C_LIBPATH"
	,"Directory containing F2C library (i.e. g2c, gfortran, f2c, etc.), if not already accessible"
	,"off"
))

#------- tcl/tk --------

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

#----- installed file locations (for 'scons install') -----

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
	,'Location to put libraries during installation'
	,"$INSTALL_PREFIX/lib"
)

opts.Add(
	'INSTALL_SHARE'
	,'Common shared-file location on this system'
	,"$INSTALL_PREFIX/share"
)

opts.Add(
	'INSTALL_DOC'
	,'Location to install documentation files'
	,"$INSTALL_SHARE/doc"
)

opts.Add(
	'INSTALL_ASCDATA'
	,"Location of ASCEND shared data (TK, python, models etc)"
	,"$INSTALL_SHARE/ascend"
)

opts.Add(
	'INSTALL_INCLUDE'
	,'Location to put header files during installation'
	,"$INSTALL_PREFIX/include"
)


opts.Add(
	'INSTALL_ROOT'
	,'For use by RPM only: location of %{buildroot} during rpmbuild'
	,""
)

#----------------------

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

#------ dmalloc --------
opts.Add(PackageOption(
	'DMALLOC_PREFIX'
	,"Where are your dmalloc files?"
	,default_prefix
))

opts.Add(PackageOption(
	'DMALLOC_CPPPATH'
	,"Where are your dmalloc include files?"
	,default_cpppath
))

opts.Add(PackageOption(
	'DMALLOC_LIBPATH'
	,"Where are your dmalloc libraries?"
	,default_libpath
))

opts.Add(BoolOption(
	'WITH_DMALLOC'
	,"Link to the DMALLOC library (if available) for debugging of memory usage."
	,False
))

#-----------------------

opts.Add(
	'DISTTAR_NAME'
	,"Stem name of the tarball created by 'scons dist'. So for 'ascend-aaa.tar.bz2', set this to 'ascend-aaa'."
	,"ascend-"+version
)

opts.Add(
	'RELEASE'
	,"Release number for use in RPM spec file. This should always start with a zero for releases made by the ASCEND group, in order that third parties can make 'patch' releases of higher version numbers."
	,"0"
)

opts.Add(BoolOption(
	'ABSOLUTE_PATHS'
	,"Whether to use absolute or relative paths in the installed Tcl/Tk interface. If you want to build an RPM, set this to false."
	,default_absolute_paths
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

opts.Add(BoolOption(
	'WITH_EXTFNS'
	,"Set to 0 if you don't want to attempt to build external modules bundled"
		+ " with ASCEND."
	,True
))

opts.Add(BoolOption(
	'WITH_SCROLLKEEPER'
	,"Set to to 1 if you want to install an OMF file that can be read by scrollkeeper (eg Yelp on GNOME)"
	,default_with_scrollkeeper
))

if platform.system()!="Windows":
	opts.Add(BoolOption(
		'WITH_GCCVISIBILITY'
		, 'Whether to use GCC Visibility extensions when building with GCC 4.0'
		, True
	))



# TODO: OTHER OPTIONS?
# TODO: flags for optimisation
# TODO: turning on/off bintoken functionality
# TODO: Where will the 'Makefile.bt' file be installed?

# Import the outside environment

def c_escape(str):
        return re.sub("\\\\","/",str)

envadditional={}

if platform.system()=="Windows":
	if os.environ.get('OSTYPE')=='msys':
		envenv = os.environ;
		tools = ['mingw','lex','yacc','fortran','swig','disttar','nsis']
		#TODO removed 'doxygen' for SCons 0.96.93
		envadditional['IS_MINGW']=True
	else:
		envenv = {
			'PATH':os.environ['PATH']
			,'INCLUDE':os.environ['INCLUDE']
			,'LIB':os.environ['LIB']
			,'MSVS_IGNORE_IDE_PATHS':1
		}
		tools=['default','lex','yacc','fortran','swig','disttar','nsis']	
		#TODO removed 'doxygen' for SCons 0.96.93
		envadditional['CPPDEFINES']=['_CRT_SECURE_NO_DEPRECATE']
else:
	if os.environ.get('TARGET')=='mingw':
		envenv = os.environ
		tools=['crossmingw','lex','yacc','disttar','nsis','doxygen']
		envadditional['CPPPATH']=['/usr/local/lib/gcc/i386-mingw32/3.4.5/include','/usr/include']
	else:
		envenv = os.environ
		tools=['default','lex','yacc','fortran','swig','disttar','nsis']
		#TODO removed 'doxygen' for SCons 0.96.93
	
	
env = Environment(
	ENV=envenv
	, toolpath=['scons']
	, tools=tools
	, **envadditional
)

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

with_tcltk = env.get('WITH_TCLTK')
without_tcltk_reason = "disabled by options/config.py"

with_python = env.get('WITH_PYTHON')
without_python_reason = "disabled by options/config.py"

with_cunit = env.get('WITH_CUNIT')
without_cunit_reason = "not requested"

with_extfns = env.get('WITH_EXTFNS')
without_extfn_reason = "disabled by options/config.py"

with_scrollkeeper = env.get('WITH_SCROLLKEEPER')
without_scrollkeeper_reason = "disabled by options/config.py"

with_dmalloc = env.get('WITH_DMALLOC')
without_dmalloc_reason = "disabled by options/config.py"

if platform.system()=="Windows":
	with_installer=1
else:
	with_installer=0
	without_installer_reason = "only possible under Windows"

if 'LSOD' in env['WITH_SOLVERS']:
	with_lsode=True
else:
	with_lsode=False
	without_lsode_reason = "not requested (WITH_SOLVERS)"
	
if 'IDA' in env['WITH_SOLVERS']:
	with_ida=True
else:
	with_ida=False
	without_ida_reason = "not requested (WITH_SOLVERS)"


if 'CONOPT' in env['WITH_SOLVERS']:
	with_conopt=True
else:
	with_conopt=False
	without_conopt_reason = "not requested (WITH_SOLVERS)"


#print "SOLVERS:",env['WITH_SOLVERS']
#print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
#print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

can_install = True
if platform.system()=='Windows':
	can_install = False

env['CAN_INSTALL']=can_install

env['INSTALL_MODELS']=env['INSTALL_ASCDATA']+"/models/"

print "TCL_CPPPATH =",env['TCL_CPPPATH']
print "TCL_LIBPATH =",env['TCL_LIBPATH']
print "TCL_LIB =",env['TCL_LIB']
print "CC =",env['CC']
print "CXX =",env['CXX']
print "FORTRAN=",env.get('FORTRAN')

print "ABSOLUTE PATHS =",env['ABSOLUTE_PATHS']
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
# Scrollkeeper (Linux documentation system)

def get_scrollkeeper_omfdir(env):
	cmd = 'scrollkeeper-config --omfdir'
	(cin,coutcerr) = os.popen4(cmd)
	output = coutcerr.read()
	return output.strip()

def CheckScrollkeeperConfig(context):
	try:
		context.Message("Checking for scrollkeeper...")
		dir=get_scrollkeeper_omfdir(context.env)
	except:
		context.Result("Failed to run 'scrollkeeper-config'")
		return 0
	context.env['OMFDIR']=dir
	context.Result("OK, %s" % dir)
	return 1

#----------------
# General purpose library-and-header test

class KeepContext:
	def __init__(self,context,varprefix,static=False):
		self.keep = {}
		for k in ['LIBS','LIBPATH','CPPPATH','LINKFLAGS']:
			#print "Keeping env %s = %s" % (k,context.env.get(k))
			self.keep[k]=context.env.get(k)
		
		if context.env.has_key(varprefix+'_CPPPATH'):
			context.env.AppendUnique(CPPPATH=[env[varprefix+'_CPPPATH']])
			#print "Adding '"+str(env[varprefix+'_CPPPATH'])+"' to cpp path"

		if static:
			staticlib=env[varprefix+'_LIB']
			#print "STATIC LIB = ",staticlib
			context.env.Append(
				LINKFLAGS=[staticlib]
			)
		else:
			if context.env.has_key(varprefix+'_LIBPATH'):
				context.env.Append(LIBPATH=[env[varprefix+'_LIBPATH']])
				#print "Adding '"+str(env[varprefix+'_LIBPATH'])+"' to lib path"

			if context.env.has_key(varprefix+'_LIB'):
				context.env.Append(LIBS=[env[varprefix+'_LIB']])
				#print "Adding '"+str(env[varprefix+'_LIB'])+"' to libs"	

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
				#print "Restoring %s to '%s'" %(k,self.keep.get(k))
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
	
	#print "LIBS is currently:",context.env.get('LIBS')
	keep = KeepContext(context,varprefix,static)

	if not context.env.has_key(varprefix+'_LIB'):
		# if varprefix_LIB were in env, KeepContext would 
		# have appended it already
		context.env.Append(LIBS=[libname])

	is_ok = context.TryLink(text,ext)
	
	#print "Link success? ",(is_ok != 0)

	keep.restore(context)

#	print "Restored CPPPATH="+str(context.env['CPPPATH'])
#	print "Restored LIBS="+str(context.env['LIBS'])
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
%{
#include <stdio.h>

/* MSVC++ needs this before it can swallow Bison output */
#ifdef _MSC_VER
# define __STDC__
#endif
%}
%token MSG
%start ROOT
%%
ROOT:
	MSG { printf("HELLO"); } 
	;
%%
"""

def CheckYacc(context):
	context.Message("Checking for Yacc ('%s')... " % context.env.get('YACC'))
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
# dmalloc test

dmalloc_test_text = """
#include <stdlib.h>
#include <dmalloc.h>

int main(void){
	char *c;
	c = malloc(100*sizeof(char));
	free(c);
	return 0;
}
"""

def CheckDMalloc(context):
	return CheckExtLib(context,'dmalloc',dmalloc_test_text)

#----------------
# MATH test

math_test_text = """
#ifndef _ALL_SOURCE
# define _ALL_SOURCE
#endif
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif
#ifndef _XOPEN_SOURCE_EXTENDED
# define _XOPEN_SOURCE_EXTENDED 1
#endif
#include <math.h>
int main(void){
	double x = 1.0; double y = 1.0; int i = 1;
	acosh(x); asinh(x); atanh(x); cbrt(x); expm1(x); erf(x); erfc(x); isnan(x);
	j0(x); j1(x); jn(i,x); ilogb(x); logb(x); log1p(x); rint(x);
	y0(x); y1(x); yn(i,x);
#ifdef _THREAD_SAFE
	gamma_r(x,&i);
	lgamma_r(x,&i);
#else
gamma(x);
	lgamma(x);
#endif
	hypot(x,y); nextafter(x,y); remainder(x,y); scalb(x,y);
	return 0;
}
"""

def CheckMath(context):
	context.Message('Checking for IEEE math library... ')
	libsave=context.env.get('LIBS');
	context.env.AppendUnique(LIBS=['m'])
	is_ok=context.TryLink(math_test_text,".c")
	context.Result(is_ok)
	if not is_ok:
		context.env['LIBS']=libsave
	return is_ok

#----------------
# libpython test

libpython_test_text = """
#include <Python.h>
int main(void){
	PyObject *p;
	p = Py_None;
	return 0;
}
"""

def CheckPythonLib(context):
	context.Message('Checking for libpython... ')

	if platform.system()=="Windows":
		python_lib='python%d%d'
	else:
		python_lib='python%d.%d'
	python_libs = [python_lib % (sys.version_info[0],sys.version_info[1])]

	python_cpppath = [distutils.sysconfig.get_python_inc()]
	cfig = distutils.sysconfig.get_config_vars()	
	
	lastLIBS = context.env.get('LIBS')
	lastLIBPATH = context.env.get('LIBPATH')
	lastCPPPATH = context.env.get('CPPPATH')
	lastLINKFLAGS = context.env.get('LINKFLAGS')

	python_libpath = []
	python_linkflags = []
	if platform.system()=="Windows":
		pass
	else:
		# checked on Linux and SunOS
		if cfig['LDLIBRARY']==cfig['LIBRARY']:
			sys.stdout.write("(static)")
			python_libpath += [cfig['LIBPL']]
			python_linkflags += cfig['LIBS'].split(' ')

	context.env.AppendUnique(LIBS=python_libs)
	context.env.AppendUnique(LIBPATH=python_libpath)
	context.env.AppendUnique(CPPPATH=python_cpppath)
	context.env.AppendUnique(LINKFLAGS=python_linkflags)
	result = context.TryLink(libpython_test_text,".c");

	context.Result(result)	

	if(result):
		context.env['PYTHON_LIBPATH']=python_libpath
		context.env['PYTHON_LIB']=python_libs
		context.env['PYTHON_CPPPATH']=python_cpppath
		context.env['PYTHON_LINKFLAGS']=python_linkflags

	context.env['LIBS'] = lastLIBS
	context.env['LIBPATH'] = lastLIBPATH
	context.env['CPPPATH'] = lastCPPPATH
	context.env['LINKFLAGS'] = lastLINKFLAGS

	return result

#----------------
# IDA test

sundials_version_major_required = 2
sundials_version_minor_min = 2
sundials_version_minor_max = 3

sundials_version_text = """
#include <sundials/sundials_config.h>
#include <stdio.h>
int main(){
	printf("%s",SUNDIALS_PACKAGE_VERSION);
	return 0;
}
"""

ida_test_text = """
# include <ida/ida.h>
# include <nvector/nvector_serial.h>
# include <ida/ida_spgmr.h>
int main(){
	void *ida_mem;
	ida_mem = IDACreate();
	return 0;
}
"""

def CheckIDA(context):
	context.Message( 'Checking for IDA (SUNDIALS)... ' )

	keep = KeepContext(context,"IDA")
	
	is_ok = context.TryLink(ida_test_text,".c")
	context.Result(is_ok)
	
	keep.restore(context)
		
	return is_ok

# slightly changed calling convention (IDACalcID) in newer versions of SUNDIALS,
# so detect the version and act accordingly.
def CheckIDAVersion(context):
	keep = KeepContext(context,'IDA')
	context.Message("Checking SUNDIALS version... ")
	(is_ok,output) = context.TryRun(sundials_version_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result("failed to run check")
		return 0

	major,minor,patch = tuple([int(i) for i in output.split(".")])
	context.env['SUNDIALS_VERSION_MAJOR'] = major
	context.env['SUNDIALS_VERSION_MINOR'] = minor
	if major != sundials_version_major_required \
			or minor < sundials_version_minor_min \
			or minor > sundials_version_minor_max:
		context.Result(output+" (bad version)")
		# bad version
		return 0
		
	# good version
	context.Result(output+", good")
	return 1
	
#----------------
# CONOPT test

conopt_test_text = """
#if !defined(_WIN32)
# define FNAME_LCASE_DECOR
#endif

#include <conopt.h>
#include <stdlib.h>
int main(){
	int s, *v, e;
	s = COIDEF_Size();
	v = (int *)malloc(s*sizeof(int));
	e = COIDEF_Ini(v);
	return e;
}
"""

def CheckCONOPT(context):
	context.Message( 'Checking for CONOPT... ' )

	keep = KeepContext(context,"CONOPT")
	
	is_ok = context.TryLink(conopt_test_text,".c")
	context.Result(is_ok)
	
	keep.restore(context)
		
	return is_ok

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
	return CheckExtLib(context,'tk',tk_check_text,static=env['STATIC_TCLTK'])


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
# Check that we can raise and catch sigint

sigint_test_text = r"""
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
static jmp_buf g_jmpenv;
void sighandler(int sig){
	longjmp(g_jmpenv,sig);
}
void testsigint(){
	raise(SIGINT);
}
int main(void){
	signal(SIGINT,&sighandler);
	switch(setjmp(g_jmpenv)){
		case 0:
			testsigint();
			exit(1);
		case SIGINT:
			exit(0);
		default:
			exit(2);
	}
}
"""

def CheckSIGINT(context):
	context.Message("Checking SIGINT is catchable... ")
	(is_ok,output)=context.TryRun(sigint_test_text,".c")
	context.Result(is_ok)
	return is_ok

#----------------
# Check that we're able to catch floating point errors

sigfpe_test_text = r"""
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <fenv.h>
static jmp_buf g_jmpenv;
void fpehandler(int sig){
	longjmp(g_jmpenv,sig);
}
int main(void){
	fenv_t myfenv;
    fegetenv(&myfenv);
    fesetenv(&myfenv);
	feenableexcept(FE_ALL_EXCEPT);
	signal(SIGFPE,&fpehandler);
	double x;
	switch(setjmp(g_jmpenv)){
		case 0:
			x = 1.0 / 0.0;
			/* failed to catch */
			exit(1);			
		case SIGFPE:
			exit(0);
	}
}
"""

def CheckFPE(context):
	context.Message("Checking C99 FPE behaviour... ")
	(is_ok,output) = context.TryRun(sigfpe_test_text,'.c')
	context.Result(is_ok)
	return is_ok

#----------------
# signal reset needed?

sigreset_test_text = r"""
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
typedef void SigHandlerFn(int);
static jmp_buf g_jmpenv;
void sighandler(int sig){
	longjmp(g_jmpenv,sig);
}
void testsigint(){
	fprintf(stderr,"Raising SIGINT\n");
	raise(SIGINT);
}
int main(void){
	SigHandlerFn *last,*saved;
	saved = signal(SIGINT,&sighandler);
	if(saved!=SIG_DFL){
		fprintf(stderr,"Default handler was not correctly set\n");
		exit(3);
	}
	switch(setjmp(g_jmpenv)){
		case 0:
			testsigint();
			fprintf(stderr,"Back from SIGINT\n");
			exit(1);
		case SIGINT:
			break;
		default:
			exit(2);
	};
	last = signal(SIGINT,(saved!=NULL)?saved:SIG_DFL);
	if(last!=&sighandler){
		printf("1");
		exit(0);
	}
	printf("0");
	exit(0);
}
"""

def CheckSigReset(context):
	context.Message("Checking signal handler reset... ")
	(is_ok,output) = context.TryRun(sigreset_test_text,'.c')
	if not is_ok:
		context.Result("ERROR")
		return 0
	if(int(output)):
		context.Result("required");
		context.env['ASC_RESETNEEDED'] = True
	else:
		context.Result("not required");
		context.env['ASC_RESETNEEDED'] = False
	return is_ok

#----------------
# GCC Version sniffing

# TODO FIXME

gcc_version4 = False

#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
		'CheckMath' : CheckMath
		, 'CheckSwigVersion' : CheckSwigVersion
		, 'CheckPythonLib' : CheckPythonLib
		, 'CheckCUnit' : CheckCUnit
		, 'CheckDMalloc' : CheckDMalloc
		, 'CheckTcl' : CheckTcl
		, 'CheckTclVersion' : CheckTclVersion
		, 'CheckTk' : CheckTk
		, 'CheckTkVersion' : CheckTkVersion
		, 'CheckGcc' : CheckGcc
		, 'CheckGccVisibility' : CheckGccVisibility
		, 'CheckYacc' : CheckYacc
		, 'CheckTkTable' : CheckTkTable
		, 'CheckX11' : CheckX11
		, 'CheckIDA' : CheckIDA
		, 'CheckIDAVersion' : CheckIDAVersion
		, 'CheckCONOPT' : CheckCONOPT
		, 'CheckScrollkeeperConfig' : CheckScrollkeeperConfig
		, 'CheckFPE' : CheckFPE
		, 'CheckSIGINT' : CheckSIGINT
		, 'CheckSigReset' : CheckSigReset
#		, 'CheckIsNan' : CheckIsNan
#		, 'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
#	, config_h = "config.h"
)

# stdio -- just to check that compiler is behaving

if not conf.CheckHeader('stdio.h'):
	print "CPPPATH =",env.get('CPPPATH')
	print "Did not find 'stdio.h'! Check your compiler configuration."
	Exit(1)

# Math library

if need_libm:
	if not conf.CheckMath():
		print 'Did not find math library, exiting!'
		Exit(1)
	#pass

# Where is 'isnan'?

if not conf.CheckFunc('isnan') and not conf.CheckFunc('_isnan'):
	print "Didn't find isnan"
#	Exit(1)

# GCC visibility

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env['WITH_GCCVISIBILITY'] and conf.CheckGccVisibility():
		conf.env['HAVE_GCCVISIBILITY']=True;
		conf.env.Append(CCFLAGS=['-fvisibility=hidden'])
		conf.env.Append(CPPDEFINES=['HAVE_GCCVISIBILITY'])
	conf.env.Append(CCFLAGS=['-Wall'])

# Catching SIGINT

if not conf.CheckSIGINT():
	print "SIGINT unable to be caught. Aborting."
	exit(1)

# Catching SIGFPE

if conf.CheckFPE():
	conf.env['HAVE_C99FPE']=True
else:
	conf.env['HAVE_C99FPE']=False

# Checking for signal reset requirement

if not conf.CheckSigReset():
	print "Unable to determine if signal reset is required"
	exit(1)

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

if not conf.CheckPythonLib():
	without_python_reason = 'libpython2.x not found or not linkable'
	with_python = False

# SWIG version

if with_python and not conf.CheckSwigVersion():
	without_python_reason = 'SWIG >= 1.3.24 is required'
	with_python = False

# CUnit

if with_cunit:
	if not conf.CheckCUnit():
		without_cunit_reason = 'CUnit not found'
		with_cunit = False
		#print "CUNIT NOT FOUND, LIBS=",conf.env.get('LIBS')

# DMALLOC

if with_dmalloc:
	if not conf.CheckDMalloc():
		without_dmalloc_reason = 'dmalloc not found'
		with_dmalloc = False

# IDA

if not with_ida:
	without_ida_reason = "Not selected (see config option WITH_SOLVERS)"
elif not conf.CheckIDA():
	with_ida = False
	without_ida_reason = "IDA not found"
elif not conf.CheckIDAVersion():
	with_ida = False
	without_ida_reason = "Unsupported (or undetected) SUNDIALS version"

# CONOPT

if not with_conopt:
	without_conopt_reason = "Not selected (see config option WITH_SOLVERS)"
elif not conf.CheckCONOPT():
	with_conopt = False
	without_conpt_reason = "CONOPT not found"

# BLAS

need_blas=False

if with_lsode:
	need_fortran = True
	need_blas=True

if need_blas:
	if conf.CheckLib('blas'):
		with_local_blas = False
		without_local_blas_reason = "Found BLAS installed on system"
	else:
		with_local_blas = True
		need_fortran = True
else:
	with_local_blas= False;
	without_local_blas_reason = "BLAS not required"

# FORTRAN

if need_fortran:
	conf.env.Tool('fortran')
	detect_fortran = conf.env.Detect(['g77','f77','gfortran'])
	if detect_fortran:
		# For some reason, g77 doesn't get detected properly on MinGW
		if not env.has_key('F77') and not env.has_key('FORTRAN'):
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
		with_lsode=False;
		without_lsode_reason="FORTRAN-77 required but not found"

#else:
#	print "FORTRAN not required"

# F2C

if need_fortran:
	if platform.system()=="Windows":
		conf.env.Append(LIBPATH='c:\mingw\lib')

# scrollkeeper

if with_scrollkeeper:
	if not conf.CheckScrollkeeperConfig():
		with_scrollkeeper=False
		without_scrollkeeper_reason="unable to detect scrollkeeper-config"

# TODO: -D_HPUX_SOURCE is needed

# TODO: check size of void*

# TODO: detect if dynamic libraries are possible or not

if platform.system()=="Windows" and env.has_key('MSVS'):
	_found_windows_h = conf.CheckHeader('Windows.h')

	if not _found_windows_h:
		print "Could not locate 'Windows.h' in CPPPATH. Check your configuration."
		Exit(1)

	if with_python and not conf.CheckHeader(['basetsd.h','BaseTsd.h']):
		with_python = 0;
		without_python_reason = "Header file 'basetsd.h' not found. Install the MS Platform SDK."

conf.env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

conf.Finish()

#---------------------------------------
# SUBSTITUTION DICTIONARY for .in files

release = env.get('RELEASE')
if release=="0.":
	release="0"

#print "SUBSTITUTED CONOPT_LIBPATH:",c_escape(env.subst("$CONOPT_LIBPATH"))

subst_dict = {
	'@DEFAULT_ASCENDLIBRARY@':env['DEFAULT_ASCENDLIBRARY']
	, '@GLADE_FILE@':'ascend.glade'
	, '@HELP_ROOT@':''
	, '@ICON_EXTENSION@':icon_extension
	, '@INSTALL_ASCDATA@':env['INSTALL_ASCDATA']
	, '@INSTALL_BIN@':env['INSTALL_BIN']
	, '@INSTALL_INCLUDE@':env['INSTALL_INCLUDE']
	, '@INSTALL_LIB@':env['INSTALL_LIB']
	, '@INSTALL_MODELS@':env['INSTALL_MODELS']
	, '@PYGTK_ASSETS@':env['PYGTK_ASSETS']
	, '@VERSION@':version
	, '@RELEASE@':release
	, '@DISTTAR_NAME@':env['DISTTAR_NAME']
	, '@WEBHELPROOT@':'http://pye.dyndns.org/ascend/manual/'
	, '@ASC_SHLIBSUFFIX@':env['SHLIBSUFFIX']
	, '@ASC_SHLIBPREFIX@':env['SHLIBPREFIX']
	, '@ASC_ENV_TK_DEFAULT@' : '$$ASCENDDIST/tcltk'
	, '@ASC_DISTDIR_REL_BIN@' : default_rel_distdir
	, '@PYTHON@' : python_exe
	, '@ASC_CONOPT_LIB@':env.get('CONOPT_LIB')
	, '@ASC_CONOPT_ENVVAR@':env.get('CONOPT_ENVVAR')
	, '@ASC_CONOPT_DLPATH@':c_escape(env.subst("$CONOPT_LIBPATH"))
	, '@SOURCE_ROOT@':os.path.abspath(str(env.Dir("#")))
}

if env.get('WITH_LOCAL_HELP'):
	print "WITH_LOCAL_HELP:",env['WITH_LOCAL_HELP']
	subst_dict['@HELP_ROOT@']=env['WITH_LOCAL_HELP']

# bool options...
for k,v in {
		'ABSOLUTE_PATHS' : 'ASC_ABSOLUTE_PATHS'
		,'WITH_XTERM_COLORS' : 'ASC_XTERM_COLORS'
		,'MALLOC_DEBUG' : 'MALLOC_DEBUG'
}.iteritems():
	if env.get(k):
#		subst_dict['@'+v+'@']='1'
		subst_dict["/\\* #define "+v+' @'+v+"@ \\*/"]='# define '+v+' 1 '

if with_ida:
	subst_dict["/\\* #define ASC_WITH_IDA @ASC_WITH_IDA@ \\*/"]='#define ASC_WITH_IDA '

if with_dmalloc:
	subst_dict["/\\* #define ASC_WITH_DMALLOC @ASC_WITH_DMALLOC@ \\*/"]='#define ASC_WITH_DMALLOC '

if with_conopt:
	subst_dict["/\\* #define ASC_WITH_CONOPT @ASC_WITH_CONOPT@ \\*/"]='#define ASC_WITH_CONOPT '

if with_lsode:
	subst_dict["/\\* #define ASC_WITH_LSODE @ASC_WITH_LSODE@ \\*/"]='#define ASC_WITH_LSODE '

if with_python:
	subst_dict['@ASCXX_USE_PYTHON@']="1"
	env['WITH_PYTHON']=1;

if env.has_key('HAVE_GCCVISIBILITY'):
	subst_dict['@HAVE_GCCVISIBILITY@'] = "1"

if env.get('ASC_RESETNEEDED'):
	subst_dict["/\\* #define ASC_RESETNEEDED @ASC_RESETNEEDED@ \\*/"]='#define ASC_RESETNEEDED '

if env.get('HAVE_C99FPE'):
	subst_dict["/\\* #define HAVE_C99FPE @HAVE_C99FPE@ \\*/"]='#define HAVE_C99FPE '

env.Append(SUBST_DICT=subst_dict)

#------------------------------------------------------
# RECIPE: Fix up long command-line bug on Win2k

# Currently this is broken, awaiting help from the SCons users list

if 0 and env['PLATFORM'] == 'win32':
    import win32file
    import win32event
    import win32process
    import win32security
    import string

    def my_spawn(sh, escape, cmd, args, spawnenv):
        for var in spawnenv:
            spawnenv[var] = spawnenv[var].encode('ascii', 'replace')

        sAttrs = win32security.SECURITY_ATTRIBUTES()
        StartupInfo = win32process.STARTUPINFO()
        newargs = string.join(map(escape, args[1:]), ' ')
        cmdline = cmd + " " + newargs

        # check for any special operating system commands
        if cmd == 'del':
            for arg in args[1:]:
                win32file.DeleteFile(arg)
            exit_code = 0
        else:
            # otherwise execute the command.
            hProcess, hThread, dwPid, dwTid = win32process.CreateProcess(None, cmdline, None, None, 1, 0, spawnenv, None, StartupInfo)
            win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
            exit_code = win32process.GetExitCodeProcess(hProcess)
            win32file.CloseHandle(hProcess);
            win32file.CloseHandle(hThread);
        return exit_code

    env['SPAWN'] = my_spawn

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
SConsEnvironment.InstallShared = lambda env, dest, files: InstallPerm(env, dest, files, 0644)

#------------------------------------------------------
# BUILD...

# so that #include <modulename/headername.h> works across all modules...
env.AppendUnique(CPPPATH=['#base/generic'])

if env['DEBUG']:
	env.Append(CCFLAGS=['-g'])

if env['GCOV']:
	env.Append(
		CPPFLAGS=['-g','-fprofile-arcs','-ftest-coverage']
		, LIBS=['gcov']
		, LINKFLAGS=['-fprofile-arcs','-ftest-coverage']
	)

if with_ida:
	env.Append(WITH_IDA=1)

if with_conopt:
	env.Append(WITH_CONOPT=1)

#-------------
# TCL/TK GUI

if with_tcltk:
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

libascend_env = env.Copy()

dirs = ['general','utilities','compiler','solver','packages']

srcs = []
for d in dirs:
	heresrcs = libascend_env.SConscript('base/generic/'+d+'/SConscript','libascend_env')
	srcs += heresrcs

#-------------
# IMPORTED CODE: LSODE, BLAS, etc

if with_lsode:
	srcs += env.SConscript(['lsod/SConscript'],'env')
	srcs += env.SConscript(['linpack/SConscript'],'env')
else:
	print "Skipping... LSODE won't be built:", without_lsode_reason

if with_local_blas:
	srcs += env.SConscript(['blas/SConscript'],'env')
else:
	print "Skipping... BLAS won't be built:", without_local_blas_reason

if not with_ida:
	print "Skipping... IDA won't be built:", without_ida_reason

#-------------
# LIBASCEND -- all base/generic functionality

if with_dmalloc:
	libascend_env.Append(LIBS=['dmalloc'])

libascend = libascend_env.SharedLibrary('ascend',srcs)

env.Alias('libascend',libascend)

#-------------
# UNIT TESTS (C CODE)

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

#-------------
# EXTERNAL FUNCTIONS

env['extfns']=[]
modeldirs = env.SConscript(['models/SConscript'],'env')

if not with_extfns:
	print "Skipping... External modules aren't being built:",without_extfns_reason

env.Alias('extfns',env['extfns'])

#------------------------------------------------------
# CREATE ASCEND-CONFIG scriptlet

ascendconfig = env.SubstInFile('ascend-config.in')

#------------------------------------------------------
# INSTALLATION

if env.get('CAN_INSTALL'):

	dirs = ['INSTALL_BIN','INSTALL_ASCDATA','INSTALL_LIB', 'INSTALL_INCLUDE', 'INSTALL_DOC']
	install_dirs = [env.Entry(env['INSTALL_ROOT']+env[d]) for d in dirs]
	install_dirs += modeldirs

	# TODO: add install options
	env.Alias('install',install_dirs)

	env.InstallShared(env['INSTALL_ROOT']+env['INSTALL_LIB'],libascend)

	env.InstallProgram(env['INSTALL_ROOT']+env['INSTALL_BIN'],ascendconfig)

#------------------------------------------------------
# WINDOWS INSTALLER
# For the windows installer, please see pygtk/SConscript

if with_installer:
	pass
else:
	print "Skipping... Windows installer isn't being built:",without_installer_reason

#------------------------------------------------------
# PROJECT FILE for MSVC

env.SConscript(['base/msvc/SConscript'],['env','libascend']);

#------------------------------------------------------
# CREATE the SPEC file for generation of RPM packages

if platform.system()=="Linux":
	env.SubstInFile('ascend.spec.in')

#------------------------------------------------------
# CREATE OMF FILE FOR USE WITH SCROLLKEEPER

if with_scrollkeeper:
	env.SubstInFile('#/pygtk/gnome/ascend.omf.in')
	env.InstallShared(env['INSTALL_ROOT']+env['OMFDIR'],"#/pygtk/gnome/ascend.omf")

#------------------------------------------------------
# DISTRIBUTION TAR FILE

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cc','.cache','.pyc','.cvsignore','.dblite','.log','.pl','.out']
	, DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist']
)

tar = env.DistTar("dist/"+env['DISTTAR_NAME']
	, [env.Dir('#')]
)

env.Depends(tar,'ascend.spec')

Alias('dist',tar)

#------------------------------------------------------
# USER'S MANUAL

env.SConscript('doc/SConscript',['env'])

#------------------------------------------------------
# LIBASCEND DOXYGEN DOCUMENTATION

env.SConscript('base/doc/SConscript',['env'])

#------------------------------------------------------
# RPM BUILD

# for RPM builds, 'scons dist' then 'rpmbuild -ta dist/ascend-*.tar.bz2'
# (check * for the version number used to create the tarball)

#------------------------------------------------------
# DEFAULT TARGETS

default_targets =['libascend']
if with_tcltk:
	default_targets.append('tcltk')
if with_python:
	default_targets.append('pygtk')
if with_installer:
	default_targets.append('installer')
if with_extfns:
	default_targets.append('extfns')

env.Default(default_targets)

print "Building targets:"," ".join([str(i) for i in BUILD_TARGETS])

# vim: set syntax=python:

