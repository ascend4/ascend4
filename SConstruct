#!/usr/bin/env python3 -m SCons
# This is a build script for use with SCons. Use it to compile ASCEND on 
# Linux, Windows. It should also give some success on Mac, although this is
# much less tested.

# version number for this ASCEND build:
version = "0.9.10"

# shared library API numbering, for Linux (FIXME windows too?)
soname_major_int = "1"
soname_minor = ".0"

import sys, os, subprocess, platform, distutils.sysconfig, os.path, re, types, pathlib
import subprocess, shutil

# version number for python, useful on Windows
pyversion = "%d.%d" % (sys.version_info[0],sys.version_info[1])

# architecture label
winarchtag = "-win32"
mingw64suff = ""
mingw64excpt ="_dw2"
if platform.architecture()[0] == "64bit":
	winarchtag="-amd64"
	mingw64suff = "_64"
	mingw64excpt ="_seh"

import SCons.Warnings
SCons.Warnings.suppressWarningClass(SCons.Warnings.VisualCMissingWarning) 

#------------------------------------------------------
# PLATFORM DEFAULTS

#print "PLATFORM = ",platform.system()

soname_major = "." + soname_major_int
default_install_prefix = '/usr/local'
default_install_bin = "$INSTALL_PREFIX/bin"
default_install_lib = "$INSTALL_PREFIX/lib"
default_install_models = "$INSTALL_LIB/ascend/models"
default_install_solvers = "$INSTALL_LIB/ascend/solvers"
default_install_assets = "$INSTALL_ASCDATA/glade/"
default_install_ascdata = "$INSTALL_SHARE/ascend"
default_install_include = "$INSTALL_PREFIX/include"
default_install_python = distutils.sysconfig.get_python_lib(prefix="$INSTALL_PREFIX",plat_specific=1)
default_install_python_ascend = "$INSTALL_PYTHON/ascend"
default_tcl = '/usr'
default_tcl_libpath = "$TCL/lib"
default_tcl_cpppath = "$TCL/include"
default_conopt_envvar="CONOPT_PATH"
default_with_graphviz = True
default_tcl_lib = "tcl8.6"
default_tk_lib = "tk8.6"
default_tktable_lib = "Tktable2.9"
default_ida_prefix="$DEFAULT_PREFIX"
#default_ipopt_libpath = "$IPOPT_PREFIX/lib"
#default_ipopt_dll = ["$DEFAULT_PREFIX/bin/%s.dll"%i for i in ["libgfortran$MINGW64SUFF-3", "libstdc++$MINGW64SUFF-6","libquadmath$MINGW64SUFF-0","libgcc_s$MINGW64EXCPT$MINGW64SUFF-1"]]+[None] # should be five here
#default_ipopt_libs = ["$F2C_LIB","blas","lapack","pthread","ipopt"]
default_conopt_prefix="$DEFAULT_PREFIX"
default_conopt_libpath="$CONOPT_PREFIX"
default_conopt_cpppath="$CONOPT_PREFIX"
default_conopt_dlpath="$CONOPT_PREFIX"
default_prefix="/usr"
default_libpath="$DEFAULT_PREFIX/lib"
default_cpppath="$DEFAULT_PREFIX/include"
default_f2c_lib="gfortran"
default_swig="swig"
default_pcre_prefix="$DEFAULT_PREFIX"
default_pcre_libs=['pcre']
default_pcre_libpath="$PCRE_PREFIX/lib"
default_pcre_cpppath="$PCRE_PREFIX/include"

icon_extension = '.png'

if platform.system()=="Windows":
	try:
		d = os.path.split(os.path.dirname(WhereIs("gcc.exe")))[0]
		default_prefix=d
	except:
		default_prefix="c:\\mingw"

	default_libpath="$DEFAULT_PREFIX\\lib"
	default_cpppath="$DEFAULT_PREFIX\\include"	

	# these correspond the the version of Tcl/Tk linked to in the NSIS scripts
	default_tcl_lib = "tcl85"
	default_tk_lib = "tk85"
	default_tktable_lib = "Tktable28"

	# on windows, we locate explicitly in gtkbrowser.py:
	default_install_assets = ""

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
	default_ida_prefix = "$DEFAULT_PREFIX"
	
	# IPOPT. we now prefer to build our own version.
#	default_ipopt_libs = ["ipopt",'stdc++','coinmumps','coinmetis','coinlapack','coinblas','gfortran','pthread']

	# where to look for CONOPT when compiling
	default_conopt_prefix = "c:\\Program Files\\CONOPT"
	default_conopt_lib="conopt3"
		
	need_libm = False
	python_exe = sys.executable
	default_with_scrollkeeper=False
	pathsep = ";"
	
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
	default_install_models = "$INSTALL_BIN/Models"
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

	# we should move these to /Library/ASCEND/Models and /Library/ASCEND/Solvers, for visibility
	default_library_rel_dist = 'Models'
	default_solvers_rel_dist = 'Solvers'

	# where to look for CONOPT when compiling
	default_conopt_prefix = "/Library/CONOPT"

	default_conopt_lib="conopt3"
		
	need_libm = False
	python_exe = sys.executable
	default_with_scrollkeeper=False
	pathsep = ";"
	
	if not os.path.exists(default_conopt_prefix):
		default_conopt_prefix = None
	
else: # LINUX, unix we hope

	# the scons 'philosophy' if there is one is that the scons input arguments
	# completely determine what external tools get picked any time there might
	# be choices.
	# We don't follow that philosophy with regard to tcl/tk.
	# tcl/tk are often in multiple versions and locations on any given system.
	# This leaves us trying to guess whether we should take:
	# - only explicit arguments, otherwise disable tcl, tk
	# - the first tcl/tk in the users path
	# - the tcl/tk under a given user-specified prefix
	# - the folklore-based canonical location of distributor's tcl/tk on a given linux dist
	# We know one thing for sure. Any tcl installation includes tclsh, and thus
	# it's the canonical way to find anything about tcl once tclsh is picked.
	# So all the above reduces to 'how do we find tclsh?'
	icon_extension = '.svg'

	# here's the folklore we know.
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
		_f = open("/etc/lsb-release")
		_r = re.compile("([A-Z][^=]*)=(.*)")
		LSB = {}
		for l in _f:
			_m = _r.match(l.strip())
			LSB[_m.group(1)] = _m.group(2)
		print(LSB)
		if 'DISTRIB_ID' in LSB and LSB['DISTRIB_ID'] == "Ubuntu":
			if float(LSB['DISTRIB_RELEASE']) >= 9.04:
				default_tcl_lib = "tcl8.6"
				default_tk_lib = "tk8.6"
				default_tktable_lib = "Tktable2.9"
				default_tcl_cpppath = "/usr/include/tcl8.6"
			if not os.path.exists(default_tcl_cpppath):
				default_tcl_lib = "tcl8.6"
				default_tk_lib = "tk8.6"
				default_tktable_lib = "Tktable2.9"
				default_tcl_cpppath = "/usr/include/tcl8.6"
				
	# centos 5
	if os.path.exists("/etc/redhat-release"):
		default_tcl_cpppath = "/usr/include"
		default_tcl_lib = "tcl"
		if sys.maxsize > 2**32:
			default_tcl_libpath = "/usr/lib64"
		else:
			default_tcl_libpath = "/usr/lib"
		default_tk_lib = "tk"
		default_tktable_lib = "Tktable2.9"
			

	default_absolute_paths = True
	default_dist_rel_bin = '..'
	default_tk_rel_dist = 'share/ascend/tcltk'
	default_library_rel_dist = 'lib/ascend/models'
	default_solvers_rel_dist = 'lib/ascend/solvers'

	default_conopt_libpath="$CONOPT_PREFIX/lib"
	default_conopt_cpppath="$CONOPT_PREFIX/include"
	default_conopt_dlpath= default_conopt_libpath + ":/usr/local/lib"
	default_conopt_lib="consub3"

	need_libm = True
	if not os.path.isdir(default_tcl):
		default_tcl = '/usr'
	python_exe = sys.executable
	default_with_scrollkeeper=False
	pathsep = ":"

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
	
vars.Add('HOST_PREFIX'
	,"Host architecture prefix"
	,""
)

vars.Add('CC'
	,'C Compiler command'
	,"${HOST_PREFIX}gcc"
)

vars.Add('CXX'
	,'C++ Compiler command'
	,"${HOST_PREFIX}g++"
)

vars.Add('ADDCCFLAGS'
	,'Any additional CC flags to be added, eg -Werror'
	,""
)

vars.Add(BoolVariable('GCOV'
	, 'Whether to enable coverage testing in object code'
	, False
))

if platform.system()!="Windows":
	vars.Add(BoolVariable('WITH_GCCVISIBILITY'
		,"Whether to use GCC Visibility features (only applicable if available)"
		,True
	))

vars.Add(BoolVariable('WITH_SIGNALS'
	,"Whether to permit use of signals for flow control in the C-level code"
	,True
))

# You can turn off building of Tcl/Tk interface
vars.Add(BoolVariable('WITH_TCLTK'
	,"Set to False if you don't want to build the original Tcl/Tk GUI."
	, True
))

# You can turn off the building of the Python interface
vars.Add(BoolVariable('WITH_PYTHON'
	,"Set to False if you don't want to build Python wrappers."
	, True
))

vars.Add(
	'PYTHON'
	,"Python executable"
	,sys.executable
)

vars.Add(
	'PYTHON_PKG'
	,"Python package name in pkg-config"
	,"python-%d.%d" % (sys.version_info[0],sys.version_info[1])
)

vars.Add(
	'DLLDIRS'
	,"Directories where external DLLs can be loaded from by ASCEND (eg IPOPT,IDA) (Windows only, semicolon-delimited)"
	,"$CUNIT_PREFIX/bin;solvers/ipopt;solvers/qrslv"
)

default_python_pkg_embed = "$PYTHON_PKG"
if sys.version_info[0] == 3 and sys.version_info[1] >= 8:
	default_python_pkg_embed = "${PYTHON_PKG}-embed"

vars.Add(
	'PYTHON_PKG_EMBED'
	,"Python package name in pkg-config for embedded python"
	,default_python_pkg_embed
)

# Which solvers will we allow?
vars.Add(ListVariable('WITH_SOLVERS'
	,"List of the solvers you want to build. The default is the minimum that"	
		+" works. The option 'LSOD' is provided for backwards compatibility"
		+"; the value 'LSODE' is preferred."
	,["QRSLV","CMSLV","LSODE","IDA","CONOPT","LRSLV","IPOPT","DOPRI5"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSODE','LSOD','OPTSQP',"IDA","TRON","IPOPT","DOPRI5","MAKEMPS","RADAU5"
	 ]
))

# Where will the local copy of the help files be kept?
vars.Add(BoolVariable('WITH_DOC'
	, "Should we try to build and install help files? If not, ASCEND will access online help files"
	, True
))

vars.Add(BoolVariable('WITH_DOC_BUILD'
	, "If true, we'll attempt to build docs. Set false, we'll assume we already have then (eg from the tarball)"
	, "$WITH_DOC"
))

vars.Add(BoolVariable('WITH_DOC_INSTALL'
	, "If true, SCons will install the documentation file(s). If false, assume rpm or dpkg is going to do it."
	, "$WITH_DOC"
))

vars.Add('HELP_ROOT'
	, "Location of the main help file"
	, "$INSTALL_DOC/book.pdf"
)

# Will bintoken support be enabled?
vars.Add(BoolVariable('WITH_BINTOKEN'
	,"Enable bintoken support? This means compiling models as C-code before"
		+" running them, to increase solving speed for large models."
	,False
))

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
vars.Add('DEFAULT_ASCENDLIBRARY'
	,"Set the default value of the ASCENDLIBRARY -- the location where"
		+" ASCEND will look for models when running ASCEND"
	,"$INSTALL_MODELS"
)

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
vars.Add('DEFAULT_ASCENDSOLVERS'
	,"Set the default value of ASCENDSOLVERS -- the location where"
		+" ASCEND will look for solver shared-library files"
	,"$INSTALL_SOLVERS"
)

# What should the default ASCENDBTINC path be?
vars.Add('DEFAULT_ASCENDBTINC'
	,"Set the default value of ASCENDBTINC -- the location where"
		+" ASCEND will look for bintoken files for fast compilation of"
		+" equations."
	,"$INSTALL_INCLUDE"
)

vars.Add('DEFAULT_ASCENDBTLIB'
	,"Set the default value of ASCENDBTLIB -- the location where"
		+" ASCEND will look for bintoken files for fast compilation of"
		+" equations."
	,"$INSTALL_LIB"
)

# Where is SWIG?
vars.Add('SWIG'
	,"SWIG location, probably only required for MinGW and MSVC users."
		+" Enter the location as a Windows-style path, for example"
		+" 'c:\\msys\\1.0\\home\\john\\swigwin-1.3.29\\swig.exe'."
	,default_swig
)

# Build the test suite?
vars.Add(BoolVariable('WITH_CUNIT'
	,"You can disable CUnit tests with this option. This will basically stop"
		+" SCons from parsing the SConscript files relating to the 'test'"
        +" target, which just might make things marginally faster. Probably"
		+" you can just ignore this option though. SCons will sniff for Cunit"
		+" but build the tests only if you specify the 'test' target."
	,True
))

# zlib support
vars.Add(BoolVariable('WITH_ZLIB'
	,"Include features that make use of the zlib file compression library,"
	+" if available. Set to zero if you want to explicitly disable this."
	,True
))

# Build with MMIO matrix export support?
vars.Add(BoolVariable('WITH_MMIO'
	,"Include support for exporting matrices in Matrix Market format"
	,True
))

#----- default paths -----
vars.Add(PackageVariable('DEFAULT_PREFIX'
	,"Where are most of the shared libraries located on your system?"
	,default_prefix
))

#------ cunit --------
# CUnit is a unit testing library that we use to test libascend.

# Where was CUNIT installed?
vars.Add(PackageVariable('CUNIT_PREFIX'
	,"Where are your CUnit files?"
	,"$DEFAULT_PREFIX"
))

# Where are the CUnit includes?
vars.Add(PackageVariable('CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,"$CUNIT_PREFIX/include"
))

# Where are the CUnit libraries?
vars.Add(PackageVariable('CUNIT_LIBPATH'
	,"Where are your CUnit libraries?"
	,"$CUNIT_PREFIX/lib"
))

# ----- conopt-----

vars.Add(PackageVariable("CONOPT_PREFIX"
	,"Prefix for your CONOPT install (CONOPT ./configure --prefix)"
	,default_conopt_prefix
))

vars.Add("CONOPT_LIB"
	,"Library linked to for CONOPT. This is the name of the CONOPT .so or DLL. On Windows it seems to be called 'copopt3' but on linux it seems to be called 'consub3'."
	,default_conopt_lib
)

vars.Add(BoolVariable("CONOPT_LINKED"
	,"Do you want to dynamically link to CONOPT (only possible if CONOPT is available at buildtime)"
	,False
))

vars.Add('CONOPT_CPPPATH'
	,"Where is your conopt.h?"
	,default_conopt_cpppath
)

vars.Add('CONOPT_LIBPATH'
	,"Where is your CONOPT library installed?"
	,default_conopt_libpath
)

vars.Add('CONOPT_DLPATH'
	,"Default (fallback) search path that ASCEND should use when dlopening the CONOPT library at runtime? This is only used if the conopt environment variable doesn't exist and doesn't point to a location where the DLL/SO is found.  This is in platform-specific form (paths with ';' separator in Windows, ':' separator on Linux)."
	,default_conopt_dlpath
)

vars.Add('CONOPT_ENVVAR'
	,"Name of the optional environment variable which will be used for the value of the searchpath for the CONOPT DLL/SO."
	,default_conopt_envvar
)

#------- IPOPT -------

vars.Add(PackageVariable("IPOPT_PREFIX"
	,"Prefix for your IPOPT install (IPOPT ./configure --prefix)"
,pathlib.Path(os.environ['HOME'])/'.local'
))

#
#	vars.Add("IPOPT_LIBS"
#		,"Library linked to for IPOPT"
#		,default_ipopt_libs
#	)
#
#	vars.Add("IPOPT_LIBPATH"
#		,"Where is your IPOPT library installed"
#		,default_ipopt_libpath
#	)
#
#	vars.Add('IPOPT_CPPPATH'
#		,"Where is your IPOPT coin/IpStdCInterface.h (do not include the 'coin' in the path)"
#		,"$IPOPT_PREFIX/include"
#	)

#	vars.Add('MINGW64SUFF'
#		,"Suffix for 64-bit GCC-related DLLs for bundling with the installer"
#		,mingw64suff
#	)

#	vars.Add('MINGW64EXCPT'
#		,"Suffix to specify exception style for GCC-related DLLs to be bundled with the installer"
#		,mingw64excpt
#	)

#	for i in range(5):
#		vars.Add('IPOPT_DLL%d'%(i+1)
#			,"Exact path of IPOPT DLL (%d) to be included in the installer (Windows only)"%(i+1)
#			,default_ipopt_dll[i]
#		)

#-------pcre--------

vars.Add(BoolVariable('WITH_PCRE'
	,"Set to False if you don't want to enable searching of notes with PCRE1"
	, True
))

vars.Add(PackageVariable("PCRE_PREFIX"
	,"Prefix for your PCRE install (PCRE ./configure --prefix)"
	,default_pcre_prefix
))

vars.Add("PCRE_LIBS"
	,"Library linked to for PCRE"
	,default_pcre_libs
)

vars.Add("PCRE_LIBPATH"
	,"Where is your IPOPT library installed"
)

vars.Add('PCRE_CPPPATH'
	,"Where is your IPOPT coin/IpStdCInterface.h (do not include the 'coin' in the path)"
	,"$PCRE_PREFIX/include"
)

#-------- f2c ------

vars.Add("F2C_LIB"
	,"F2C library (eg. g2c, gfortran, f2c)"
	,default_f2c_lib # the default is gfortran now
)

vars.Add(PackageVariable("F2C_LIBPATH"
	,"Directory containing F2C library (i.e. g2c, gfortran, f2c, etc.), if not already accessible"
	,"off"
))

vars.Add("FORTRAN"
	,"Fortran compiler (eg g77, gfortran)"
	,"${HOST_PREFIX}gfortran"
)

vars.Add("SHFORTRAN"
	,"Fortran compiler for shared library object (should normally be same as FORTRAN)"
	,"$FORTRAN"
)

#------- tcl/tk --------

vars.Add('TCL'
	,'Base of Tcl distribution'
	,default_tcl
)

# Where are the Tcl includes?
vars.Add('TCL_CPPPATH'
	,"Where are your Tcl include files?"
	,default_tcl_cpppath
)

# Where are the Tcl libs?
vars.Add('TCL_LIBPATH'
	,"Where are your Tcl libraries?"
	,default_tcl_libpath
)

# What is the name of the Tcl lib?
vars.Add('TCL_LIB'
	,"Name of Tcl lib (eg 'tcl' or 'tcl83'), for full path to static library (if STATIC_TCLTK is set)"
	,default_tcl_lib
)

# Where are the Tk includes?
vars.Add('TK_CPPPATH'
	,"Where are your Tk include files?"
	,'$TCL_CPPPATH'
)

# Where are the Tk libs?
vars.Add('TK_LIBPATH'
	,"Where are your Tk libraries?"
	,'$TCL_LIBPATH'
)

# What is the name of the Tk lib?
vars.Add('TK_LIB'
	,"Name of Tk lib (eg 'tk' or 'tk83'), or full path to static library"
	,default_tk_lib
)	

# Static linking to TkTable

vars.Add(BoolVariable('STATIC_TCLTK'
	,'Set true for static linking for Tcl/Tk and TkTable. EXPERIMENTAL'
	,False
))

vars.Add('TKTABLE_LIBPATH'
	,'Location of TkTable static library'
	,'$TCL_LIBPATH/Tktable2.8'
)

vars.Add('TKTABLE_LIB'
	,'Stem name of TkTable (eg tktable2.8, no ".so" or "lib") shared library, or full path of static tktable (/usr/lib/...)'
	,default_tktable_lib
)

vars.Add('TKTABLE_CPPPATH'
	,'Location of TkTable header file'
	,'$TCL_CPPPATH'
)

vars.Add('X11'
	,'Base X11 directory. Only used when STATIC_TCLTK is turned on. EXPERIMENTAL'
	,'/usr/X11R6'
)

vars.Add('X11_LIBPATH'
	,'Location of X11 lib. EXPERIMENTAL'
	,'$X11/lib'
)

vars.Add('X11_CPPPATH'
	,'Location of X11 includes. EXPERIMENTAL'
	,'$X11/include'
)

vars.Add('X11_LIB'
	,'Name of X11 lib. EXPERIMENTAL'
	,'X11'
)

#----- installed file locations (for 'scons install') -----

vars.Add('INSTALL_PREFIX'
	,'Root location for installed files'
	,default_install_prefix
)

vars.Add('INSTALL_BIN'
	,'Location to put binaries during installation'
	,default_install_bin
)

vars.Add('INSTALL_LIB'
	,'Location to put libraries during installation'
	,default_install_lib
)

vars.Add('INSTALL_SHARE'
	,'Common shared-file location on this system'
	,"$INSTALL_PREFIX/share"
)

vars.Add('INSTALL_ASCDATA'
	,"Location of ASCEND shared data (TK, python, models etc)"
	,default_install_ascdata
)

vars.Add('INSTALL_PYTHON'
	,'General location for Python extensions on this system'
	,default_install_python
)

vars.Add('INSTALL_PYTHON_ASCEND'
	,'Location for installation of Python modules specific to ASCEND'
	,default_install_python_ascend
)

vars.Add('INSTALL_TK'
	,'Location for Tcl/Tk files used by ASCEND Tk GUI'
	,"$INSTALL_ASCDATA/tcltk"
)

vars.Add('INSTALL_MODELS'
	,"Location of ASCEND model files (.a4c,.a4l,.a4s)"
	,default_install_models
)

vars.Add('INSTALL_SOLVERS'
	,"Location of ASCEND solvers"
	,default_install_solvers
)

vars.Add('INSTALL_DOC'
	,"Location of ASCEND documentation files"
	,"$INSTALL_SHARE/doc/ascend-"+version
)

vars.Add('INSTALL_INCLUDE'
	,'Location to put header files during installation'
	,default_install_include
)

vars.Add('INSTALL_ROOT'
	,'For use by RPM only: location of %{buildroot} during rpmbuild'
	,""
)

vars.Add('EXTLIB_SUFFIX'
	,"Filename suffix for ASCEND 'external libraries' (for use with IMPORT"
	,"_ascend$SHLIBSUFFIX"
)

vars.Add('EXTLIB_PREFIX'
	,"Filename suffix for ASCEND 'external libraries' (for use with IMPORT"
	,"$SHLIBPREFIX"
)

#----------------------

vars.Add('PYGTK_ASSETS'
	,'Default location for Glade assets (will be recorded in pygtk/config.py)'
	,default_install_assets
)

vars.Add(BoolVariable('DEBUG'
	,"Compile source with debugger symbols, eg for use with 'gdb'"
	,False
))

vars.Add(BoolVariable('MALLOC_DEBUG'
	,"Compile with debugging version of MALLOC. Required for full CUnit testing"
	,False
))

#------ dmalloc --------
vars.Add(PackageVariable('DMALLOC_PREFIX'
	,"Where are your dmalloc files?"
	,"$DEFAULT_PREFIX"
))

vars.Add(PackageVariable('DMALLOC_CPPPATH'
	,"Where are your dmalloc include files?"
	,default_cpppath
))

vars.Add(PackageVariable('DMALLOC_LIBPATH'
	,"Where are your dmalloc libraries?"
	,default_libpath
))

vars.Add(BoolVariable('WITH_DMALLOC'
	,"Link to the DMALLOC library (if available) for debugging of memory usage."
	,False
))

vars.Add(BoolVariable('WITH_GRAPHVIZ'
	,"Link to the GRAPHVIZ library (if available, for generating incidence graphs)"
	,default_with_graphviz
))


#------ ufsparse --------
vars.Add(PackageVariable('UFSPARSE_PREFIX'
	,"Where are your UFSPARSE files?"
	,"$DEFAULT_PREFIX"
))

vars.Add(PackageVariable('UFSPARSE_CPPPATH'
	,"Where are your UFSPARSE include files?"
	,default_cpppath
))

vars.Add(PackageVariable('UFSPARSE_LIBPATH'
	,"Where are your UFSPARSE libraries?"
	,default_libpath
))

vars.Add(BoolVariable('WITH_UFSPARSE'
	,"Link to the UFSPARSE library (if available, for additional sparse matrix routines)"
	,True
))

#-----------------------

vars.Add(BoolVariable('UPDATE_NO_YACC_LEX'
	,"Update the *_no_yacc* and *_no_lex* files in the source tree? (these files are created so that ASCEND can be compiled in the absence of those tools)"
	,False
))

vars.Add('DISTTAR_NAME'
	,"Stem name of the tarball created by 'scons dist'. So for 'ascend-aaa.tar.bz2', set this to 'ascend-aaa'."
	,"ascend-"+version
)

vars.Add('RELEASE'
	,"Release number for use in RPM spec file. This should always start with a zero for releases made by the ASCEND group, in order that third parties can make 'patch' releases of higher version numbers."
	,"0"
)

vars.Add(BoolVariable('ABSOLUTE_PATHS'
	,"Whether to use absolute or relative paths in the installed Tcl/Tk interface. If you want to build an RPM, set this to false."
	,default_absolute_paths
))

vars.Add('WIN_INSTALLER_NAME'
	,"Name of the installer .exe to create under Windows (minus the '.exe')"
	,"ascend-"+version+winarchtag+"-py"+pyversion+".exe"
)

vars.Add(BoolVariable('WITH_XTERM_COLORS'
	,"Set to 0 if you don't want xterm colour codes in the console output"
	,True
))

vars.Add(BoolVariable('WITH_EXTFNS'
	,"Set to 0 if you don't want to attempt to build the external modules bundled with ASCEND"
	,True
))

vars.Add(BoolVariable('WITH_SCROLLKEEPER'
	,"Set to to 1 if you want to install an OMF file that can be read by scrollkeeper (eg Yelp on GNOME)"
	,default_with_scrollkeeper
))

vars.Add(BoolVariable('WITH_MSVCR71'
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
	'lex', 'yacc', 'fortran', 'swig', 'textfile'#, 'substinfile'
	,'disttar', 'tar', 'sundials', 'dvi', 'pdflatex', 'graphviz'
]
if platform.system()=="Windows":
	tools += ['nsis']
	
#	if os.environ.get('OSTYPE')=='msys' or os.environ.get('MSYSTEM'):
	envenv = os.environ
	tools += ['mingw']
	envadditional['IS_MINGW']=True
#	else:
#		print("Assuming VC++ build environment (Note: MinGW is preferred)")
#		envenv = {
#			'PATH':os.environ['PATH']
#			,'INCLUDE':os.environ['INCLUDE']
#			,'LIB':os.environ['LIB']
#			,'MSVS_IGNORE_IDE_PATHS':1
#		}
#		tools += ['default']
#		envadditional['CPPDEFINES']=['_CRT_SECURE_NO_DEPRECATE']
else:
	envenv = os.environ
	tools += ['default','doxygen','ipopt']

env = Environment(
	ENV=envenv
	, toolpath=['scons']
	, tools=tools
	, **envadditional
)

# Create .def files by default on Windows (or else SCons 2.0.1 never seems to be happy)
if platform.system()=="Windows":
	env.Append(WINDOWS_INSERT_DEF=1)

#print "PATH =",os.environ['PATH']
#print "PROGSUFFIX =",env['PROGSUFFIX']
#print "CPPPATH =",env['CPPPATH']

vars.Update(env)

for l in ['SUNDIALS','IPOPT']:
	var = "%s_LIBS" % l
	if env.get(var) and not isinstance(env[var],list):
		env[var] = env[var].split(",")

if 'LSOD' in env['WITH_SOLVERS']:
	if 'LSODE' not in env['WITH_SOLVERS']:
		env['WITH_SOLVERS'].append('LSODE')
	env['WITH_SOLVERS'].remove('LSOD')

vars.Save('options.cache',env)

Help(vars.GenerateHelpText(env))

if env['ENV'].get('HOST_PREFIX'):
	triplet = re.compile("^[a-z0-9_]+-[a-z0-9_]+-[a-z0-9]+$")
	if not triplet.match(env['ENV']['HOST_PREFIX']):
		print("NOTE: invalid host triplet from environment variable HOST_PREFIX has been ignored")
	else:
		print("NOTE: using HOST_PREFIX=%s from environment to override HOST_PREFIX SCons variable" % env['ENV']['HOST_PREFIX'])
		env['HOST_PREFIX'] = env['ENV']['HOST_PREFIX']+"-"


#-----------------------------
# optional components: attempting to unify handling of these. in each case, they can be disabled via scons command line, or 
# based on tests that determine they are unavailable. reason in each will be reported before building commences.

env['OPTIONALS'] = {}
def set_optional(env,comp,reason=None,active=None):
	"""
	Record WITH_OPTION variables for build, but also store
	reasons why optional components are not being built, so 
	they can be output at the end.
	"""
	NAME = f'WITH_{comp.upper()}'
	if active is None:
		active = False
		if env.get(NAME):
			active = True
	if active:
		reason = None
	if not active and reason is None:
		reason = "disabled in user's build options"
	env['OPTIONALS'][comp] = (active,reason)
	if env.get(NAME) != active:
		env[NAME] = active
	return active

AddMethod(Environment, set_optional, 'set_optional')

for opt in ['tcltk','cunit','extfns','scrollkeeper','dmalloc','graphviz','ufsparse','zlib','mmio','blas','signals','doc','doc_build','pcre','installer']:
	env.set_optional(opt)

if not env['WITH_DOC']:
	env.set_optional('doc_build',reason='documentation was disabled',active=False)

for solv in 'LSODE','IDA','DOPRI5','RADAU5','CONOPT','IPOPT','MAKEMPS':
	env.set_optional(solv,active = solv in env['WITH_SOLVERS'], reason="Not selected (see option WITH_SOLVERS)")
	

print(f"DEBUG: WITH_CUNIT = {env['WITH_CUNIT']}")
print(f"DEBUG: WITH_IPOPT = {env['WITH_IPOPT']}")
print(f"DEBUG: WITH_MAKEMPS = {env['WITH_MAKEMPS']}")

with_latex2html = False

if platform.system()=="Windows":
	with_installer=1
else:
	with_installer=0
	without_installer_reason = "only possible under Windows"

#print "SOLVERS:",env['WITH_SOLVERS']
#print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
#print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

can_install = True
if platform.system()=='Windows':
	can_install = False

env['CAN_INSTALL']=can_install

#print "TCL=",env['TCL']
#print "TCL_CPPPATH =",env['TCL_CPPPATH']
#print "TCL_LIBPATH =",env['TCL_LIBPATH']
#print "TCL_LIB =",env['TCL_LIB']

#print "ABSOLUTE PATHS =",env['ABSOLUTE_PATHS']
#print "INSTALL_ASCDATA =",env['INSTALL_ASCDATA']
#print "INSTALL_PREFIX =",env['INSTALL_PREFIX']
#print "INSTALL_MODELS =",env['INSTALL_MODELS']
#print "INSTALL_SOLVERS =",env['INSTALL_SOLVERS']
#print "INSTALL_PYTHON =",env['INSTALL_PYTHON']
#print "INSTALL_PYTHON_ASCEND =",env['INSTALL_PYTHON_ASCEND']
#print "DEFAULT_ASCENDLIBRARY =",env['DEFAULT_ASCENDLIBRARY']
#print "DEFAULT_ASCENDSOLVERS =",env['DEFAULT_ASCENDSOLVERS']


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
	context.Message("Checking C compiler ('%s')... " % context.env.subst('$CC'))
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
	context.Message("Checking C++ compiler ('%s')... " % context.env.subst('$CXX'))
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

def CheckFortran(context):
	context.Message("Checking Fortran compiler ('%s')..." % context.env.subst('$FORTRAN'))
	if not context.env.get('FORTRAN'):
		context.Result('not found')
		return False
	is_ok = context.TryCompile(f77_test_text,".f")
	context.Result(is_ok)
	return is_ok
	
#----------------
# Address Sanitizer

asan_test_text = """
#if !defined(__has_feature)
# error "__has_feature" not defined"
#else
# if !__has_feature(address_sanitizer)
#  error "address_sanitizer is not available"
# endif
#endif
int main(void){
	return 0;
}
"""

def CheckASan(context):
	context.Message("Checking for AddressSanitizer... ")
	ccf = context.env.get('CCFLAGS')
	context.env.AppendUnique(CCFLAGS=['-O1','-g','-fsanitize=address','-fno-omit-frame-pointer'])
	is_ok = context.TryCompile(asan_test_text,".c")
	context.Result(is_ok)
	if ccf is None:
		del context.env['CCFLAGS']
	else:
		context.env['CCFLAGS'] = ccf	
	return is_ok

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
		
		if varprefix+'_CPPPATH' in context.env:
			context.env.AppendUnique(CPPPATH=[env[varprefix+'_CPPPATH']])
			#print "Adding '"+str(env[varprefix+'_CPPPATH'])+"' to cpp path"

		if static:
			staticlib=env[varprefix+'_LIB']
			#print "STATIC LIB = ",staticlib
			context.env.Append(
				LINKFLAGS=[staticlib]
			)
		else:
			if varprefix+'_LIBPATH' in context.env:
				context.env.Append(LIBPATH=[env[varprefix+'_LIBPATH']])
				#print "Adding '"+str(env[varprefix+'_LIBPATH'])+"' to lib path"

			if varprefix+'_LIB' in context.env:
				context.env.Append(LIBS=[env[varprefix+'_LIB']])
				#print "Adding '"+str(env[varprefix+'_LIB'])+"' to libs"	
			elif varprefix+'_LIBS' in context.env:
				context.env.AppendUnique(LIBS=env[varprefix+'_LIBS'])

	def restore(self,context):
		#print "RESTORING CONTEXT"
		#print self.keep
		#print "..."
		for k in self.keep:
			if self.keep[k]==None:
				if k in context.env:
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
		if not isinstance(libname,str):
			raise RuntimeError("varprefix must be provided, as libname is not a string")
		varprefix = libname.upper()
	
	#print "LIBS is currently:",context.env.get('LIBS')
	keep = KeepContext(context,varprefix,static)

	if varprefix+'_LIB' not in context.env and varprefix+'_LIBS' not in context.env:
		# if varprefix_LIB were in env, KeepContext would 
		# have appended it already
		if isinstance(libname,str):
			context.env.Append(LIBS=[libname])
		else:
			context.env.Append(LIBS=libname)

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
	if 'WITH_GCCVISIBILITY' not in context.env or not env['WITH_GCCVISIBILITY']:
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

print("YACC =",env['YACC'])

def CheckYacc(context):
	context.Message("Checking for Yacc ('%s')... " % context.env.get('YACC'))
	#is_ok = context.TryCompile(yacc_test_text,".y")
	is_ok = context.TryAction('$YACC < $SOURCE > $TARGET',yacc_test_text,".y")
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
	#is_ok = context.TryCompile(lex_test_text,".l")
	is_ok = context.TryAction('$LEX < $SOURCE > $TARGET',lex_test_text,".l")
	context.Result(is_ok)
	return is_ok

lexdestroy_test_text = """
%{
#include <stdio.h>
#include <unistd.h>
%}

%%
username	printf("%s", getlogin());
%%

int yywrap(void){
  return 1;
}

main(){
  //yylex();
  yylex_destroy();
}

"""

def CheckLexDestroy(context):
	context.Message("Checking for yylex_destroy... ")
	is_ok, outstring = context.TryRun(lexdestroy_test_text,".l")
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

# test graphviz agraph...
graphviz_agraph_test_text = """
#ifdef __WIN32__
# include <gvc.h>
#else
# include <graphviz/gvc.h>
#endif
#ifdef WITH_CGRAPH
# error WITH_CGRAPH is defined!
#endif
int main(void){
	Agraph_t *g;
	g = agopen("g", AGDIGRAPH);
	return 0;
}
"""
def CheckGraphVizAgraph(context):
	return CheckExtLib(context,'gvc',graphviz_agraph_test_text,ext=".c",varprefix="GRAPHVIZ",testname="graphviz agraph")

# test graphviz cgraph
graphviz_cgraph_test_text = """
#ifdef __WIN32__
# include <gvc.h>
#else
# include <graphviz/cgraph.h>
# include <graphviz/gvc.h>
#endif
int main(void){
	Agraph_t *g;
	GVC_t *gvc;
	gvc = gvContext();
	g = agopen("g", Agdirected, 0);
	return 0;
}
"""
def CheckGraphVizCgraph(context):
	return CheckExtLib(context,['gvc','cgraph'],graphviz_cgraph_test_text,ext=".c",varprefix="GRAPHVIZ",testname="graphviz cgraph")

#	GVC_t *gvc;
#	gvc = gvContext();

# test for definition of 'boolean' in graphviz/types.h
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
# IDA test

sundials_version_major_required = 2
sundials_version_minor_min = 4
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
# Tcl test

# TCL and TK required version 8.1 through 8.5:
tcltk_minor_newest_acceptable = 6
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

def CheckLyX(context):
	context.Message("Checking for LyX... ")
	r = context.env.WhereIs("lyx")
	if r:
		context.Result(r)
		return True
	else:
		context.Result(0)
		return False 

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
# Check usable 'erf' function

erf_test_text = r"""
#include <math.h>
int main(){
	double x = erf(0.5);
	return 0;
}
"""
def CheckErf(context):
	context.Message("Checking for erf... ")
	libsave=context.env.get('LIBS')
	context.env.AppendUnique(LIBS=['m'])
	(is_ok,output) = context.TryRun(erf_test_text,'.c')
	context.Result(is_ok)

#----------------
# PCRE1 check

pcre_test_text = """
#include <pcre.h>
int main(){
	pcre *re;
	const char *errstr;
	int erroffset;
	re = pcre_compile("^([a-z]+)[0-9])$",0,&errstr,&erroffset,NULL);
}
"""

def CheckPCRE(context):
	return CheckExtLib(context,libname='pcre',text=pcre_test_text)

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
		, 'CheckFortran' : CheckFortran
		, 'CheckMath' : CheckMath
		, 'CheckMalloc' : CheckMalloc
		, 'CheckASan' : CheckASan
		, 'CheckDLOpen' : CheckDLOpen
#		, 'CheckSwigVersion' : CheckSwigVersion
#		, 'CheckPythonLib' : CheckPythonLib
		, 'CheckCUnit' : CheckCUnit
		, 'CheckDMalloc' : CheckDMalloc
		, 'CheckLyX' : CheckLyX
		, 'CheckLatex2HTML' : CheckLatex2HTML
		, 'CheckGraphVizAgraph' : CheckGraphVizAgraph
		, 'CheckGraphVizCgraph' : CheckGraphVizCgraph
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
		, 'CheckLexDestroy' : CheckLexDestroy
		, 'CheckTkTable' : CheckTkTable
		, 'CheckX11' : CheckX11
		, 'CheckIDA' : CheckIDA
		, 'CheckSUNDIALS' : CheckSUNDIALS
		, 'CheckCONOPT' : CheckCONOPT
#		, 'CheckIPOPT' : CheckIPOPT
		, 'CheckScrollkeeperConfig' : CheckScrollkeeperConfig
		, 'CheckFPE' : CheckFPE
		, 'CheckSIGINT' : CheckSIGINT
		, 'CheckSigReset' : CheckSigReset
		, 'CheckErf' : CheckErf
		, 'CheckPCRE' : CheckPCRE
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
		if v[0] == '1' or v[0] == '2' or v[0] == '3' or v[0] == '4':
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
	print("Scons version is not OK. Please try version 0.96.92 or 0.96.93,")
	print("or consult the developers in the case of newer versions. Modify")
	print("the function 'sconsversioncheck' in the file SConstruct if you")
	print("want to *force* SCons to continue.")
	Exit(1)

# check C compiler

if conf.CheckCC() is False:
	print("Failed to build simple test file with your C compiler.")
	print("Check your compiler is installed and running correctly.")
	Exit(1)

if conf.CheckCXX() is False:
	print("Failed to build simple test file with your C++ compiler.")
	print("Check your compiler is installed and running correctly.")
	print("You can set your C++ compiler using the CXX scons option.")
	Exit(1)

if conf.CheckASan() is False:
	conf.env['HAVE_ASAN'] = True
else:
	conf.env['HAVE_ASAN'] = False

# stdio -- just to check that compiler is behaving

if conf.CheckHeader('stdio.h') is False:
	print("CPPPATH =",env.get('CPPPATH'))
	print("Did not find 'stdio.h'! Check your compiler configuration.")
	print("")
	print("You environment is printed here:")
	for k,v in os.environ.items():
		print("%-30s%s" % ("%s :" % k, v))
	Exit(1)

# sizes of vars used in libascend eg in gl_list etc.

_sizes = {
	"VOID_P" : "void *"
	,"INT" : "int"
	,"LONG" : "long"
	,"LONG_LONG" : "long long"
	,"UINT" : "unsigned int"
	,"ULONG" : "unsigned long"
	,"ULONGLONG" : "unsigned long long"
}

for _var,_type in _sizes.items():
	_size = conf.CheckTypeSize(_type)
	if not _size:
		print("Couldn't determine 'sizeof(%s)'" % _type)
		Exit(1)
	#print("SIZEOF_%s is %d"%(_var,_size))
	conf.env["SIZEOF_%s" % (_var,)] = str(_size)

#print("SIZEOF_VOID_P = %s"%(conf.env['SIZEOF_VOID_P']))

# check for some string functions

if conf.CheckFunc('sprintf') is False:
	print("Didn't find sprintf");
	Exit(1)

if conf.CheckErf() is False:
	print("Didn't find erf");
	Exit(1)
else:
	conf.env['HAVE_ERF'] = True

if conf.CheckFunc('strdup'):
	conf.env['HAVE_STRDUP'] = True

if conf.CheckFunc('snprintf'):
	conf.env['HAVE_SNPRINTF'] = True
elif conf.CheckFunc('_snprintf'):
	conf.env['HAVE__SNPRINTF'] = True

if conf.CheckFunc('cpow'):
	conf.env['HAVE_CPOW'] = True

# attempt to support MSVCRT 7.1 on Windows

if platform.system()=="Windows" and env.get('WITH_MSVCR71'):
	conf.env.Append(LIBS='msvcr71')
	
# Math library

conf.env['HAVE_IEEE']=True

if need_libm and (conf.CheckMath() is False):
	conf.env['HAVE_IEEE']=False
	print('Did not find math library, exiting!')
	Exit(1)

# Malloc

if conf.CheckMalloc() is False:
	conf.env['HAVE_MALLOC']=False
	print("Did not find functioning 'malloc', exiting!")
	Exit(1)

# dlopen/LoadLibrary

# CURRENTLY BREAKS LATER TEST (libsave?)
#if conf.CheckDLOpen() is False:
#	print "Did not find functioning dlopen/LoadLibrary, exiting!"
#	Exit(1)

# Where is 'isnan'?

if conf.CheckFunc('isnan') is False and conf.CheckFunc('_isnan') is False:
	print("Didn't find isnan")
#	Exit(1)

# GCC visibility

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env.get('WITH_GCCVISIBILITY') and conf.CheckGccVisibility():
		conf.env['HAVE_GCCVISIBILITY']=True;
		conf.env.Append(CCFLAGS=['-fvisibility=hidden'])
		conf.env.Append(CPPDEFINES=['HAVE_GCCVISIBILITY'])
	conf.env.Append(CCFLAGS=['-Wall','-O2','-g'])

# Catching SIGINT

if conf.env['WITH_SIGNALS']:
	if not conf.CheckSIGINT():
		conf.env.set_optional('signals',active=False,reason="SIGINT uncatchable")

# Catching SIGFPE

if conf.CheckFPE():
	conf.env['HAVE_C99FPE']=True
else:
	conf.env['HAVE_C99FPE']=False

# Checking for signal reset requirement

if conf.CheckSigReset() is False:
	print("Unable to determine if signal reset is required")
	Exit(1)

# YACC

#if conf.CheckYacc():
if conf.env['YACC']:
	conf.env['HAVE_YACC']=True

#if conf.CheckLex():
if conf.env['LEX']:
	conf.env['HAVE_LEX']=True

if conf.CheckLexDestroy():
	conf.env['HAVE_LEXDESTROY']=True

# Tcl/Tk

if conf.env['WITH_TCLTK']:
	if conf.CheckTcl():
		if conf.CheckTclVersion():
			if conf.CheckTk():
				if conf.CheckTkVersion():
					if conf.env['STATIC_TCLTK']:
						if conf.CheckTkTable():
							pass
						else:
							conf.env.set_optional('tcltk',active=False,reason="TkTable not found")
				else:
					conf.env.set_optional('tcltk',active=False,reason="Require Tk version <= 8.4. See 'scons -h'")
			else:
				conf.env.set_optional('tcltk',active=False,reason="Tk not found")
		else:
			conf.env.set_optional('tcltk',active=False,reason="Require Tcl <= 8.4 Tcl.")
	else:
		conf.env.set_optional('tcltk',active=False,reason='Tcl not found')

if conf.env['STATIC_TCLTK']:
	conf.CheckX11()

# CUnit

if conf.env['WITH_CUNIT']:
	conf.env.set_optional('cunit',active=conf.CheckCUnit(),reason='not found')

# DMALLOC

if conf.env['WITH_DMALLOC']:
	conf.env.set_optional('dmalloc',active=conf.CheckDMalloc(),reason='not found')

# GRAPHVIZ

if conf.env['WITH_GRAPHVIZ']:
	if not conf.CheckGraphVizCgraph() and not conf.CheckGraphVizAgraph():
		conf.env.set_optional('graphviz',active=False,reason='graphviz not found (cgraph nor agraph)')
	conf.env['HAVE_GRAPHVIZ_BOOLEAN'] = conf.CheckGraphVizBoolean()		

# UFSPARSE

if conf.env['WITH_UFSPARSE']:
	conf.env.set_optional('ufsparse',active=conf.CheckUFSparse(),reason="not found")

# IDA

if conf.env['WITH_IDA']:
	if not conf.CheckSUNDIALS():
		conf.env.set_optional('ida',active=False,reason="SUNDIALS not found, or bad version")
	else:
		if not conf.CheckIDA():
			conf.env.set_optional('ida',active=False,reason="Unable to compile/link against SUNDIALS/IDA")

# CONOPT

if conf.env['WITH_CONOPT']:
	if not conf.CheckCONOPT():
		conf.env['CONOPT_LINKED']=False
	# we no longer require CONOPT at buildtime in order to build support for it
	#with_conopt = False
	#without_conpt_reason = "CONOPT not found"

# ZLIB

if conf.env['WITH_ZLIB']:
	if not conf.CheckCHeader('zlib.h'):
		conf.env.set_optional('zlib',active=False,reason="zlib.h not found")
	if not conf.CheckLib('z'):
		conf.env.set_optional('zlib',active=False,reason='library libz not found')

# LSODE needs Fortran; no fortran then no LSODE

if conf.env['WITH_LSODE']:
	conf.env.set_optional('lsode',active=conf.CheckFortran(),reason="Fortran compiler was not functional. Check setup.")

# BLAS (WITH_BLAS = a copy exists on the system)

if conf.env['WITH_LSODE']:
	# we only need BLAS for LSODE
	if env['WITH_BLAS']:
		print("CHECKING BLAS")
		conf.env.set_optional('blas',active=conf.CheckLib('blas'),reason="Not found (local code will be used instead)")

# scrollkeeper

if conf.env['WITH_SCROLLKEEPER']:
	conf.env.set_optional('scrollkeeper',active=conf.CheckScrollkeeperConfig(),reason="unable to detect 'scrollkeeper-config'")

# lyx

if conf.env['WITH_DOC_BUILD']:
	conf.env.set_optional('doc_build',active=conf.CheckLyX(),reason="unable to locate LyX")
	print(f"WITH_DOC_BUILD = {conf.env['WITH_DOC_BUILD']}")

# PCRE

if conf.env['WITH_PCRE']:
	conf.env.set_optional('pcre',active=conf.CheckPCRE(),reason="PCRE not found")

# TODO: -D_HPUX_SOURCE is needed

# TODO: detect if dynamic libraries are possible or not

if platform.system()=="Windows" and 'MSVS' in env:
	_found_windows_h = conf.CheckHeader('Windows.h')

	if not _found_windows_h:
		print("Could not locate 'Windows.h' in CPPPATH. Check your configuration.")
		Exit(1)

	if with_python and conf.CheckHeader(['basetsd.h','BaseTsd.h']) is False:
		with_python = 0;
		without_python_reason = "Header file 'basetsd.h' not found. Install the MS Platform SDK."

#print("1. SIZEOF_VOID_P = %s"%(conf.env['SIZEOF_VOID_P']))
env = conf.Finish()
#print("2. SIZEOF_VOID_P = %s"%(env['SIZEOF_VOID_P']))
#print "-=-=-=-=-=-=-=-=- LIBS =",env.get('LIBS')

#---------------------------------------
# SUBSTITUTION DICTIONARY for .in files

def cygpath(mypath):
	cmd = [pathlib.Path(shutil.which('cygpath')),'-w',mypath]
	print(f"CMD = {cmd}")
	return subprocess.run(cmd,check=1,capture_output=1,encoding="utf=8").stdout.strip("\r\n \t")

def get_dlldirs(pathlist):
	print("start:",pathlist)
	l1 = str(pathlist).split(os.pathsep)
	print(f"l1 = {l1}")
	l2 = [cygpath(p) for p in l1]
	print(f"l2 = {l2}")
	return os.pathsep.join(l2)

if platform.system()=="Windows":	
	env['DLLDIRS'] = get_dlldirs(env.subst(env["DLLDIRS"]))
else:
	env['DLLDIRS'] = ""
#print(f"final DLLDIRS = {env['DLLDIRS']}")

release = env.get('RELEASE')
if release=="0.":
	release="0"

#print "SUBSTITUTED CONOPT_LIBPATH:",c_escape(env.subst("$CONOPT_LIBPATH"))

print("SIZEOF_VOID_P = %s"%(env['SIZEOF_VOID_P']))

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
#	, '@DLLDIRS@':c_escape(env["DLLDIRS"])
	, '@VERSION@':version
	, '@RELEASE@':release
	, '@SONAME_MAJOR_INT@':soname_major_int
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
	, '@DEFAULT_ASCENDBTINC@': c_escape(os.path.abspath(env.subst(env['DEFAULT_ASCENDBTINC'])))
	, '@DEFAULT_ASCENDBTLIB@': c_escape(os.path.abspath(env.subst(env['DEFAULT_ASCENDBTLIB'])))
	, '@ASC_DIST_REL_BIN@' : default_dist_rel_bin
	, '@ASC_TK_REL_DIST@' : default_tk_rel_dist
	, '@ASC_LIBRARY_REL_DIST@' : default_library_rel_dist
	, '@ASC_SOLVERS_REL_DIST@' : default_solvers_rel_dist
	, '@SIZEOF_VOID_P@' : env['SIZEOF_VOID_P']
	, '@SIZEOF_INT@' : env['SIZEOF_INT']
	, '@SIZEOF_LONG@' : env['SIZEOF_LONG']
	, '@SIZEOF_LONG_LONG@' : env['SIZEOF_LONG_LONG']
	, '@SIZEOF_UINT@' : env['SIZEOF_UINT']
	, '@SIZEOF_ULONG@' : env['SIZEOF_ULONG']
	, '@SIZEOF_ULONGLONG@' : env['SIZEOF_ULONGLONG']
	, '@WITH_SOLVERS@' : ",".join(env.get('WITH_SOLVERS'))
	, '@ASCXX_USE_PYTHON@' : "1" if env['WITH_PYTHON'] else "0"
}



if env.get('WITH_DOC'):
	#print "WITH_DOC:",env['WITH_DOC']
	subst_dict['@HELP_ROOT@']=env['HELP_ROOT']

# bool options...
for k,v in {
		'ASC_WITH_DMALLOC':env['WITH_DMALLOC']
		,'ASC_WITH_UFSPARSE':env['WITH_UFSPARSE']
		,'ASC_WITH_MMIO':env['WITH_MMIO']
		,'ASC_WITH_ZLIB':env['WITH_ZLIB']
		,'ASC_WITH_PCRE':env['WITH_PCRE']
		,'ASC_SIGNAL_TRAPS':env['WITH_SIGNALS']
		,'ASC_RESETNEEDED':env.get('ASC_RESETNEEDED')
		,'HAVE_C99FPE':env.get('HAVE_C99FPE')
		,'HAVE_IEEE':env.get('HAVE_IEEE')
		,'HAVE_ERF':env.get('HAVE_ERF')
		,'ASC_XTERM_COLORS':env.get('WITH_XTERM_COLORS')
		,'MALLOC_DEBUG':env.get('MALLOC_DEBUG')
		,'ASC_HAVE_LEXDESTROY':env.get('HAVE_LEXDESTROY')
		,'HAVE_SNPRINTF':env.get('HAVE_SNPRINTF')
		,'HAVE__SNPRINTF':env.get('HAVE__SNPRINTF')
		}.items():
		
#	if v: subst_dict["/\\* #\\s*define %s @%s@ \\*/" % (k,k)]='# define %s 1 ' % k
	if v:
		subst_dict['@%s@' %(k,)] = "#define %s 1" %(k,)
	else:
		subst_dict['@%s@' %(k,)] = "// %s is not set." %(k,)

if with_latex2html:
	env['WITH_LATEX2HTML']=1

if 'HAVE_GCCVISIBILITY' in env:
	subst_dict['@HAVE_GCCVISIBILITY@'] = "1"

env.Append(SUBST_DICT=subst_dict)

#for k,v in subst_dict.iteritems():
#	print "%-50s%s" % ("'%s'"%k,v)

# REMOVED: long command-line support on Win2k

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
SConsEnvironment.InstallProgram = lambda env, dest, files: InstallPerm(env, dest, files, 0o755) 	 
SConsEnvironment.InstallHeader = lambda env, dest, files: InstallPerm(env, dest, files, 0o644)
SConsEnvironment.InstallShared = lambda env, dest, files: InstallPerm(env, dest, files, 0o644)
SConsEnvironment.InstallSharedAs = lambda env, dest, files: InstallPermAs(env, dest, files, 0o644)
SConsEnvironment.InstallLibraryAs = lambda env, dest, files: InstallPermAs(env, dest, files, 0o644)

#------------------------------------------------------
# BUILD...

# so that #include <ascend/modulename/headername.h> works across all modules...
env.AppendUnique(CPPPATH=['#'])

if env['DEBUG']:
	env.Append(
		CCFLAGS=['-g']
		,LINKFLAGS=['-g']
	)

if env['ADDCCFLAGS']:
	env.Append(CCFLAGS=env['ADDCCFLAGS'])

if env['GCOV']:
	env.Append(
		CPPFLAGS=['-g','-fprofile-arcs','-ftest-coverage']
		, LIBS=['gcov']
		, LINKFLAGS=['-fprofile-arcs','-ftest-coverage']
	)

#FIXME there must be a better way of doing this...

#if with_graphviz and env.get('GRAPHVIZ_RPATH'):
#	env.Append(RPATH=env['GRAPHVIZ_RPATH'])

#-------------
# PYTHON INTERFACE

if env.get('WITH_PYTHON'):	
	subst_dict['@ASCXX_USE_PYTHON@']="1"
	env['WITH_PYTHON']=1;

	env.SConscript(['ascxx/SConscript'],'env')
	env.SConscript(['pygtk/SConscript'],'env')

#-------------
# TCL/TK GUI

if env['WITH_TCLTK']:
	env.SConscript(['tcltk/SConscript'],'env')

#------------
# BASE/GENERIC SUBDIRECTORIES

libascend_env = env.Clone()

dirs = ['general','utilities','compiler','system','solver','integrator','packages','linear','bintokens']

srcs = []
for d in dirs:
	heresrcs = libascend_env.SConscript('ascend/'+d+'/SConscript','libascend_env')
	srcs += heresrcs

#-------------
# IMPORTED CODE: LSODE, BLAS, etc

if env['WITH_LSODE'] and not env['WITH_BLAS']:
	env['blasobjs'] = env.SConscript(['blas/SConscript'],'env')
else:
	env['blasobjs'] = []

if env['WITH_MMIO']:
	srcs += env.SConscript(['mmio/SConscript'],'env')

#-------------
# LIBASCEND -- all 'core' functionality

# FIXME want to move these bits to ascend/SConscript

libascend_env.Append(
	CPPPATH=['#']
	,LIBS=['m']
)

if platform.system()=="Linux":
	libascend_env.Append(LIBS=['dl'])

if env['WITH_DMALLOC']:
	libascend_env.Append(LIBS=['dmalloc'])

if env['WITH_UFSPARSE']:
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

if env['WITH_CUNIT']:
	testdirs = ['general','solver','utilities','linear','compiler','system','packages','integrator']
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
	print("Skipping... CUnit tests aren't being built:",without_cunit_reason)

#-------------
# EXTERNAL SOLVERS

env['extfns']=[]
env['BUILDING_ASCEND'] = 1

env.SConscript(['solvers/SConscript'],'env')

#-------------
# EXTERNAL FUNCTIONS

#print(f"env['HAVE_CUNIT'] = {env['HAVE_CUNIT']}")
modeldirs = env.SConscript(['models/SConscript'],'env')

for _f in env['extfns']:
	env.Depends(_f,'libascend')
env.Alias('extfns',env['extfns'])

#-------------
# FPROPS python bindings

env.Alias('pyfprops',env.get('pyfprops'))

#------------------------------------------------------
# CREATE ASCEND-CONFIG scriptlet

ascendconfig = env.Substfile('ascend-config.in')

#------------------------------------------------------
# CREATE a4 CONVENIENCE SCRIPT

a4cmd = env.Substfile('a4.in')
env.AddPostAction(a4cmd, 'chmod 755 $TARGET')

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

with_installer = False
without_installer_reason = "Disabled temporarily in SConstruct"

if with_installer:
	pyarch = ""
	instarch = "win32"
	if platform.architecture()[0] == "64bit":
		instarch = "x64"
		pyarch = ".amd64"
		inst64 = 1
	nsisdefs = {
		'OUTFILE':"#dist/$WIN_INSTALLER_NAME"
		,"VERSION":version
		,'PYVERSION':pyversion
		,'PYPATCH':".%d"%sys.version_info[2]
		,'PYARCH':str(pyarch)
		,'INSTARCH':str(instarch)
	}
	# support up to 5 extra dependency DLLs to accompany IPOPT
	for i in range(5):
		_fl = ''; _dl = ''
		if env.get('IPOPT_DLL%d'%(i+1)):
			_fl = "File %s"%os.path.normcase(os.path.normpath(env.subst("$IPOPT_DLL%d"%(i+1))))
			_dl = "Delete \"$$INSTDIR\\%s\""%os.path.split(env.subst("$IPOPT_DLL%d"%(i+1)))[1]
		nsisdefs['FILE_IPOPT_%d'%(i+1)] = _fl
		nsisdefs['DEL_IPOPT_%d'%(i+1)] = _dl	
	env.Append(NSISDEFINES=nsisdefs)
	installer = env.Installer('nsis/installer.nsi')

	for i in range(5):
		if env.get('IPOPT_DLL%d'%(i+1)):
			env.Depends(installer,env['IPOPT_DLL%d'%(i+1)])
	
	env.Depends(installer,["pygtk","ascxx","tcltk","ascend.dll","models","solvers","ascend-config",'pygtk/ascend'])
	env.Depends(installer,"doc/book.pdf")
	env.Depends(installer,["nsis/detect.nsi","nsis/dependencies.nsi","nsis/download.nsi"])
	env.Alias('installer',installer)
else:
	print("Skipping... Windows installer isn't being built:",without_installer_reason)

#------------------------------------------------------
# CREATE the SPEC file for generation of RPM packages

if platform.system()=="Linux":
	env.Substfile('ascend.spec.in')

#------------------------------------------------------
# CREATE OMF FILE FOR USE WITH SCROLLKEEPER

#if with_scrollkeeper:
#	#env.Substile('#/pygtk/gnome/ascend.omf.in')
#	#env.InstallShared(env['INSTALL_ROOT']+env['OMFDIR'],"#/pygtk/gnome/ascend.omf")

#------------------------------------------------------
# DISTRIBUTION TAR FILE

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.lib','.cc','.cache',
		'.pyc','.cvsignore','.dblite','.log','.pl','.out','.exe','.aux','.idx',
		'.toc','.lof','.lot','.mm','.warnings','.tm2','.swp',',tmp','.gz',
		'.bz2','.7z','.deb','.dsc','.changes','.bak','.tex','.tmp','.def']
	, DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist','debian','doxy']
	, DISTTAR_EXCLUDERES=[r"_wrap\.cc?$", r"~$", r"ascxx/ascpy\.py","ascxx/testipopt$"
		,r"/lib.*\.so\.[.0-9]+$", r"tcltk/asc4dev$", r"tcltk/interface/typelex\.c$"
		,r"ascend/compiler/ascParse\.[ch]$", r"ascend/solver/conoptconfig\.h$"
		,r"ascend/utilities/config\.h$", r"pygtk/config\.h$", r"pygtk/config\.py$"
		,r"pygtk/ascdev$", r"ascxx/testconopt$", r"ascend/compiler/scanner\.c$"
		,r"datareader/.*TY\.csv$"
		,r"[a-z]+/.*/.*\.spec$"
		,r"ascxx/ascpy_wrap\.h",r"ascxx/config\.h$"
		,r"tcltk/interface/ascend4$",r"ascxx/testslvreq$",r"test/test$"
		,r"models/johnpye/datareader/.*\.tm2\.Z$"
		,r"models/johnpye/fprops/[a-z][a-z0-9]+(.*\.exe)?$" # FPROPS test executables
		,r"fprops/fluids/fluids_list\.h$" # FPROPS fluids list
		,r"fprops/test/ph$"
		,r"fprops/test/sat$"
		,r"fprops/test/sat1$"
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

if env['WITH_DOC_BUILD']:
	#user's manual
	env.SConscript('doc/SConscript',['env'])

#------------------------------------------------------
# RPM BUILD

# for RPM builds, 'scons dist' then 'rpmbuild -ta dist/ascend-*.tar.bz2'
# (check * for the version number used to create the tarball)

#------------------------------------------------------
# DEFAULT TARGETS

default_targets =['libascend','solvers',a4cmd]
if env['WITH_TCLTK']:
	default_targets.append('tcltk')
if 'WITH_PYTHON' in env:
	default_targets.append('ascxx')
	default_targets.append('pygtk')
	default_targets.append('pyfprops')
if env['WITH_EXTFNS']:
	default_targets.append('extfns')
if env['WITH_DOC_BUILD']:
	default_targets.append('doc')
if env['WITH_CUNIT']:
	default_targets.append('test')

env.Default(default_targets)

# reporting skipped components

for comp,val in env['OPTIONALS'].items():
	active,reason = val
	if active:
		pass #print(f"Building {comp}")
	else:
		print(f"Skipping {comp}... {reason}")

print("Building targets:"," ".join([str(i) for i in BUILD_TARGETS]))

print(f"WITH_DOC_BUILD = {env['WITH_DOC_BUILD']}")
# vim: ts=4:sw=4:noet:syntax=python
