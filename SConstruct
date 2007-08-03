import sys, os, commands, platform, distutils.sysconfig, os.path, re

version = "0.9.5.112"

pyversion = "%d.%d" % (sys.version_info[0],sys.version_info[1])

#------------------------------------------------------
# OPTIONS
#
# Note that if you set the options via the command line, they will be
# remembered in the file 'options.cache'. It's a feature ;-)

opts = Options(['options.cache', 'config.py'])
#print "PLATFORM = ",platform.system()

default_tcl_cpppath = "$TCL/include"
default_tron_envvar="TRON_PATH"
default_conopt_envvar="CONOPT_PATH"

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
	default_conopt_dlpath="$CONOPT_PREFIX"
	default_conopt_lib="conopt3"

	default_tron_prefix="c:\\Program Files\\TRON"
	default_tron_dlpath="$TRON_PREFIX"
	default_tron_lib="tron1"

	default_prefix="c:\\MinGW"
	default_libpath="$DEFAULT_PREFIX\\lib"
	default_cpppath="$DEFAULT_PREFIX\\include"
	
	if not os.path.exists(default_conopt_prefix):
		default_conopt_prefix = None
		
	need_libm = False
	python_exe = "c:\\Python24\\python.exe"
	default_with_scrollkeeper=False
	pathsep = ";"
	
	default_fortran="g77"
	default_f2c_lib="g2c"
else:
	default_tcl_lib = "tcl8.4"
	default_tk_lib = "tk8.4"
	default_tktable_lib = "Tktable2.8"
	default_install_assets = "$INSTALL_ASCDATA/glade/"
	icon_extension = '.svg'
	default_tcl = '/usr'
	default_tcl_libpath = "$TCL/lib"

	if os.path.exists("/etc/debian_version"):
		default_tcl_cpppath = "/usr/include/tcl8.4"

	default_rel_distdir = '../share/ascend'
	default_absolute_paths = True
	default_ida_prefix="/usr"
	default_conopt_prefix="/usr"
	default_conopt_libpath="$CONOPT_PREFIX/lib"
	default_conopt_cpppath="$CONOPT_PREFIX/include"
	default_conopt_dlpath= default_conopt_libpath + ":/usr/local/lib"
	default_conopt_lib="consub3"

	default_tron_prefix="/usr"
	default_tron_dlpath="$TRON_PREFIX/lib"
	default_tron_lib="tron1"

	default_prefix="/usr"
	default_libpath="$DEFAULT_PREFIX/lib"
	default_cpppath="$DEFAULT_PREFIX/include"

	need_libm = True
	if not os.path.isdir(default_tcl):
		default_tcl = '/usr'
	python_exe = distutils.sysconfig.EXEC_PREFIX+"/bin/python"
	default_with_scrollkeeper=False
	pathsep = ":"
	
	default_fortran="gfortran"
	default_f2c_lib="gfortran"

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

opts.Add(BoolOption(
	'WITH_GCCVISIBILITY'
	,"Whether to use GCC Visibility features (only applicable if available)"
	,True
))

opts.Add(BoolOption(
	'WITH_SIGNALS'
	,"Whether to permit use of signals for flow control in the C-level code"
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
		+" works. The option 'LSOD' is provided for backwards compatibility"
		+"; the value 'LSODE' is preferred."
	,["QRSLV","CMSLV","LSODE","IDA","CONOPT","LRSLV","TRON","IPOPT"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSODE','LSOD','OPTSQP',"IDA","TRON","IPOPT"
	 ]
))

# Where will the local copy of the help files be kept?
opts.Add(BoolOption(
	'WITH_DOC'
	, "Should we try to build and install help files? If not, ASCEND will access online help files"
	, True
))

opts.Add(BoolOption(
	'WITH_DOC_BUILD'
	, "If true, we'll attempt to build docs. Set false, we'll assume we already have then (eg from the tarball)"
	, "$WITH_DOC"
))

opts.Add(BoolOption(
	'WITH_DOC_INSTALL'
	, "If true, SCons will install the documentation file(s). If false, assume rpm or dpkg is going to do it."
	, "$WITH_DOC"
))

opts.Add(
	'HELP_ROOT'
	, "Location of the main help file"
	, "$INSTALL_DOC/book.pdf"
)

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
	,"$INSTALL_MODELS"
)

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
opts.Add(
	'DEFAULT_ASCENDSOLVERS'
	,"Set the default value of ASCENDSOLVERS -- the location where"
		+" ASCEND will look for solver shared-library files"
	,"$INSTALL_SOLVERS"
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

# Build with MMIO matrix export support?
opts.Add(BoolOption(
	'WITH_MMIO'
	,"Include support for exporting matrices in Matrix Market format"
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
	"SUNDIALS_PREFIX"
	,"Prefix for your IDA install (IDA ./configure --prefix)"
	,default_ida_prefix
))

opts.Add(
	'SUNDIALS_CPPPATH'
	,"Where is your ida.h?"
	,"$SUNDIALS_PREFIX/include"
)

# 
opts.Add(
	'SUNDIALS_LIBPATH'
	,"Where are your SUNDIALS libraries installed?"
	,"$SUNDIALS_PREFIX/lib"
)

opts.Add(
	'SUNDIALS_LIBS'
	,"What libraries are required for SUNDIALS?"
	,['sundials_nvecserial','sundials_ida','m']
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

opts.Add(BoolOption(
	"CONOPT_LINKED"
	,"Do you want to dynamically link to CONOPT (only possible if CONOPT is available at buildtime)"
	,False
))

opts.Add(
	'CONOPT_CPPPATH'
	,"Where is your conopt.h?"
	,default_conopt_cpppath
)

opts.Add(
	'CONOPT_LIBPATH'
	,"Where is your CONOPT library installed?"
	,default_conopt_libpath
)

opts.Add(
	'CONOPT_DLPATH'
	,"What is the default search path that ASCEND should use when dlopening the CONOPT library at runtime?"
	,default_conopt_dlpath
)

opts.Add(
	'CONOPT_ENVVAR'
	,"What environment variable should be used at runtime to override the default search location for CONOPT DLL/SO?"
	,default_conopt_envvar
)

#------- IPOPT -------

opts.Add(PackageOption(
	"IPOPT_PREFIX"
	,"Prefix for your IPOPT install (IPOPT ./configure --prefix)"
	,default_conopt_prefix
))

opts.Add(
	"IPOPT_LIBS"
	,"Library linked to for IPOPT"
	,["$F2C_LIB","blas","lapack","pthread","ipopt"]
)


opts.Add(
	"IPOPT_LIBPATH"
	,"Where is your IPOPT library installed"
	,"$IPOPT_PREFIX/lib"
)

opts.Add(
	'IPOPT_CPPPATH'
	,"Where is your ipopt/IpStdCInterface.h (do not include the 'ipopt' in the path)"
	,"$IPOPT_PREFIX/include"
)


#------- TRON -------

opts.Add(
	'TRON_ENVVAR'
	,"What environment variable should be used at runtime to override the default search location for TRON DLL/SO?"
	,default_tron_envvar
)

opts.Add(
	"TRON_LIB"
	,"Library linked to for TRON"
	,"tron"
)

opts.Add(
	"TRON_PREFIX"
	,"Prefix for your TRON install"
	,default_tron_prefix
)

opts.Add(
	'TRON_DLPATH'
	,"What is the default search path that ASCEND should use when dlopening the TRON library at runtime?"
	,default_tron_dlpath
)

#-------- f2c ------

opts.Add(
	"F2C_LIB"
	,"F2C library (eg. g2c, gfortran, f2c)"
	,default_f2c_lib # the default is gfortran now
)

opts.Add(PackageOption(
	"F2C_LIBPATH"
	,"Directory containing F2C library (i.e. g2c, gfortran, f2c, etc.), if not already accessible"
	,"off"
))

opts.Add(
	"FORTRAN"
	,"Fortran compiler (eg g77, gfortran)"
	,default_fortran
)

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
	,default_tcl_cpppath
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
	'INSTALL_ASCDATA'
	,"Location of ASCEND shared data (TK, python, models etc)"
	,"$INSTALL_SHARE/ascend"
)

opts.Add(
	'INSTALL_MODELS'
	,"Location of ASCEND model files (.a4c,.a4l,.a4s)"
	,"$INSTALL_ASCDATA/models"
)

opts.Add(
	'INSTALL_SOLVERS'
	,"Location of ASCEND solvers"
	,"$INSTALL_ASCDATA/solvers"
)

opts.Add(
	'INSTALL_DOC'
	,"Location of ASCEND documentation files"
	,"$INSTALL_SHARE/doc/ascend-"+version
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

#------ f --------
opts.Add(PackageOption(
	'MFGRAPH_PREFIX'
	,"Where are your MFGRAPH files?"
	,default_prefix
))

opts.Add(PackageOption(
	'MFGRAPH_CPPPATH'
	,"Where are your MFGRAPH include files?"
	,default_cpppath
))

opts.Add(PackageOption(
	'MFGRAPH_LIBPATH'
	,"Where are your MFGRAPH libraries?"
	,default_libpath
))

opts.Add(BoolOption(
	'WITH_MFGRAPH'
	,"Link to the MFGRAPH library (if available, for generating incidence graphs)"
	,True
))


#------ ufsparse --------
opts.Add(PackageOption(
	'UFSPARSE_PREFIX'
	,"Where are your UFSPARSE files?"
	,default_prefix
))

opts.Add(PackageOption(
	'UFSPARSE_CPPPATH'
	,"Where are your UFSPARSE include files?"
	,default_cpppath
))

opts.Add(PackageOption(
	'UFSPARSE_LIBPATH'
	,"Where are your UFSPARSE libraries?"
	,default_libpath
))

opts.Add(BoolOption(
	'WITH_UFSPARSE'
	,"Link to the UFSPARSE library (if available, for additional sparse matrix routines)"
	,True
))

#-----------------------

opts.Add(BoolOption(
	'UPDATE_NO_YACC_LEX'
	,"Update the *_no_yacc* and *_no_lex* files in the source tree? (these files are created so that ASCEND can be compiled in the absence of those tools)"
	,False
))

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
	,"ascend-"+version+"-py"+pyversion+".exe"
)

opts.Add(BoolOption(
	'WITH_XTERM_COLORS'
	,"Set to 0 if you don't want xterm colour codes in the console output"
	,True
))

opts.Add(BoolOption(
	'WITH_EXTFNS'
	,"Set to 0 if you don't want to attempt to build the external modules bundled with ASCEND"
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

tools = ['lex','yacc','fortran','swig','nsis','substinfile','disttar']
if platform.system()=="Windows":
	if os.environ.get('OSTYPE')=='msys' or os.environ.get('MSYSTEM'):
		envenv = os.environ;
		tools += ['mingw']
		#TODO removed 'doxygen' for SCons 0.96.93
		envadditional['IS_MINGW']=True
	else:
		print "Assuming VC++ build environment (Note: MinGW is preferred)"
		envenv = {
			'PATH':os.environ['PATH']
			,'INCLUDE':os.environ['INCLUDE']
			,'LIB':os.environ['LIB']
			,'MSVS_IGNORE_IDE_PATHS':1
		}
		tools += ['default']
		#TODO removed 'doxygen' for SCons 0.96.93
		envadditional['CPPDEFINES']=['_CRT_SECURE_NO_DEPRECATE']
else:
	if os.environ.get('TARGET')=='mingw':
		envenv = os.environ
		tools += ['crossmingw']
		envadditional['CPPPATH']=['/usr/lib/gcc/i586-mingw32msvc/3.4.5/include','/usr/include']
		envadditional['LIBS']=['gcc']
	else:
		envenv = os.environ
		tools += ['default']
		#TODO removed 'doxygen' for SCons 0.96.93
	
	
env = Environment(
	ENV=envenv
	, toolpath=['scons']
	, tools=tools
	, **envadditional
)

#print "PATH =",os.environ['PATH']
#print "PROGSUFFIX =",env['PROGSUFFIX']
#print "CPPPATH =",env['CPPPATH']

opts.Update(env)

if 'LSOD' in env['WITH_SOLVERS']:
	if 'LSODE' not in env['WITH_SOLVERS']:
		env['WITH_SOLVERS'].append('LSODE')
	env['WITH_SOLVERS'].remove('LSOD')

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

with_mfgraph = env.get('WITH_MFGRAPH')
without_mfgraph_reason = "disabled by options/config.py"

with_ufsparse = env.get('WITH_UFSPARSE')
without_ufsparse_reason = "disabled by options/config.py"

with_mmio = env.get('WITH_MMIO')
without_mmio_reason = "disabled by options/config.py"

with_signals = env.get('WITH_SIGNALS')
without_signals_reason = "disabled by options/config.py"

with_doc = env.get('WITH_DOC')

with_doc_build = env.get('WITH_DOC_BUILD');
without_doc_build_reason = "disabled by options/config.py"
if not with_doc:
	with_doc_build = False
	without_doc_build_reason = "disabled by with_doc"

with_latex2html = False

if platform.system()=="Windows":
	with_installer=1
else:
	with_installer=0
	without_installer_reason = "only possible under Windows"
		
if 'LSODE' in env['WITH_SOLVERS']:
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

if 'IPOPT' in env['WITH_SOLVERS']:
	with_ipopt=True
else:
	with_ipopt=False
	without_ipopt_reason = "not requested (WITH_SOLVERS)"


#print "SOLVERS:",env['WITH_SOLVERS']
#print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
#print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

can_install = True
if platform.system()=='Windows':
	can_install = False

env['CAN_INSTALL']=can_install

print "TCL=",env['TCL']
print "TCL_CPPPATH =",env['TCL_CPPPATH']
print "TCL_LIBPATH =",env['TCL_LIBPATH']
print "TCL_LIB =",env['TCL_LIB']
print "CC =",env['CC']
print "CXX =",env['CXX']
print "FORTRAN=",env.get('FORTRAN')

print "ABSOLUTE PATHS =",env['ABSOLUTE_PATHS']
print "INSTALL_ASCDATA =",env['INSTALL_ASCDATA']
print "INSTALL_PREFIX =",env['INSTALL_PREFIX']
print "INSTALL_MODELS =",env['INSTALL_MODELS']
print "INSTALL_SOLVERS =",env['INSTALL_SOLVERS']
print "DEFAULT_ASCENDLIBRARY =",env['DEFAULT_ASCENDLIBRARY']
print "DEFAULT_ASCENDSOLVERS =",env['DEFAULT_ASCENDSOLVERS']


#------------------------------------------------------
# SPECIAL CONFIGURATION TESTS

need_fortran = False

#----------------
# CC

cc_test_text = """
int main(void){
	return 0;
}
""";

def CheckCC(context):
	context.Message("Checking C compiler ('%s')... " % context.env.get('CC'))
	is_ok = context.TryCompile(cc_test_text,".c")
	context.Result(is_ok)
	return is_ok

#----------------
# CXX

cxx_test_text = """
template<class X>
class pair{
public:
	X a;
	X b;
};

int main(void){
	pair<double> P;
	P.a = 0;
	return 0;
}
""";

def CheckCXX(context):
	context.Message("Checking C++ compiler ('%s')... " % context.env.get('CXX'))
	is_ok = context.TryCompile(cc_test_text,".cpp")
	context.Result(is_ok)
	return is_ok

	

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
			elif context.env.has_key(varprefix+'_LIBS'):
				context.env.AppendUnique(LIBS=env[varprefix+'_LIBS'])

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
# LEX

lex_test_text = """
%{
#include <stdio.h>
%}
DIGIT	[0-9]
ID		[a-z][a-z0-9]*
%%
{DIGIT}+	{
		printf("A digit: %s\\n",yytext);
	}

[ \\t\\n]+    /* ignore */

.			{
		printf("Unrecognized guff");
	}
%%
main(){
	yylex();
}
"""

def CheckLex(context):
	context.Message("Checking for Lex ('%s')... " % context.env.get('LEX'))
	is_ok = context.TryCompile(lex_test_text,".l")
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
# mfgraph test

mfgraph_test_text = """
#include <mfgraph/mfg_draw_graph.h>
int main(void){
	using namespace mfg;
	DrawGraph g;
	return 0;
}
"""

def CheckMFGraph(context):
	return CheckExtLib(context,'mfgraph',mfgraph_test_text,ext=".cpp")

#----------------
# ufsparse test

ufsparse_test_text = """
#include <ufsparse/cs.h>
int main(void){
	cs *A,*B,*C;
	C = cs_multiply(A,B);
	return 0;
}
"""

def CheckUFSparse(context):
	return CheckExtLib(context
		,libname='cxsparse'
		,varprefix='ufsparse'
		,text=ufsparse_test_text
		,ext=".c"
	)

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
/* this part causes problems with crossmingw... */
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
		python_libpath += [os.path.join(sys.prefix,"libs")]
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
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==2
# include <sundials/sundials_config.h>
# include <sundials/sundials_nvector.h>
# include <nvector_serial.h>
# include <ida.h>
# include <ida/ida_spgmr.h>
#else
# include <sundials/sundials_config.h>
# include <nvector/nvector_serial.h>
# include <ida/ida.h>
#endif
int main(){
	void *ida_mem;
	ida_mem = IDACreate();
	return 0;
}
"""

# slightly changed calling convention (IDACalcID) in newer versions of SUNDIALS,
# so detect the version and act accordingly.
def CheckSUNDIALS(context):
	keep = KeepContext(context,'SUNDIALS')
	context.Message("Checking for SUNDIALS... ")
	(is_ok,output) = context.TryRun(sundials_version_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result(0)
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
	context.Result("%d.%d.%d, good" % (major,minor,patch))

	return 1
	

def CheckIDA(context):
	context.Message( 'Checking for IDA... ' )

	keep = KeepContext(context,"SUNDIALS")

	major = context.env['SUNDIALS_VERSION_MAJOR']
	minor = context.env['SUNDIALS_VERSION_MINOR'] 

	cppdef = context.env.get('CPPDEFINES')

	context.env.Append(CPPDEFINES=[
		('SUNDIALS_VERSION_MAJOR',"$SUNDIALS_VERSION_MAJOR")
		,('SUNDIALS_VERSION_MINOR',"$SUNDIALS_VERSION_MINOR")
	])

	context.env['SUNDIALS_CPPPATH_EXTRA']=[]
	if major==2 and minor==2:
		context.env.Append(SUNDIALS_CPPPATH_EXTRA = ["$SUNDIALS_CPPPATH/sundials"])

	context.env.Append(CPPDEFINES=[('SUNDIALS_VERSION_MAJOR',"$SUNDIALS_VERSION_MAJOR"),('SUNDIALS_VERSION_MINOR',"$SUNDIALS_VERSION_MINOR")])
	context.env.AppendUnique(LIBS=context.env['SUNDIALS_LIBS'])
	context.env.AppendUnique(CPPPATH=context.env['SUNDIALS_CPPPATH_EXTRA'])

	is_ok = context.TryLink(ida_test_text,".c")
	context.Result(is_ok)
	
	if cppdef:
		context.env['CPPDEFINES']=cppdef
	else:
		del context.env['CPPDEFINES']

	keep.restore(context)
		
	return is_ok


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
# IPOPT test

ipopt_test_text = """
#if !defined(_WIN32)
# define FNAME_LCASE_DECOR
#endif

#include <ipopt/IpStdCInterface.h>
int main(){
	Number n;
	IpoptProblem nlp = 0;
	FreeIpoptProblem(nlp); // probably a crash if you run this
	return 0;
}
"""

def CheckIPOPT(context):
	context.Message( 'Checking for IPOPT... ' )

	keep = KeepContext(context,"IPOPT")
	is_ok = context.TryLink(ipopt_test_text,".c")
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
	/* fprintf(stderr,"Raising SIGINT\n"); */
	raise(SIGINT);
}
int main(void){
	SigHandlerFn *last,*saved;
	saved = signal(SIGINT,&sighandler);
	if(saved!=SIG_DFL){
		fprintf(stderr,"Default handler (%p) was not correctly set\n",SIG_DFL);
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
	last = signal(SIGINT,SIG_DFL);
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
		return False
	if int(output)==1:
		context.Result("required");
		context.env['ASC_RESETNEEDED'] = True
	else:
		context.Result("not required");
		context.env['ASC_RESETNEEDED'] = False
	return is_ok

#----------------
# LyX on this system?

def CheckLyx(context):
	context.Message("Checking for LyX... ")
	r = context.env.WhereIs("lyx")
	if r:
		context.Result(r)
	else:
		context.Result(0)
	return r

#----------------
# Latex2HTML on this system?

def CheckLatex2HTML(context):
	context.Message("Checking for latex2html...")
	if context.env.WhereIs("latex2html"):
		r = True
	else:
		r = False
	context.Result(r)
	return r

#----------------
# GCC Version sniffing

# TODO FIXME

gcc_version4 = False

#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
		'CheckCC' : CheckCC
		, 'CheckCXX' : CheckCXX
		, 'CheckMath' : CheckMath
		, 'CheckSwigVersion' : CheckSwigVersion
		, 'CheckPythonLib' : CheckPythonLib
		, 'CheckCUnit' : CheckCUnit
		, 'CheckDMalloc' : CheckDMalloc
		, 'CheckLyx' : CheckLyx
		, 'CheckLatex2HTML' : CheckLatex2HTML
		, 'CheckMFGraph' : CheckMFGraph
		, 'CheckUFSparse' : CheckUFSparse
		, 'CheckTcl' : CheckTcl
		, 'CheckTclVersion' : CheckTclVersion
		, 'CheckTk' : CheckTk
		, 'CheckTkVersion' : CheckTkVersion
		, 'CheckGcc' : CheckGcc
		, 'CheckGccVisibility' : CheckGccVisibility
		, 'CheckYacc' : CheckYacc
		, 'CheckLex' : CheckLex
		, 'CheckTkTable' : CheckTkTable
		, 'CheckX11' : CheckX11
		, 'CheckIDA' : CheckIDA
		, 'CheckSUNDIALS' : CheckSUNDIALS
		, 'CheckCONOPT' : CheckCONOPT
		, 'CheckIPOPT' : CheckIPOPT
		, 'CheckScrollkeeperConfig' : CheckScrollkeeperConfig
		, 'CheckFPE' : CheckFPE
		, 'CheckSIGINT' : CheckSIGINT
		, 'CheckSigReset' : CheckSigReset
#		, 'CheckIsNan' : CheckIsNan
#		, 'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
#	, config_h = "config.h"
)

def sconsversioncheck():

#	uncomment the following line to skip the version check:
#	return 1

	import SCons
	v = SCons.__version__.split(".")
	if v[0] != '0':
		return 0
	if int(v[1]) >= 97:
		return 1
	if v[1] != '96':
		return 0
	micro = int(v[2])
	if micro == 92 or micro == 93 or micro == 96:
		return 1;
	return 0

if not sconsversioncheck():
	print "Scons version is not OK. Please try version 0.96.92 or 0.96.93,"
	print "or consult the developers in the case of newer versions. Modify"
	print "the function 'sconsversioncheck' in the file SConstruct if you"
	print "want to *force* SCons to continue."
	Exit(1)

# check C compiler

if not conf.CheckCC():
	print "Failed to build simple test file with your C compiler."
	print "Check your compiler is installed and running correctly."
	Exit(1)

if not conf.CheckCXX():
	print "Failed to build simple test file with your C++ compiler."
	print "Check your compiler is installed and running correctly."
	Exit(1)

# stdio -- just to check that compiler is behaving

if not conf.CheckHeader('stdio.h'):
	print "CPPPATH =",env.get('CPPPATH')
	print "Did not find 'stdio.h'! Check your compiler configuration."
	print ""
	print "You environment is printed here:"
	for k,v in os.environ.iteritems():
		print "%-30s%s" % ("%s :" % k, v)
	Exit(1)

if not conf.CheckFunc('snprintf'):
	print "Didn't find snprintf";
	exit(1)

# Math library

conf.env['HAVE_IEEE']=True

if need_libm and not conf.CheckMath():
	conf.env['HAVE_IEEE']=False
	print 'Did not find math library, exiting!'
	Exit(1)

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

if env['WITH_SIGNALS']:
	if not conf.CheckSIGINT():
		with_signals = False
		without_signals_reason = "SIGINT uncatchable"

# Catching SIGFPE

if conf.CheckFPE():
	conf.env['HAVE_C99FPE']=True
else:
	conf.env['HAVE_C99FPE']=False

# Checking for signal reset requirement

if not conf.CheckSigReset():
	print "Unable to determine if signal reset is required"
	Exit(1)

# YACC

if conf.CheckYacc():
	conf.env['HAVE_YACC']=True

if conf.CheckLex():
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
	env['WITH_PYTHON']=False

# SWIG version

if with_python and not conf.CheckSwigVersion():
	without_python_reason = 'SWIG >= 1.3.24 is required'
	with_python = False
	env['WITH_PYTHON']=False

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

# MFGRAPH

if with_mfgraph:
	if not conf.CheckMFGraph():
		without_mfgraph_reason = 'mfgraph not found'
		with_mfgraph = False
		env['WITH_MFGRAPH'] = False

# UFSPARSE

if with_ufsparse:
	if not conf.CheckUFSparse():
		without_ufsparse_reason = 'mfgraph not found'
		with_ufsparse = False
		env['WITH_UFSPARSE'] = False

# IDA

if not with_ida:
	without_ida_reason = "Not selected (see config option WITH_SOLVERS)"
elif not conf.CheckSUNDIALS():
	with_ida = False
	without_ida_reason = "SUNDIALS not found, or bad version"
elif not conf.CheckIDA():
	with_ida = False
	without_ida_reason = "Unable to compile/link against SUNDIALS/IDA"

# CONOPT

if not with_conopt:
	without_conopt_reason = "Not selected (see config option WITH_SOLVERS)"
elif not conf.CheckCONOPT():
	if conf.env.get('CONOPT_LINKED'):
		conf.env['CONOPT_LINKED'] = False
	# we no longer require CONOPT at buildtime in order to build support for it
	#with_conopt = False
	#without_conpt_reason = "CONOPT not found"

# IPOPT

if not with_ipopt:
	without_ipopt_reason = "Not selected (see config option WITH_SOLVERS)"
elif not conf.CheckIPOPT():
	with_ipopt = False
	without_ipopt_reason = "IPOPT not found"

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
	detect_fortran = conf.env.Detect(['gfortran','g77'])
	if detect_fortran:
		# For some reason, g77 doesn't get detected properly on MinGW
		if not env.has_key('F77') and not env.has_key('FORTRAN'):
			print "Fixing detection of F77 on MinGW...(?)"
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
		if platform.system()=="Linux":
			conf.env.Append(SHFORTRANFLAGS=['-fPIC'])
	else:
		with_lsode=False;
		without_lsode_reason="FORTRAN-77 required but not found"

#else:
#	print "FORTRAN not required"

# F2C

if need_fortran:
	if platform.system()=="Windows":
		pass
		#conf.env.Append(LIBPATH='c:\mingw\lib')

# scrollkeeper

if with_scrollkeeper:
	if not conf.CheckScrollkeeperConfig():
		with_scrollkeeper=False
		without_scrollkeeper_reason="unable to detect scrollkeeper-config"

# lyx

if with_doc_build:
	if not conf.CheckLyx():
		with_doc_build = False
		without_doc_build_reason="unable to locate lyx"

	with_latext2html = conf.CheckLatex2HTML()

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

conf.Finish()

#---------------------------------------
# SUBSTITUTION DICTIONARY for .in files

release = env.get('RELEASE')
if release=="0.":
	release="0"

#print "SUBSTITUTED CONOPT_LIBPATH:",c_escape(env.subst("$CONOPT_LIBPATH"))

subst_dict = {
	'@DEFAULT_ASCENDLIBRARY@':env['DEFAULT_ASCENDLIBRARY']
	,'@DEFAULT_ASCENDSOLVERS@':env['DEFAULT_ASCENDSOLVERS']
	, '@GLADE_FILE@':'ascend.glade'
	, '@HELP_ROOT@':''
	, '@ICON_EXTENSION@':icon_extension
	, '@INSTALL_ASCDATA@':env['INSTALL_ASCDATA']
	, '@INSTALL_BIN@':env['INSTALL_BIN']
	, '@INSTALL_INCLUDE@':env['INSTALL_INCLUDE']
	, '@INSTALL_LIB@':env['INSTALL_LIB']
	, '@INSTALL_MODELS@':env['INSTALL_MODELS']
	, '@INSTALL_SOLVERS@':env['INSTALL_SOLVERS']
	, '@PYGTK_ASSETS@':env['PYGTK_ASSETS']
	, '@VERSION@':version
	, '@RELEASE@':release
	, '@DISTTAR_NAME@':env['DISTTAR_NAME']
	, '@WEBHELPROOT@':'http://pye.dyndns.org/ascend/manual/'
	, '@SHLIBSUFFIX@':env['SHLIBSUFFIX']
	, '@SHLIBPREFIX@':env['SHLIBPREFIX']
	, '@ASC_ENV_TK_DEFAULT@' : '$$ASCENDDIST/tcltk'
	, '@ASC_DISTDIR_REL_BIN@' : default_rel_distdir
	, '@PYTHON@' : python_exe
	, '@PYVERSION@' : pyversion
	, '@ASC_CONOPT_LIB@':env.get('CONOPT_LIB')
	, '@ASC_CONOPT_ENVVAR@':env.get('CONOPT_ENVVAR')
	, '@ASC_CONOPT_DLPATH@':c_escape(env.subst("$CONOPT_DLPATH"))
	, '@SOURCE_ROOT@':c_escape(os.path.abspath(str(env.Dir("#"))))
	, '@WITH_SOLVERS@':",".join(env.get('WITH_SOLVERS'))
}

if env.get('WITH_DOC'):
	print "WITH_DOC:",env['WITH_DOC']
	subst_dict['@HELP_ROOT@']=env['HELP_ROOT']

# bool options...
for k,v in {
		'ASC_WITH_IDA':with_ida
		,'ASC_WITH_DMALLOC':with_dmalloc
		,'ASC_WITH_MFGRAPH':with_mfgraph
		,'ASC_WITH_UFSPARSE':with_ufsparse
		,'ASC_WITH_CONOPT':with_conopt
		,'ASC_LINKED_CONOPT':env.get('CONOPT_LINKED')
		,'ASC_WITH_IPOPT':with_ipopt
		,'ASC_WITH_LSODE':with_lsode
		,'ASC_WITH_MMIO':with_mmio
		,'ASC_SIGNAL_TRAPS':with_signals
		,'ASC_RESETNEEDED':env.get('ASC_RESETNEEDED')
		,'HAVE_C99FPE':env.get('HAVE_C99FPE')
		,'HAVE_IEEE':env.get('HAVE_IEEE')
		,'ASC_ABSOLUTE_PATHS':env.get('ABSOLUTE_PATHS')
		,'ASC_XTERM_COLORS':env.get('WITH_XTERM_COLORS')
		,'MALLOC_DEBUG':env.get('MALLOC_DEBUG')
		}.iteritems():
		
	if v: subst_dict["/\\* #define %s @%s@ \\*/" % (k,k)]='# define %s 1 ' % k

if with_python:
	subst_dict['@ASCXX_USE_PYTHON@']="1"
	env['WITH_PYTHON']=1;

if with_latex2html:
	env['WITH_LATEX2HTML']=1

if env.has_key('HAVE_GCCVISIBILITY'):
	subst_dict['@HAVE_GCCVISIBILITY@'] = "1"

env.Append(SUBST_DICT=subst_dict)

#for k,v in subst_dict.iteritems():
#	print "%-50s%s" % ("'%s'"%k,v)

# REMOVED: long command-line support on Win2k

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
	env.Append(LINKFLAGS=['-g'])

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

if with_ipopt:
	env.Append(WITH_IPOPT=1)

#-------------
# TCL/TK GUI

if with_tcltk:
	env.SConscript(['tcltk/generic/interface/SConscript'],'env')
else:
	print "Skipping... Tcl/Tk bindings aren't being built:",without_tcltk_reason

#-------------
# PYTHON INTERFACE

if with_python:
	env.SConscript(['pygtk/SConscript'],'env')
else:
	print "Skipping... Python bindings aren't being built:",without_python_reason

#------------
# BASE/GENERIC SUBDIRECTORIES

libascend_env = env.Copy()

dirs = ['general','utilities','compiler','system','solver','integrator','packages','linear']

srcs = []
for d in dirs:
	heresrcs = libascend_env.SConscript('base/generic/'+d+'/SConscript','libascend_env')
	srcs += heresrcs

#-------------
# IMPORTED CODE: LSODE, BLAS, etc

#if with_lsode:
#	srcs += env.SConscript(['lsod/SConscript'],'env')
#	srcs += env.SConscript(['linpack/SConscript'],'env')
#else:
#	print "Skipping... LSODE won't be built:", without_lsode_reason

if with_local_blas:
	env['blasobjs'] = env.SConscript(['blas/SConscript'],'env')
else:
	env['blasobjs'] = []
	print "Skipping... BLAS won't be built:", without_local_blas_reason

if not with_ida:
	print "Skipping... IDA won't be built:", without_ida_reason

if with_mmio:
	srcs += env.SConscript(['mmio/SConscript'],'env')
else:
	print "Skipping... MMIO export won't be built:", without_mmio_reason
#-------------
# LIBASCEND -- all base/generic functionality

if with_dmalloc:
	libascend_env.Append(LIBS=['dmalloc'])

if with_ufsparse:
	libascend_env.Append(LIBS=['cxsparse'])

libascend = libascend_env.SharedLibrary('ascend',srcs)

# for use in declaring dependent shared libraries in SConscript files (eg solvers/*/SConscript)
env['libascend'] = libascend

env.Alias('libascend',libascend)

#-------------
# UNIT TESTS (C CODE)

if with_cunit:
	testdirs = ['general','solver','utilities','linear']
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
# EXTERNAL SOLVERS

env['extfns']=[]

env.SConscript(['solvers/SConscript'],'env')

#-------------
# EXTERNAL FUNCTIONS

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

	dirs = ['INSTALL_BIN','INSTALL_ASCDATA','INSTALL_LIB', 'INSTALL_INCLUDE','INSTALL_DOC']
	install_dirs = [Dir(env.subst("$INSTALL_ROOT$"+d)) for d in dirs]
	install_dirs += modeldirs + [Dir(env.subst("$INSTALL_ROOT$INSTALL_SOLVERS"))]

	# TODO: add install options
	env.Alias('install',install_dirs)

	env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_LIB")),libascend)

	env.InstallProgram(Dir(env.subst("$INSTALL_ROOT$INSTALL_BIN")),ascendconfig)

#------------------------------------------------------
# WINDOWS INSTALLER
# For the windows installer, please see pygtk/SConscript

if not env.get('NSIS'):
	with_installer = False
	without_installer_reason = "NSIS not found"

if with_installer:
	env.Append(NSISDEFINES={
		'OUTFILE':"#dist/"+env['WIN_INSTALLER_NAME']
		,"VERSION":version
		,'PYVERSION':pyversion
	})
	installer = env.Installer('nsis/installer.nsi')
	Depends(installer,["pygtk","tcltk","ascend.dll","models","ascend-config"])
	env.Alias('installer',installer)
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

#if with_scrollkeeper:
#	#env.SubstInFile('#/pygtk/gnome/ascend.omf.in')
#	#env.InstallShared(env['INSTALL_ROOT']+env['OMFDIR'],"#/pygtk/gnome/ascend.omf")

#------------------------------------------------------
# DISTRIBUTION TAR FILE

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.lib','.cc','.cache','.pyc','.cvsignore','.dblite','.log','.pl','.out','.exe','.aux','.idx','.toc','.lof','.lot','.mm','.warnings','.tm2','.swp',',tmp','.gz','.bz2','.7z']
	, DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist','debian']
)

tar = env.DistTar("dist/"+env['DISTTAR_NAME']
	, [env.Dir('#')]
)

env.Depends(tar,'ascend.spec')
env.Depends(tar,'#doc/book.pdf')

Alias('dist',tar)

#------------------------------------------------------
# DOCUMENTATION


if not with_doc_build:
	print "Skipping... Documentation isn't being built:",without_doc_build_reason

#user's manual
env.SConscript('doc/SConscript',['env'])

# doxygen documentation
env.SConscript('base/doc/SConscript',['env'])

#------------------------------------------------------
# RPM BUILD

# for RPM builds, 'scons dist' then 'rpmbuild -ta dist/ascend-*.tar.bz2'
# (check * for the version number used to create the tarball)

#------------------------------------------------------
# DEFAULT TARGETS

default_targets =['libascend','solvers']
if with_tcltk:
	default_targets.append('tcltk')
if with_python:
	default_targets.append('pygtk')
if with_installer:
	default_targets.append('installer')
if with_extfns:
	default_targets.append('extfns')
if with_doc_build:
	default_targets.append('doc')

env.Default(default_targets)

print "Building targets:"," ".join([str(i) for i in BUILD_TARGETS])

# vim: set syntax=python:
