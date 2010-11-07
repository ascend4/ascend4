#!/usr/bin/python invoke_using_scons
# This is a build script for use with SCons. Use it to compile ASCEND on 
# Linux, Windows. It should also give some success on Mac, although this is
# much less tested.

# version number for this ASCEND build:
version = "0.9.8"

# shared library API numbering, for Linux (FIXME windows too?)
soname_major = ".1"
soname_minor = ".0"

import sys, os, commands, platform, distutils.sysconfig, os.path, re, types
import subprocess

# version number for python, useful on Windows
pyversion = "%d.%d" % (sys.version_info[0],sys.version_info[1])

#------------------------------------------------------
# PLATFORM DEFAULTS

#print "PLATFORM = ",platform.system()

default_install_prefix = '/usr/local'
default_install_bin = "$INSTALL_PREFIX/bin"
default_install_lib = "$INSTALL_PREFIX/lib"
default_install_models = "$INSTALL_LIB/ascend/models"
default_install_solvers = "$INSTALL_LIB/ascend/solvers"
default_install_assets = "$INSTALL_ASCDATA/glade/"
default_install_ascdata = "$INSTALL_SHARE/ascend"
default_install_include = "$INSTALL_PREFIX/include"
default_install_python = distutils.sysconfig.get_python_lib()
default_install_python_ascend = "$INSTALL_PYTHON/ascend"
default_tcl = '/usr'
default_tcl_libpath = "$TCL/lib"
default_tcl_cpppath = "$TCL/include"
default_tron_envvar="TRON_PATH"
default_conopt_envvar="CONOPT_PATH"
default_with_graphviz = True
default_tcl_lib = "tcl8.5"
default_tk_lib = "tk8.5"
default_tktable_lib = "Tktable2.9"
default_ida_prefix="$DEFAULT_PREFIX"
default_ipopt_libpath = "$IPOPT_PREFIX/lib"
default_ipopt_dll = "$IPOPT_LIBPATH/Ipopt38.dll"
default_ipopt_libs = ["$F2C_LIB","blas","lapack","pthread","ipopt"]
default_conopt_prefix="$DEFAULT_PREFIX"
default_conopt_libpath="$CONOPT_PREFIX"
default_conopt_cpppath="$CONOPT_PREFIX"
default_conopt_dlpath="$CONOPT_PREFIX"
default_tron_prefix="$DEFAULT_PREFIX"
default_tron_dlpath="$TRON_PREFIX/lib"
default_tron_lib="tron1"
default_prefix="/usr"
default_libpath="$DEFAULT_PREFIX/lib"
default_cpppath="$DEFAULT_PREFIX/include"
default_fortran="gfortran"
default_f2c_lib="gfortran"
default_swig="swig"

icon_extension = '.png'

if platform.system()=="Windows":
	default_prefix="c:\\MinGW"
	default_libpath="$DEFAULT_PREFIX\\lib"
	default_cpppath="$DEFAULT_PREFIX\\include"	

	# these correspond the the version of Tcl/Tk linked to in the NSIS scripts
	default_tcl_lib = "tcl85"
	default_tk_lib = "tk85"
	default_tktable_lib = "Tktable28"

	default_install_assets = "glade/"
	default_tcl = "c:\\Tcl"
	if os.environ.get('MSYSTEM'):
		default_tcl_libpath="$TCL\\bin"
	else:
		default_tcl_libpath="$TCL\\lib"

	# on Windows, we build ASCEND such that it finds it support files 
	# using paths relative to the location of the executable
	default_absolute_paths = False
	default_dist_rel_bin = '.'
	default_tk_rel_dist = 'tcltk'
	default_library_rel_dist = 'models'
	default_solvers_rel_dist = 'solvers'
	
	# where to look for IDA solver libraries, headers, etc.
	default_ida_prefix = "c:\\MinGW"
	
	# IPOPT
	default_ipopt_libpath = "$IPOPT_PREFIX/lib/win32/release"
	default_ipopt_libs = ["Ipopt"]

	# where to look for CONOPT when compiling
	default_conopt_prefix = "c:\\Program Files\\CONOPT"
	default_conopt_libpath="$CONOPT_PREFIX"
	default_conopt_cpppath="$CONOPT_PREFIX"
	default_conopt_dlpath="$CONOPT_PREFIX"
	default_conopt_lib="conopt3"

	# FIXME remove this
	default_tron_prefix="c:\\Program Files\\TRON"
	default_tron_dlpath="$TRON_PREFIX"
		
	need_libm = False
	python_exe = sys.executable
	default_with_scrollkeeper=False
	pathsep = ";"
	
	default_fortran="gfortran"
	default_f2c_lib="gfortran"
	
	default_swig=WhereIs("swig.exe")
	
	soname_minor = ""
	soname_major = ""
	# still problems with Graphviz on Windows, leave it off now by default.

	if not os.path.exists(default_conopt_prefix):
		default_conopt_prefix = None

elif platform.system()=="Darwin":

	default_install_prefix = ''
	default_install_bin = "$INSTALL_PREFIX/ASCEND.app/Contents"
	default_install_lib = "$INSTALL_BIN"
	#default_install_models = "$INSTALL_PREFIX/Library/Application Support/ASCEND/Models"
	default_install_models = "$INSTALL_BIN/Models"
	#default_install_solvers = "$INSTALL_PREFIX/Library/Application Support/ASCEND/Solvers"
	default_install_solvers = "$INSTALL_BIN/Solvers"
	default_install_ascdata = "$INSTALL_BIN/Resources"
	default_install_include = "$INSTALL_BIN/Headers"
	default_install_python = "$INSTALL_BIN/Python"
	default_install_python_ascend = default_install_python
	default_install_assets = "$INSTALL_ASCDATA/glade/"
	# FIXME still need to work out the Tcl/Tk side of things...

	# within the bundle, we'll use relative paths
	default_absolute_paths = False
	default_dist_rel_bin = '.'
	default_tk_rel_dist = 'tcltk'

	# we want these to be in /Library/ASCEND/Models and /Library/ASCEND/Solvers
	default_library_rel_dist = 'Models'
	default_solvers_rel_dist = 'Solvers'

	# where to look for CONOPT when compiling
	default_conopt_prefix = "/Library/CONOPT"

	default_conopt_lib="conopt3"

	# FIXME remove this
	default_tron_dlpath="$TRON_PREFIX"
	default_tron_lib="tron1"
		
	need_libm = False
	python_exe = sys.executable
	default_with_scrollkeeper=False
	pathsep = ";"
	
	if not os.path.exists(default_conopt_prefix):
		default_conopt_prefix = None
	
else: # LINUX

	icon_extension = '.svg'

	if os.path.exists("/etc/debian_version"):
		default_tcl_cpppath = "/usr/include/tcl8.4"
		default_tcl_lib = "tcl8.4"
		default_tk_lib = "tk8.4"
		default_tktable_lib = "Tktable2.8"

	if os.path.exists("/etc/SuSE-release"):
		default_tcl_cpppath = "/usr/include"
		default_tcl_lib = "tcl8.4"
		default_tk_lib = "tk8.4"
		default_tktable_lib = "Tktable2.9"

	if os.path.exists("/etc/lsb-release"):
		_f = file("/etc/lsb-release")
		_r = re.compile("([A-Z][^=]*)=(.*)")
		LSB = {}
		for l in _f:
			_m = _r.match(l.strip())
			LSB[_m.group(1)] = _m.group(2)
                print LSB
		if LSB.has_key('DISTRIB_ID') and LSB['DISTRIB_ID'] == "Ubuntu":
			if float(LSB['DISTRIB_RELEASE']) >= 9.04:
				default_tcl_lib = "tcl8.5"
				default_tk_lib = "tk8.5"
				default_tktable_lib = "Tktable2.9"
				default_tcl_cpppath = "/usr/include/tcl8.5"
			

	default_absolute_paths = True
	default_dist_rel_bin = '..'
	default_tk_rel_dist = 'share/ascend/tcltk'
	default_library_rel_dist = 'lib/ascend/models'
	default_solvers_rel_dist = 'lib/ascend/solvers'

	default_conopt_prefix = "$DEFAULT_PREFIX"
	default_conopt_libpath="$CONOPT_PREFIX/lib"
	default_conopt_cpppath="$CONOPT_PREFIX/include"
	default_conopt_dlpath= default_conopt_libpath + ":/usr/local/lib"
	default_conopt_lib="consub3"

	need_libm = True
	if not os.path.isdir(default_tcl):
		default_tcl = '/usr'
	python_exe = distutils.sysconfig.EXEC_PREFIX+"/bin/python"
	default_with_scrollkeeper=False
	pathsep = ":"
		
	#default_graphviz_libs=["graph","cdt","gvc"]
	#default_graphviz_libpath = default_libpath
	#if os.path.exists("/usr/lib/graphviz/libgraph.so"):
	#	# for Ubuntu 7.04
	#	default_graphviz_libpath="/usr/lib/graphviz"
	#	default_graphviz_rpath="$GRAPHVIZ_LIBPATH"

if not os.path.exists(default_tron_prefix):
	default_tron_prefix = None

if not os.path.exists(default_ida_prefix):
	default_ida_prefix = None

soname_clean = "${SHLIBPREFIX}ascend${SHLIBSUFFIX}"
soname_full = "%s%s" % (soname_clean,soname_major)

#------------------------------------------------------
# OPTIONS
#
# The following give the set of command-line parameters that can be passed to
# SCons from the commandline. Options will be 'remembered' by being cached
# in the file 'options.cache'; if you want to start with a clean slate, you
# should remove that file.

vars = Variables(['options.cache', 'config.py'])
	
vars.Add(
	'CC'
	,'C Compiler command'
	,None
)

vars.Add(
	'CXX'
	,'C++ Compiler command'
	,None
)

vars.Add(BoolVariable(
	'GCOV'
	, 'Whether to enable coverage testing in object code'
	, False
))

if platform.system()!="Windows":
	vars.Add(BoolVariable(
		'WITH_GCCVISIBILITY'
		,"Whether to use GCC Visibility features (only applicable if available)"
		,True
	))

vars.Add(BoolVariable(
	'WITH_SIGNALS'
	,"Whether to permit use of signals for flow control in the C-level code"
	,True
))

# You can turn off building of Tcl/Tk interface
vars.Add(BoolVariable(
	'WITH_TCLTK'
	,"Set to False if you don't want to build the original Tcl/Tk GUI."
	, True
))

# You can turn off the building of the Python interface
vars.Add(BoolVariable(
	'WITH_PYTHON'
	,"Set to False if you don't want to build Python wrappers."
	, True
))

# Which solvers will we allow?
vars.Add(ListVariable(
	'WITH_SOLVERS'
	,"List of the solvers you want to build. The default is the minimum that"	
		+" works. The option 'LSOD' is provided for backwards compatibility"
		+"; the value 'LSODE' is preferred."
	,["QRSLV","CMSLV","LSODE","IDA","CONOPT","LRSLV","TRON","IPOPT","DOPRI5"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSODE','LSOD','OPTSQP',"IDA","TRON","IPOPT","DOPRI5"
	 ]
))

# Where will the local copy of the help files be kept?
vars.Add(BoolVariable(
	'WITH_DOC'
	, "Should we try to build and install help files? If not, ASCEND will access online help files"
	, True
))

vars.Add(BoolVariable(
	'WITH_DOC_BUILD'
	, "If true, we'll attempt to build docs. Set false, we'll assume we already have then (eg from the tarball)"
	, "$WITH_DOC"
))

vars.Add(BoolVariable(
	'WITH_DOC_INSTALL'
	, "If true, SCons will install the documentation file(s). If false, assume rpm or dpkg is going to do it."
	, "$WITH_DOC"
))

vars.Add(
	'HELP_ROOT'
	, "Location of the main help file"
	, "$INSTALL_DOC/book.pdf"
)

# Will bintoken support be enabled?
vars.Add(BoolVariable(
	'WITH_BINTOKEN'
	,"Enable bintoken support? This means compiling models as C-code before"
		+" running them, to increase solving speed for large models."
	,False
))

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
vars.Add(
	'DEFAULT_ASCENDLIBRARY'
	,"Set the default value of the ASCENDLIBRARY -- the location where"
		+" ASCEND will look for models when running ASCEND"
	,"$INSTALL_MODELS"
)

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
vars.Add(
	'DEFAULT_ASCENDSOLVERS'
	,"Set the default value of ASCENDSOLVERS -- the location where"
		+" ASCEND will look for solver shared-library files"
	,"$INSTALL_SOLVERS"
)

# Where is SWIG?
vars.Add(
	'SWIG'
	,"SWIG location, probably only required for MinGW and MSVC users."
		+" Enter the location as a Windows-style path, for example"
		+" 'c:\\msys\\1.0\\home\\john\\swigwin-1.3.29\\swig.exe'."
	,default_swig
)

# Build the test suite?
vars.Add(BoolVariable(
	'WITH_CUNIT'
	,"You can disable CUnit tests with this option. This will basically stop"
		+" SCons from parsing the SConscript files relating to the 'test'"
        +" target, which just might make things marginally faster. Probably"
		+" you can just ignore this option though. SCons will sniff for Cunit"
		+" but build the tests only if you specify the 'test' target."
	,True
))

# Build with MMIO matrix export support?
vars.Add(BoolVariable(
	'WITH_MMIO'
	,"Include support for exporting matrices in Matrix Market format"
	,True
))

#----- default paths -----
vars.Add(PackageVariable(
	'DEFAULT_PREFIX'
	,"Where are most of the shared libraries located on your system?"
	,default_prefix
))

#------ cunit --------
# CUnit is a unit testing library that we use to test libascend.

# Where was CUNIT installed?
vars.Add(PackageVariable(
	'CUNIT_PREFIX'
	,"Where are your CUnit files?"
	,"$DEFAULT_PREFIX"
))

# Where are the CUnit includes?
vars.Add(PackageVariable(
	'CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,"$CUNIT_PREFIX/include"
))

# Where are the CUnit libraries?
vars.Add(PackageVariable(
	'CUNIT_LIBPATH'
	,"Where are your CUnit libraries?"
	,"$CUNIT_PREFIX/lib"
))

# ----- conopt-----

vars.Add(PackageVariable(
	"CONOPT_PREFIX"
	,"Prefix for your CONOPT install (CONOPT ./configure --prefix)"
	,default_conopt_prefix
))

vars.Add(
	"CONOPT_LIB"
	,"Library linked to for CONOPT. This is the name of the CONOPT .so or DLL. On Windows it seems to be called 'copopt3' but on linux it seems to be called 'consub3'."
	,default_conopt_lib
)

vars.Add(BoolVariable(
	"CONOPT_LINKED"
	,"Do you want to dynamically link to CONOPT (only possible if CONOPT is available at buildtime)"
	,False
))

vars.Add(
	'CONOPT_CPPPATH'
	,"Where is your conopt.h?"
	,default_conopt_cpppath
)

vars.Add(
	'CONOPT_LIBPATH'
	,"Where is your CONOPT library installed?"
	,default_conopt_libpath
)

vars.Add(
	'CONOPT_DLPATH'
	,"Default (fallback) search path that ASCEND should use when dlopening the CONOPT library at runtime? This is only used if the conopt environment variable doesn't exist and doesn't point to a location where the DLL/SO is found.  This is in platform-specific form (paths with ';' separator in Windows, ':' separator on Linux)."
	,default_conopt_dlpath
)

vars.Add(
	'CONOPT_ENVVAR'
	,"Name of the optional environment variable which will be used for the value of the searchpath for the CONOPT DLL/SO."
	,default_conopt_envvar
)

#------- IPOPT -------

vars.Add(PackageVariable(
	"IPOPT_PREFIX"
	,"Prefix for your IPOPT install (IPOPT ./configure --prefix)"
	,default_conopt_prefix
))

vars.Add(
	"IPOPT_LIBS"
	,"Library linked to for IPOPT"
	,default_ipopt_libs
)


vars.Add(
	"IPOPT_LIBPATH"
	,"Where is your IPOPT library installed"
	,default_ipopt_libpath
)

vars.Add(
	'IPOPT_CPPPATH'
	,"Where is your IPOPT coin/IpStdCInterface.h (do not include the 'coin' in the path)"
	,"$IPOPT_PREFIX/include"
)

vars.Add(
	'IPOPT_DLL'
	,"Exact path of IPOPT DLL to be included in the installer (Windows only)"
	,default_ipopt_dll
)



#------- TRON -------

vars.Add(
	'TRON_ENVVAR'
	,"What environment variable should be used at runtime to override the default search location for TRON DLL/SO?"
	,default_tron_envvar
)

vars.Add(
	"TRON_LIB"
	,"Library linked to for TRON"
	,"tron"
)

vars.Add(
	"TRON_PREFIX"
	,"Prefix for your TRON install"
	,default_tron_prefix
)

vars.Add(
	'TRON_DLPATH'
	,"What is the default search path that ASCEND should use when dlopening the TRON library at runtime?"
	,default_tron_dlpath
)

#-------- f2c ------

vars.Add(
	"F2C_LIB"
	,"F2C library (eg. g2c, gfortran, f2c)"
	,default_f2c_lib # the default is gfortran now
)

vars.Add(PackageVariable(
	"F2C_LIBPATH"
	,"Directory containing F2C library (i.e. g2c, gfortran, f2c, etc.), if not already accessible"
	,"off"
))

vars.Add(
	"FORTRAN"
	,"Fortran compiler (eg g77, gfortran)"
	,default_fortran
)

#------- tcl/tk --------

vars.Add(
	'TCL'
	,'Base of Tcl distribution'
	,default_tcl
)

# Where are the Tcl includes?
vars.Add(
	'TCL_CPPPATH'
	,"Where are your Tcl include files?"
	,default_tcl_cpppath
)

# Where are the Tcl libs?
vars.Add(
	'TCL_LIBPATH'
	,"Where are your Tcl libraries?"
	,default_tcl_libpath
)

# What is the name of the Tcl lib?
vars.Add(
	'TCL_LIB'
	,"Name of Tcl lib (eg 'tcl' or 'tcl83'), for full path to static library (if STATIC_TCLTK is set)"
	,default_tcl_lib
)

# Where are the Tk includes?
vars.Add(
	'TK_CPPPATH'
	,"Where are your Tk include files?"
	,'$TCL_CPPPATH'
)

# Where are the Tk libs?
vars.Add(
	'TK_LIBPATH'
	,"Where are your Tk libraries?"
	,'$TCL_LIBPATH'
)

# What is the name of the Tk lib?
vars.Add(
	'TK_LIB'
	,"Name of Tk lib (eg 'tk' or 'tk83'), or full path to static library"
	,default_tk_lib
)	

# Static linking to TkTable

vars.Add(BoolVariable(
	'STATIC_TCLTK'
	,'Set true for static linking for Tcl/Tk and TkTable. EXPERIMENTAL'
	,False
))

vars.Add(
	'TKTABLE_LIBPATH'
	,'Location of TkTable static library'
	,'$TCL_LIBPATH/Tktable2.8'
)

vars.Add(
	'TKTABLE_LIB'
	,'Stem name of TkTable (eg tktable2.8, no ".so" or "lib") shared library, or full path of static tktable (/usr/lib/...)'
	,default_tktable_lib
)

vars.Add(
	'TKTABLE_CPPPATH'
	,'Location of TkTable header file'
	,'$TCL_CPPPATH'
)

vars.Add(
	'X11'
	,'Base X11 directory. Only used when STATIC_TCLTK is turned on. EXPERIMENTAL'
	,'/usr/X11R6'
)

vars.Add(
	'X11_LIBPATH'
	,'Location of X11 lib. EXPERIMENTAL'
	,'$X11/lib'
)

vars.Add(
	'X11_CPPPATH'
	,'Location of X11 includes. EXPERIMENTAL'
	,'$X11/include'
)

vars.Add(
	'X11_LIB'
	,'Name of X11 lib. EXPERIMENTAL'
	,'X11'
)

#----- installed file locations (for 'scons install') -----

vars.Add(
	'INSTALL_PREFIX'
	,'Root location for installed files'
	,default_install_prefix
)

vars.Add(
	'INSTALL_BIN'
	,'Location to put binaries during installation'
	,default_install_bin
)

vars.Add(
	'INSTALL_LIB'
	,'Location to put libraries during installation'
	,default_install_lib
)

vars.Add(
	'INSTALL_SHARE'
	,'Common shared-file location on this system'
	,"$INSTALL_PREFIX/share"
)

vars.Add(
	'INSTALL_ASCDATA'
	,"Location of ASCEND shared data (TK, python, models etc)"
	,default_install_ascdata
)

vars.Add(
	'INSTALL_PYTHON'
	,'General location for Python extensions on this system'
	,default_install_python
)

vars.Add(
	'INSTALL_PYTHON_ASCEND'
	,'Location for installation of Python modules specific to ASCEND'
	,default_install_python_ascend
)

vars.Add(
	'INSTALL_TK'
	,'Location for Tcl/Tk files used by ASCEND Tk GUI'
	,"$INSTALL_ASCDATA/tcltk"
)

vars.Add(
	'INSTALL_MODELS'
	,"Location of ASCEND model files (.a4c,.a4l,.a4s)"
	,default_install_models
)

vars.Add(
	'INSTALL_SOLVERS'
	,"Location of ASCEND solvers"
	,default_install_solvers
)

vars.Add(
	'INSTALL_DOC'
	,"Location of ASCEND documentation files"
	,"$INSTALL_SHARE/doc/ascend-"+version
)

vars.Add(
	'INSTALL_INCLUDE'
	,'Location to put header files during installation'
	,default_install_include
)


vars.Add(
	'INSTALL_ROOT'
	,'For use by RPM only: location of %{buildroot} during rpmbuild'
	,""
)

vars.Add(
	'EXTLIB_SUFFIX'
	,"Filename suffix for ASCEND 'external libraries' (for use with IMPORT"
	,"_ascend$SHLIBSUFFIX"
)

vars.Add(
	'EXTLIB_PREFIX'
	,"Filename suffix for ASCEND 'external libraries' (for use with IMPORT"
	,"$SHLIBPREFIX"
)

#----------------------

vars.Add(
	'PYGTK_ASSETS'
	,'Default location for Glade assets (will be recorded in pygtk/config.py)'
	,default_install_assets
)

vars.Add(BoolVariable(
	'DEBUG'
	,"Compile source with debugger symbols, eg for use with 'gdb'"
	,False
))

vars.Add(BoolVariable(
	'MALLOC_DEBUG'
	,"Compile with debugging version of MALLOC. Required for full CUnit testing"
	,False
))

#------ dmalloc --------
vars.Add(PackageVariable(
	'DMALLOC_PREFIX'
	,"Where are your dmalloc files?"
	,default_prefix
))

vars.Add(PackageVariable(
	'DMALLOC_CPPPATH'
	,"Where are your dmalloc include files?"
	,default_cpppath
))

vars.Add(PackageVariable(
	'DMALLOC_LIBPATH'
	,"Where are your dmalloc libraries?"
	,default_libpath
))

vars.Add(BoolVariable(
	'WITH_DMALLOC'
	,"Link to the DMALLOC library (if available) for debugging of memory usage."
	,False
))

#------ Graphviz --------
#	vars.Add(PackageVariable(
#		'GRAPHVIZ_PREFIX'
#		,"Where are your GRAPHVIZ files?"
#		,default_prefix
#	))
#
#	vars.Add(PackageVariable(
#		'GRAPHVIZ_CPPPATH'
#		,"Where are your GRAPHVIZ include files? (don't need the final '/graphviz')"
#		,default_cpppath
#	))
#
#	vars.Add(PackageVariable(
#		'GRAPHVIZ_LIBPATH'
#		,"Where are your GRAPHVIZ libraries?"
#		,default_graphviz_libpath
#	))
#
#	vars.Add(
#		'GRAPHVIZ_LIBS'
#		,"What are your GRAPHVIZ libraries named?"
#		,default_graphviz_libs
#	)
#
#	vars.Add(PackageVariable(
#		'GRAPHVIZ_RPATH'
#		,"What is your GRAPHVIZ rpath for locating libraries at runtime? (only required for old Ubuntu)"
#		,default_graphviz_rpath
#	))

vars.Add(BoolVariable(
	'WITH_GRAPHVIZ'
	,"Link to the GRAPHVIZ library (if available, for generating incidence graphs)"
	,default_with_graphviz
))


#------ ufsparse --------
vars.Add(PackageVariable(
	'UFSPARSE_PREFIX'
	,"Where are your UFSPARSE files?"
	,default_prefix
))

vars.Add(PackageVariable(
	'UFSPARSE_CPPPATH'
	,"Where are your UFSPARSE include files?"
	,default_cpppath
))

vars.Add(PackageVariable(
	'UFSPARSE_LIBPATH'
	,"Where are your UFSPARSE libraries?"
	,default_libpath
))

vars.Add(BoolVariable(
	'WITH_UFSPARSE'
	,"Link to the UFSPARSE library (if available, for additional sparse matrix routines)"
	,True
))

#-----------------------

vars.Add(BoolVariable(
	'UPDATE_NO_YACC_LEX'
	,"Update the *_no_yacc* and *_no_lex* files in the source tree? (these files are created so that ASCEND can be compiled in the absence of those tools)"
	,False
))

vars.Add(
	'DISTTAR_NAME'
	,"Stem name of the tarball created by 'scons dist'. So for 'ascend-aaa.tar.bz2', set this to 'ascend-aaa'."
	,"ascend-"+version
)

vars.Add(
	'RELEASE'
	,"Release number for use in RPM spec file. This should always start with a zero for releases made by the ASCEND group, in order that third parties can make 'patch' releases of higher version numbers."
	,"0"
)

vars.Add(BoolVariable(
	'ABSOLUTE_PATHS'
	,"Whether to use absolute or relative paths in the installed Tcl/Tk interface. If you want to build an RPM, set this to false."
	,default_absolute_paths
))

vars.Add(
	'WIN_INSTALLER_NAME'
	,"Name of the installer .exe to create under Windows (minus the '.exe')"
	,"ascend-"+version+"-py"+pyversion+".exe"
)

vars.Add(BoolVariable(
	'WITH_XTERM_COLORS'
	,"Set to 0 if you don't want xterm colour codes in the console output"
	,True
))

vars.Add(BoolVariable(
	'WITH_EXTFNS'
	,"Set to 0 if you don't want to attempt to build the external modules bundled with ASCEND"
	,True
))

vars.Add(BoolVariable(
	'WITH_SCROLLKEEPER'
	,"Set to to 1 if you want to install an OMF file that can be read by scrollkeeper (eg Yelp on GNOME)"
	,default_with_scrollkeeper
))

vars.Add(BoolVariable(
	'WITH_MSVCR71'
	,"Attempt to link against MSVCR71.DLL, to enable passing of FILE* objects to/from python"
	,False
))


# TODO: OTHER OPTIONS?
# TODO: flags for optimisation
# TODO: turning on/off bintoken functionality
# TODO: Where will the 'Makefile.bt' file be installed?

# Import the outside environment

def c_escape(str):
        return re.sub("\\\\","/",str)

envadditional={}

tools = [
	'lex', 'yacc', 'fortran', 'swig', 'substinfile'
	,'disttar', 'tar', 'graphviz','sundials'
]
if platform.system()=="Windows":
	tools += ['nsis']
	
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

vars.Update(env)

for l in ['SUNDIALS','IPOPT']:
	var = "%s_LIBS" % l
	if env.get(var) and not isinstance(env[var],types.ListType):
		env[var] = env[var].split(",")

if 'LSOD' in env['WITH_SOLVERS']:
	if 'LSODE' not in env['WITH_SOLVERS']:
		env['WITH_SOLVERS'].append('LSODE')
	env['WITH_SOLVERS'].remove('LSOD')

vars.Save('options.cache',env)

Help(vars.GenerateHelpText(env))

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

with_graphviz = env.get('WITH_GRAPHVIZ')
without_graphiviz_reason = "disabled by options/config.py"

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

with_lsode = 'LSODE' in env['WITH_SOLVERS']
without_lsode_reason = "Not selected (see config option WITH_SOLVERS)"

with_ida = 'IDA' in env['WITH_SOLVERS']
without_ida_reason = "Not selected (see config option WITH_SOLVERS)"

with_dopri5 = 'DOPRI5' in env['WITH_SOLVERS']
without_dopri5_reason = "Not selected (see config option WITH_SOLVERS)"

with_conopt = 'CONOPT' in env['WITH_SOLVERS']
without_conopt_reason = "Not selected (see config option WITH_SOLVERS)"

with_ipopt = 'IPOPT' in env['WITH_SOLVERS']
without_ipopt_reason = "Not selected (see config option WITH_SOLVERS)"


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
print "INSTALL_PYTHON =",env['INSTALL_PYTHON']
print "INSTALL_PYTHON_ASCEND =",env['INSTALL_PYTHON_ASCEND']

print "DEFAULT_ASCENDLIBRARY =",env['DEFAULT_ASCENDLIBRARY']
print "DEFAULT_ASCENDSOLVERS =",env['DEFAULT_ASCENDSOLVERS']


#------------------------------------------------------
# SPECIAL CONFIGURATION TESTS

need_fortran = False
need_fortran_reasons = []

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
	if not context.env.get('CXX'):
		context.Result("not found")
		return False
	is_ok = context.TryCompile(cxx_test_text,".cpp")
	context.Result(is_ok)
	return is_ok

#----------------

f77_test_text = """
C     Hello World in Fortran 77

      PROGRAM HELLO
      PRINT*, 'Hello World!'
      END	
""";

def CheckF77(context):
	context.Message("Checking Fortran 77 compiler ('%s')..." % context.env.get('FORTRAN'))
	if not context.env.get('FORTRAN'):
		context.Result('not found')
		return False
	is_ok = context.TryCompile(f77_test_text,".f")
	context.Result(is_ok)
	return is_ok
	
#----------------
# SWIG

import os,re

def get_swig_version(env):
	cmd = [env['SWIG'],'-version']
	p = subprocess.Popen(cmd,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output, err = p.communicate()
	
	restr = r"\s*SWIG\s+Version\s+(?P<maj>[0-9]+)\.(?P<min>[0-9]+)\.(?P<pat>[0-9]+)\b"
	expr = re.compile(restr,re.MULTILINE|re.IGNORECASE);
	m = expr.match(output);
	if not m:
		raise RuntimeError("Failed match on output '%s'"  % output)
	maj = int(m.group('maj'))
	min = int(m.group('min'))
	pat = int(m.group('pat'))

	return (maj,min,pat)
	

def CheckSwigVersion(context):
	
	try:
		context.Message("Checking version of SWIG... ")
		maj,min,pat = get_swig_version(context.env)
	except Exception,e:
		context.Result("Failed to detect version, or failed to run SWIG (%s)" % str(e))
		return False;
	
	context.env['SWIGVERSION']=tuple([maj,min,pat])
	
	msg = "too old"
	res = False
	if maj == 1 and (
			min > 3
			or (min == 3 and pat >= 24)
		):
		msg = "ok"
		res = True
	elif maj == 2:
		msg = "ok"
		res = True

	context.Result("%s, %d.%d.%d" % (msg, maj,min,pat))
	return res;

#----------------
# Scrollkeeper (Linux documentation system)

def get_scrollkeeper_omfdir(env):
	cmd = ['scrollkeeper-config','--omfdir']
	p = subprocess.Popen(cmd,stdout=subprocess.PIPE)
	output = p.communicate()
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

def CheckExtLib(context,libname,text,ext='.c',varprefix=None,static=False,testname=None):
	"""This method will check for variables LIBNAME_LIBPATH
	and LIBNAME_CPPPATH and try to compile and link the 
	file with the provided text, linking with the 
	library libname."""

	if testname is None:
		testname = libname

	if static:
		context.Message( 'Checking for static '+testname+'... ' )
	else:
		context.Message( 'Checking for '+testname+'... ' )
		
	if varprefix==None:
		varprefix = libname.upper()
	
	#print "LIBS is currently:",context.env.get('LIBS')
	keep = KeepContext(context,varprefix,static)

	if not context.env.has_key(varprefix+'_LIB') and not context.env.has_key(varprefix+'_LIBS'):
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
	c = (char *)malloc(100*sizeof(char));
	free(c);
	return 0;
}
"""

def CheckDMalloc(context):
	return CheckExtLib(context,'dmalloc',dmalloc_test_text)

#----------------
# graphviz test

graphviz_test_text = """
#ifdef __WIN32__
# include <gvc.h>
#else
# include <graphviz/gvc.h>
#endif
int main(void){
	Agraph_t *g;
	GVC_t *gvc;
	gvc = gvContext();
	g = agopen("g", AGDIGRAPH);
	return 0;
}
"""

def CheckGraphViz(context):
	return CheckExtLib(context,'graphviz',graphviz_test_text,ext=".c")

graphviz_boolean_test = """
#ifdef __WIN32__
# include <types.h>
#else
# include <graphviz/types.h>
#endif
#ifndef GV_TYPES_H
# error WHERE IS GV_TYPES_H?
#endif
int main(void){
	boolean x;
	x = TRUE;
	return 0;
}
"""

def CheckGraphVizBoolean(context):
	return CheckExtLib(context,'graphviz',graphviz_boolean_test,ext=".c" \
		,testname="graphviz 'boolean' definition"
	)

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
	if libsave is None:
		del(context.env['LIBS'])
	else:
		context.env['LIBS']=libsave
	return is_ok

#----------------
# malloc.h test

malloc_test_text = """
#include <stdlib.h>
int main(){
	double *x;
	x = malloc(sizeof(double)*5);
	x[4] = 3.3;
	free(x);
}
"""

def CheckMalloc(context):
	context.Message("Checking for malloc...")
	is_ok = context.TryLink(malloc_test_text,".c")
	context.Result(is_ok)
	return is_ok

#----------------
# dlopen test

dlopen_test_text = """
#ifdef __WIN32__
# include <windows.h>
#else
# include <dlfcn.h>
#endif
int main(){
#ifdef __WIN32__
	HINSTANCE d;
	LoadLibrary("imaginary_and_nonexistent.dll");
#else
	void *d;
  	d = dlopen("imaginary_and_nonexistent.so", 1);
#endif
	return 0;
}
"""

def CheckDLOpen(context):
	context.Message("Checking for ability to load shared libraries at runtime...")
	libsave=context.env.get('LIBS');
	if platform.system()!="Windows":
		context.env.Append(LIBS=['dl'])
	is_ok = context.TryLink(dlopen_test_text,".c")
	context.Result(is_ok)
	context.env['LIBS'] = libsave
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

	try:
		python_libs = [python_lib % (sys.version_info[0],sys.version_info[1])]
		python_cpppath = [distutils.sysconfig.get_python_inc()]
		cfig = distutils.sysconfig.get_config_vars()	
	except:
		context.Result("not found")
		return 0		
	
	lastLIBS = context.env.get('LIBS')
	lastLIBPATH = context.env.get('LIBPATH')
	lastCPPPATH = context.env.get('CPPPATH')
	lastLINKFLAGS = context.env.get('LINKFLAGS')

	python_libpath = []
	python_linkflags = []
	if platform.system()=="Windows":
		python_libpath += [os.path.join(sys.prefix,"libs")]
	elif platform.system()=="Darwin":
		python_libpath += [cfig['LIBPL']]
                python_linkflags += cfig['LIBS'].split(' ')
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
sundials_version_minor_max = 4

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

#include <coin/IpStdCInterface.h>
int main(){
	Number n;
	IpoptProblem nlp = 0;
	n = 1;
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

# TCL and TK required version 8.1 through 8.5:
tcltk_minor_newest_acceptable = 5
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
	libsave=context.env.get('LIBS')
	context.env.AppendUnique(LIBS=['m'])
	(is_ok,output) = context.TryRun(sigreset_test_text,'.c')

	if libsave is None:
		del(context.env['LIBS'])
	else:
		context.env['LIBS']=libsave

	if not is_ok:
		context.Result("ERROR")
		return False
	if int(output)==1:
		context.Result("required");
		context.env['ASC_RESETNEEDED'] = True
	else:
		context.Result("not required");
		context.env['ASC_RESETNEEDED'] = False
	return True

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
# 'lmodern' package for LaTeX available?

lmodern_test_text = r"""
\documentclass{article}
\usepackage{lmodern}
\title{Cartesian closed categories and the price of eggs}
\author{Jane Doe}
\date{September 1994}
\begin{document}
   \maketitle
   Hello world!
\end{document}
""";

def CheckLModern(context):
	context.Message("Checking for lmodern...")
	b = context.env.get("DVI")
	if not b:
		context.Result(False)
		return False
	ff = context.env.get('LATEXFLAGS')
	context.env.Append(LATEXFLAGS=['-interaction=nonstopmode','-halt-on-error'])
	is_ok = context.TryBuild(builder=b,text=lmodern_test_text,extension=".latex")
	print "is_ok=",is_ok
	if ff is not None:
		context.env['LATEXFLAGS'] = ff
	else:
		del context.env['LATEXFLAGS']
	context.Result(is_ok)
	return is_ok

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
		, 'CheckF77' : CheckF77
		, 'CheckMath' : CheckMath
		, 'CheckMalloc' : CheckMalloc
		, 'CheckDLOpen' : CheckDLOpen
		, 'CheckSwigVersion' : CheckSwigVersion
		, 'CheckPythonLib' : CheckPythonLib
		, 'CheckCUnit' : CheckCUnit
		, 'CheckDMalloc' : CheckDMalloc
		, 'CheckLyx' : CheckLyx
		, 'CheckLatex2HTML' : CheckLatex2HTML
		, 'CheckLModern' : CheckLModern
		, 'CheckGraphViz' : CheckGraphViz
		, 'CheckGraphVizBoolean' : CheckGraphVizBoolean
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
		if v[0] == '1':
			return 1;
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

if conf.CheckCC() is False:
	print "Failed to build simple test file with your C compiler."
	print "Check your compiler is installed and running correctly."
	Exit(1)

if conf.CheckCXX() is False:
	print "Failed to build simple test file with your C++ compiler."
	print "Check your compiler is installed and running correctly."
	print "You can set your C++ compiler using the CXX scons option."
	Exit(1)

# stdio -- just to check that compiler is behaving

if conf.CheckHeader('stdio.h') is False:
	print "CPPPATH =",env.get('CPPPATH')
	print "Did not find 'stdio.h'! Check your compiler configuration."
	print ""
	print "You environment is printed here:"
	for k,v in os.environ.iteritems():
		print "%-30s%s" % ("%s :" % k, v)
	Exit(1)

# sizes of vars used in libascend eg in gl_list etc.

_sizes = {
	"VOID_P" : "void *"
	,"INT" : "int"
	,"LONG" : "long"
	,"LONG_LONG" : "long long"
}

for _var,_type in _sizes.iteritems():
	_size = conf.CheckTypeSize(_type)
	if not _size:
		print "Couldn't determine 'sizeof(%s)'" % _type
		Exit(1)
	conf.env["SIZEOF_%s" % _var] = str(_size)

# check for some string functions

if conf.CheckFunc('snprintf') is False:
	print "Didn't find snprintf";
	Exit(1)

if conf.CheckFunc('strdup'):
	conf.env['HAVE_STRDUP'] = True

# attempt to support MSVCRT 7.1 on Windows

if platform.system()=="Windows" and env.get('WITH_MSVCR71'):
	conf.env.Append(LIBS='msvcr71')
	
# Math library

conf.env['HAVE_IEEE']=True

if need_libm and (conf.CheckMath() is False):
	conf.env['HAVE_IEEE']=False
	print 'Did not find math library, exiting!'
	Exit(1)

# Malloc

if conf.CheckMalloc() is False:
	conf.env['HAVE_MALLOC']=False
	print "Did not find functioning 'malloc', exiting!"
	Exit(1)

# dlopen/LoadLibrary

# CURRENTLY BREAKS LATER TEST (libsave?)
#if conf.CheckDLOpen() is False:
#	print "Did not find functioning dlopen/LoadLibrary, exiting!"
#	Exit(1)

# Where is 'isnan'?

if conf.CheckFunc('isnan') is False and conf.CheckFunc('_isnan') is False:
	print "Didn't find isnan"
#	Exit(1)

# GCC visibility

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env.get('WITH_GCCVISIBILITY') and conf.CheckGccVisibility():
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

if conf.CheckSigReset() is False:
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

if with_python and conf.CheckSwigVersion() is False:
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

# GRAPHVIZ

if with_graphviz:
	if not conf.CheckGraphViz():
		without_graphviz_reason = 'graphviz not found'
		with_graphviz = False
		env['WITH_GRAPHVIZ'] = False
	env['HAVE_GRAPHVIZ_BOOLEAN'] = conf.CheckGraphVizBoolean()		

# UFSPARSE

if with_ufsparse:
	if not conf.CheckUFSparse():
		without_ufsparse_reason = 'ufsparse not found'
		with_ufsparse = False
		env['WITH_UFSPARSE'] = False

# IDA

if with_ida:
	if not conf.CheckSUNDIALS():
		with_ida = False
		without_ida_reason = "SUNDIALS not found, or bad version"
	elif not conf.CheckIDA():
		with_ida = False
		without_ida_reason = "Unable to compile/link against SUNDIALS/IDA"

# CONOPT

if not with_conopt:
	without_conopt_reason = "Not selected (see config option WITH_SOLVERS)"
elif conf.CheckCONOPT() is False:
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
	need_fortran_reasons.append("LSODE")
	need_blas=True

if with_ipopt:
	need_blas=True

if need_blas:
	if conf.CheckLib('blas'):
		with_local_blas = False
		without_local_blas_reason = "Found BLAS installed on system"
	else:
		with_local_blas = True
		need_fortran = True
		need_fortran_reasons.append("BLAS")
else:
	with_local_blas= False;
	without_local_blas_reason = "BLAS not required"

# FORTRAN

if need_fortran:
	print "NEED FORTRAN"
	# Removed stuff for SCons 0.97 (Ubuntu 8.04) from here -- JP
	detect_fortran = conf.env.Detect(['gfortran','g77'])
	if detect_fortran:
		conf.env.Tool(detect_fortran)
		if detect_fortran == 'g77':
			print "Setting F2C_LIB to 'g2c' because G77 compiler in use"
			conf.env['F2C_LIB'] = 'g2c'
		#print "DETECT_FORTRAN =",detect_fortran
		#print "NOW, FORTAN =",env['FORTRAN']
	else:
		print "FAILED G77/FORTRAN DETECTION"
		with_lsode=False;
		without_lsode_reason="FORTRAN required but not detected"
else:
	print "FORTRAN WAS NOT FOUND TO BE REQUIRED"

if need_fortran and conf.CheckF77() is False:
	print "Failed to build simple test file with your Fortran compiler."
	print "Check your compiler is installed and running correctly."
	print "You can set your Fortran compiler using the FORTRAN scons option."
	print "The fortran compiler is REQUIRED to build:",", ".join(need_fortran_reasons)
	print "Perhaps try examining the value of your WITH_SOLVERS option (remove LSODE, etc)."
	Exit(1)

#else:
#	print "FORTRAN not required"

# F2C

if need_fortran:
	if platform.system()=="Windows":
		pass
		#conf.env.Append(LIBPATH='c:\mingw\lib')

# scrollkeeper

if with_scrollkeeper:
	if conf.CheckScrollkeeperConfig() is False:
		with_scrollkeeper=False
		without_scrollkeeper_reason="unable to detect scrollkeeper-config"

# lyx

if with_doc_build:
	if not conf.CheckLyx():
		with_doc_build = False
		without_doc_build_reason="unable to locate LyX"

	with_latex2html = conf.CheckLatex2HTML()

	if conf.CheckLModern() is False:
		with_doc_build = False
		without_doc_build_reason="'lmodern' is not available"

# TODO: -D_HPUX_SOURCE is needed

# TODO: detect if dynamic libraries are possible or not

if platform.system()=="Windows" and env.has_key('MSVS'):
	_found_windows_h = conf.CheckHeader('Windows.h')

	if not _found_windows_h:
		print "Could not locate 'Windows.h' in CPPPATH. Check your configuration."
		Exit(1)

	if with_python and conf.CheckHeader(['basetsd.h','BaseTsd.h']) is False:
		with_python = 0;
		without_python_reason = "Header file 'basetsd.h' not found. Install the MS Platform SDK."

conf.Finish()

#print "-=-=-=-=-=-=-=-=- LIBS =",env.get('LIBS')

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
	, '@INSTALL_PYTHON@':env['INSTALL_PYTHON']
	, '@INSTALL_PYTHON_ASCEND@':env['INSTALL_PYTHON_ASCEND']
	, '@PYGTK_ASSETS@':env['PYGTK_ASSETS']
	, '@VERSION@':version
	, '@RELEASE@':release
	, '@DISTTAR_NAME@':env['DISTTAR_NAME']
	, '@WEBHELPROOT@':'http://ascendwiki.cheme.cmu.edu/Category:Documentation'
	, '@SHLIBSUFFIX@':env['SHLIBSUFFIX']
	, '@SHLIBPREFIX@':env['SHLIBPREFIX']
	, '@EXTLIB_SUFFIX@':env['EXTLIB_SUFFIX']
	, '@EXTLIB_PREFIX@':env['EXTLIB_PREFIX']
	, '@ASC_ENV_TK_DEFAULT@' : '$$ASCENDDIST/tcltk'
	, '@PYTHON@' : python_exe
	, '@PYVERSION@' : pyversion
	, '@SOURCE_ROOT@':c_escape(os.path.abspath(str(env.Dir("#"))))
	, '@WITH_GRAPHVIZ@': str(int(env.get('WITH_GRAPHVIZ')))
#define ASC_ABSOLUTE_PATHS @ASC_ABSOLUTE_PATHS@
#if ASC_ABSOLUTE_PATHS
# define ASCENDDIST_DEFAULT "@ASCENDDIST_DEFAULT@"
# define ASCENDTK_DEFAULT "@ASCENDTK_DEFAULT@"
# define ASCENDLIBRARY_DEFAULT "@ASCENDLIBRARY_DEFAULT@"
# define ASCENDSOLVERS_DEFAULT "@ASCENDSOLVERS_DEFAULT@"
#else
# define ASC_DIST_REL_BIN "@ASC_DIST_REL_BIN@"
# define ASC_TK_REL_DIST "@ASC_TK_REL_DIST@"
# define ASC_LIBRARY_REL_DIST "@ASC_LIBRARY_REL_DIST@"
# define ASC_SOLVERS_REL_DIST "@ASC_SOLVERS_REL_DIST@"
#endif
	, '@ASC_ABSOLUTE_PATHS@': str(int(env.get('ABSOLUTE_PATHS')))
	, '@ASCENDDIST_DEFAULT@': c_escape(env['INSTALL_PREFIX'])
	, '@ASCENDTK_DEFAULT@': c_escape(os.path.abspath(env.subst(env['INSTALL_TK'])))
	, '@ASCENDLIBRARY_DEFAULT@': c_escape(os.path.abspath(env.subst(env['DEFAULT_ASCENDLIBRARY'])))
	, '@ASCENDSOLVERS_DEFAULT@': c_escape(os.path.abspath(env.subst(env['DEFAULT_ASCENDSOLVERS'])))
	, '@ASC_DIST_REL_BIN@' : default_dist_rel_bin
	, '@ASC_TK_REL_DIST@' : default_tk_rel_dist
	, '@ASC_LIBRARY_REL_DIST@' : default_library_rel_dist
	, '@ASC_SOLVERS_REL_DIST@' : default_solvers_rel_dist
	, '@SIZEOF_VOID_P@' : env['SIZEOF_VOID_P']
	, '@SIZEOF_INT@' : env['SIZEOF_INT']
	, '@SIZEOF_LONG@' : env['SIZEOF_LONG']
	, '@SIZEOF_LONG_LONG@' : env['SIZEOF_LONG_LONG']
}

if env.get('WITH_DOC'):
	print "WITH_DOC:",env['WITH_DOC']
	subst_dict['@HELP_ROOT@']=env['HELP_ROOT']

# bool options...
for k,v in {
		'ASC_WITH_DMALLOC':with_dmalloc
		,'ASC_WITH_UFSPARSE':with_ufsparse
		,'ASC_WITH_MMIO':with_mmio
		,'ASC_SIGNAL_TRAPS':with_signals
		,'ASC_RESETNEEDED':env.get('ASC_RESETNEEDED')
		,'HAVE_C99FPE':env.get('HAVE_C99FPE')
		,'HAVE_IEEE':env.get('HAVE_IEEE')
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

def InstallPermAs(env, dest, filen, perm): 	 
	obj = env.InstallAs(dest, filen) 	 
	for i in obj: 	 
		env.AddPostAction(i, env.Chmod(str(i), perm))
	return dest
  	 
SConsEnvironment.InstallPerm = InstallPerm 	 
  	 
# define wrappers 	 
SConsEnvironment.InstallProgram = lambda env, dest, files: InstallPerm(env, dest, files, 0755) 	 
SConsEnvironment.InstallHeader = lambda env, dest, files: InstallPerm(env, dest, files, 0644)
SConsEnvironment.InstallShared = lambda env, dest, files: InstallPerm(env, dest, files, 0644)
SConsEnvironment.InstallSharedAs = lambda env, dest, files: InstallPermAs(env, dest, files, 0644)
SConsEnvironment.InstallLibraryAs = lambda env, dest, files: InstallPermAs(env, dest, files, 0644)

#------------------------------------------------------
# BUILD...

# so that #include <ascend/modulename/headername.h> works across all modules...
env.AppendUnique(CPPPATH=['#'])

if env['DEBUG']:
	env.Append(
		CCFLAGS=['-g']
		,LINKFLAGS=['-g']
	)

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

if with_dopri5:
	env.Append(WITH_DOPRI5=1)

if with_graphviz and env.get('GRAPHVIZ_RPATH'):
	env.Append(RPATH=env['GRAPHVIZ_RPATH'])

#-------------
# TCL/TK GUI

if with_tcltk:
	env.SConscript(['tcltk/SConscript'],'env')
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

libascend_env = env.Clone()

dirs = ['general','utilities','compiler','system','solver','integrator','packages','linear']

srcs = []
for d in dirs:
	heresrcs = libascend_env.SConscript('ascend/'+d+'/SConscript','libascend_env')
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

if not with_dopri5:
	print "Skipping... DOPRI5 won't be built:", without_dopri5_reason

if with_mmio:
	srcs += env.SConscript(['mmio/SConscript'],'env')
else:
	print "Skipping... MMIO export won't be built:", without_mmio_reason
#-------------
# LIBASCEND -- all 'core' functionality

# FIXME want to move these bits to ascend/SConscript

libascend_env.Append(
	CPPPATH=['#']
	,LIBS=['m']
)

if platform.system()=="Linux":
	libascend_env.Append(LIBS=['dl'])

if with_dmalloc:
	libascend_env.Append(LIBS=['dmalloc'])

if with_ufsparse:
	libascend_env.Append(LIBS=['cxsparse'])

if platform.system()=="Linux":
	libascend_env.Append(LINKFLAGS=['-Wl,-soname,%s' % soname_full])

libascend = libascend_env.SharedLibrary('ascend',srcs)

# create local symlink for the soname stuff.
#print "SONAME =",env.subst(soname_full)

env['libascend'] = libascend
libtargets = [libascend]

if platform.system()=="Linux":
	if soname_major:
		libascend_env.Command(soname_full,libascend,Move("$TARGET","$SOURCE"))
		#print "MAKING LINK, SONAME_MAJOR =",soname_major
		liblink = libascend_env.Command(soname_full, libascend, "ln -s $SOURCE $TARGET")
		libtargets.append(liblink)

# for use in declaring dependent shared libraries in SConscript files (eg solvers/*/SConscript)

env.Alias('libascend',libtargets)

#-------------
# UNIT TESTS (C CODE)

test_env = env.Clone()
test_env.Append(
	CPPPATH="#"
)

if with_cunit:
	testdirs = ['general','solver','utilities','linear','compiler','packages']
	testsrcs = []
	for testdir in testdirs:
		path = 'ascend/'+testdir+'/test/'
		test_env.SConscript([path+'SConscript'],'test_env')
		testsrcs += [i.path for i in test_env['TESTSRCS_'+testdir.upper()]]
	test_env['TESTDIRS'] = testdirs

	#print "TESTSRCS =",testsrcs
		
	test_env.SConscript(['test/SConscript'],'test_env')

	env.Alias('test',[env.Dir('test')])
	
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

#-------------
# FPROPS python bindings

env.Alias('pyfprops',env['pyfprops'])

#------------------------------------------------------
# CREATE ASCEND-CONFIG scriptlet

ascendconfig = env.SubstInFile('ascend-config.in')

#------------------------------------------------------
# INSTALLATION

if env.get('CAN_INSTALL'):

	dirs = ['INSTALL_BIN','INSTALL_ASCDATA','INSTALL_LIB', 'INSTALL_INCLUDE','INSTALL_DOC','INSTALL_PYTHON']
	install_dirs = [Dir(env.subst("$INSTALL_ROOT$"+d)) for d in dirs]
	install_dirs += modeldirs + [Dir(env.subst("$INSTALL_ROOT$INSTALL_SOLVERS"))]

	#env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_LIB")),libascend)

	libname = "${INSTALL_LIB}/%s%s" % (soname_full,soname_minor)
	install_lib = env.InstallLibraryAs("${INSTALL_ROOT}"+libname, [libascend])
	if env['ABSOLUTE_PATHS']:
		link_target = libname
	else:
		link_target = "%s%s" % (soname_full,soname_minor)

	link1 = "${INSTALL_LIB}/%s" % soname_clean
	install_link1 = None
	if env.subst(link1) != env.subst(libname):
		#                           v--link to create       v--file to link to   command          
		install_link1 = env.Command("${INSTALL_ROOT}"+link1,install_lib
		#    v-- command to do it (note the trick about 
			,"ln -f -s %s $TARGET" % link_target
		)

	link2 = "${INSTALL_LIB}/%s" % soname_full
	install_link2 = None
	if soname_minor:
		install_link2 = env.Command("${INSTALL_ROOT}"+link2,install_lib
			,"ln -f -s %s $TARGET" % link_target
		)

	env.InstallProgram(Dir(env.subst("$INSTALL_ROOT$INSTALL_BIN")),ascendconfig)

	# MAC OS X INSTALL STUFF
	# in this case, we're installing to INSTALL_PREFIX, assumed to be a folder
	# created using Disk Utility as a new DMG which will be distributed.
	if platform.system()=="Darwin":
		# extra stuff for inside the .app
		env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_BIN")),"mac/Info.plist")
		env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_BIN/Resources/")),"mac/ascend.icns")

		# related files the .dmg folder
		env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_PREFIX")),"README-osx.txt")
		env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_PREFIX")),"LICENSE.txt")
		env.InstallShared(Dir(env.subst("$INSTALL_ROOT$INSTALL_PREFIX")),"CHANGELOG.txt")
		env.Command("$INSTALL_ROOT$INSTALL_PREFIX/Applications Folder","/Applications","ln -f -s $SOURCE $TARGET")
		install_dirs += [Dir(env.subst("$INSTALL_ROOT$INSTALL_PREFIX"))]

		# GTK libraries and related files
		gtkfiles = []
		gtksource = "dist/PyGTK.bundle/"
		def visit(gtkfiles,dirname,fnames):
			gtkfiles += Glob("%s/*" % dirname)
		os.path.walk(gtksource,visit,gtkfiles)
		
		#print "GTKFILES ="
#
		for f in gtkfiles:
			r = os.path.commonprefix([gtksource,f.path])
			dirname,filename = os.path.split(f.path[len(r):])
			dest = os.path.join(env.subst("$INSTALL_ROOT$INSTALL_BIN/PyGTK.bundle"),dirname)
		#	print "%s --> %s" %(f,dest)
			env.Install(Dir(dest),f)

	# ALIAS FOR ALL INSTALLATION
	env.Alias('install',install_dirs)

#------------------------------------------------------
# WINDOWS INSTALLER

if not env.get('NSIS'):
	with_installer = False
	without_installer_reason = "NSIS not found"

if with_installer:
	env.Append(NSISDEFINES={
		'OUTFILE':"#dist/"+env['WIN_INSTALLER_NAME']
		,"VERSION":version
		,'PYVERSION':pyversion
		,'IPOPTDLL':os.path.normpath(env['IPOPT_DLL'])
	})
	installer = env.Installer('nsis/installer.nsi')
	env.Depends(installer,["pygtk","tcltk","ascend.dll","models","solvers","ascend-config",'pygtk/ascend'])
	env.Depends(installer,[os.path.normpath(env['IPOPT_DLL'])])
	env.Alias('installer',installer)
else:
	print "Skipping... Windows installer isn't being built:",without_installer_reason

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
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.lib','.cc','.cache',
		'.pyc','.cvsignore','.dblite','.log','.pl','.out','.exe','.aux','.idx',
		'.toc','.lof','.lot','.mm','.warnings','.tm2','.swp',',tmp','.gz',
		'.bz2','.7z','.deb','.dsc','.changes','.bak','.tex','.tmp']
	, DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist','debian']
	, DISTTAR_EXCLUDERES=[r"_wrap\.cc?$", r"~$", r"pygtk/ascpy\.py","pygtk/testipopt$"
		,r"/lib.*\.so\.[.0-9]+$", r"tcltk/asc4dev$", r"tcltk/interface/typelex\.c$"
		,r"ascend/compiler/ascParse\.[ch]$", r"ascend/solver/conoptconfig\.h$"
		,r"ascend/utilities/config\.h$", r"pygtk/config\.h$", r"pygtk/config\.py$"
		,r"pygtk/ascdev$", r"pygtk/testconopt$", r"ascend/compiler/scanner\.c$"
		,r"tcltk/interface/ascend4$",r"pygtk/testslvreq$",r"test/test$"
		,r"models/johnpye/datareader/.*\.tm2\.Z$"
		,r"models/johnpye/fprops/[a-z][a-z0-9]+(.*\.exe)?$" # FPROPS test executables
	]
)

tar = env.DistTar("dist/"+env['DISTTAR_NAME']
	, [env.Dir('#')]
)

env.Depends(tar,'ascend.spec')
env.Depends(tar,'doc/book.pdf')

#------------------------------------------------------
# DEBIAN TARBALL for use with Build Service

import glob
deb_files = glob.glob('debian/*.install')
deb_files += glob.glob('debian/*.docs')
deb_files += glob.glob('debian/*.dirs')
deb_files += glob.glob('debian/*.man')
deb_files += glob.glob('debian/*.manpages')
deb_files += ['debian/%s' % s for s in ['rules','control','changelog','compat','copyright','dirs']]

deb_tar = env.Tar(
	'dist/debian.tar.gz'
	,deb_files
	,TARFLAGS = ['cz']
)

Alias('dist',[tar,deb_tar])

#------------------------------------------------------
# DOCUMENTATION

#print "WITH_DOC_BUILD = ",with_doc_build

if not with_doc_build:
	print "Skipping... Documentation isn't being built:",without_doc_build_reason

#user's manual
env.SConscript('doc/SConscript',['env'])

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
	default_targets.append('pyfprops')
#if with_installer:
#	default_targets.append('installer')
if with_extfns:
	default_targets.append('extfns')
if with_doc_build:
	default_targets.append('doc')

env.Default(default_targets)

print "Building targets:"," ".join([str(i) for i in BUILD_TARGETS])

# vim: set syntax=python:
