import os, commands, platform, distutils.sysconfig, os.path

#------------------------------------------------------
# OPTIONS
#
# Note that if you set the options via the command line, they will be
# remembered in the file 'options.cache'. It's a feature ;-)

opts = Options(['options.cache', 'config.py'])
print "PLATFORM = ",platform.system()

# Import the outside environment
env = Environment(ENV=os.environ)

# Package linking option
opts.Add(EnumOption(
	'PACKAGE_LINKING'
	, 'Style of linking for external libraries'
	, 'DYNAMIC_PACKAGES'
    , ['DYNAMIC_PACKAGES', 'STATIC_PACKAGES', 'NO_PACKAGES']
))

# You can turn off building of Tcl/Tk interface
opts.Add(BoolOption(
	'WITHOUT_TCLTK_GUI'
	,"Set to True if you don't want to build the original Tcl/Tk GUI."
	, False
))

# You can turn off the building of the Python interface
opts.Add(BoolOption(
	'WITHOUT_PYTHON'
	,"Set to True if you don't want to build Python wrappers."
	, False
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
	,os.path.expanduser("~/src/ascend/trunk/models")
)

# Where is SWIG?
opts.Add(
	'SWIG'
	,"SWIG location, probably only required for MinGW and MSVC users."
		+" Enter the location as a Windows-style path, for example"
		+" 'c:\msys\1.0\home\john\swigwin-1.3.29\swig.exe'."
)

# Build the test suite?
opts.Add(BoolOption(
	'WITH_CUNIT_TESTS'
	,"Whether to build the CUnit tests. Default is off. If set to on,"
		+" you must have CUnit installed somewhere that SCons can"
		+" find it."
	,False
))

# Where are the CUnit includes?
opts.Add(PackageOption(
	'CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,"off"
))

# Where are the CUnit includes?
opts.Add(PackageOption(
	'CUNIT_LIBPATH'
	,"Where are your CUnit include files?"
	,"off"
))

# TODO: OTHER OPTIONS?

# TODO: flags for optimisation

# TODO: turning on/off bintoken functionality

# TODO: Where will the 'Makefile.bt' file be installed
# ....

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

with_tcltk_gui = (env['WITHOUT_TCLTK_GUI']==False)

with_python = (env['WITHOUT_PYTHON']==False)

with_cunit_tests = env['WITH_CUNIT_TESTS']

print "SOLVERS:",env['WITH_SOLVERS']

print "WITH_LOCAL_HELP:",env['WITH_LOCAL_HELP']
print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

subst_dict = {
	'@WEBHELPROOT@':'http://pye.dyndns.org/ascend/manual/'
	, '@GLADE_FILE@':'glade/ascend.glade'
	, '@DEFAULT_ASCENDLIBRARY@':env['DEFAULT_ASCENDLIBRARY']
	, '@ASCEND_ICON@':'glade/ascend.png'
	, '@HELP_ROOT@':''
}

if env['WITH_LOCAL_HELP']:
	subst_dict['@HELP_ROOT@']=env['WITH_LOCAL_HELP']
		
env.Append(SUBST_DICT=subst_dict)

#------------------------------------------------------
# SPECIAL CONFIGURATION TESTS

#----------------
# SWIG

import os,re

def CheckSwigVersion(context):
	context.Message("Checking version of SWIG")
	cmd = env['SWIG']+' -version'
	(cin,coutcerr) = os.popen4(cmd);
	output = coutcerr.read()
	
	restr = "SWIG\\s+Version\\s+(?P<maj>[0-9]+)\\.(?P<min>[0-9]+)\\.(?P<pat>[0-9]+)\\s*$"
	expr = re.compile(restr,re.M);
	m = expr.search(output);
	if not m:
		context.Result("error running SWIG or detecting SWIG version")
		return 0
	maj = int(m.group('maj'))
	min = int(m.group('min'))
	pat = int(m.group('pat'))
	
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


def CheckExtLib(context,libname,text,ext='.c',varprefix=None):
	"""This method will check for variables LIBNAME_LIBPATH
	and LIBNAME_CPPPATH and try to compile and link the 
	file with the provided text, linking with the 
	library libname."""

	context.Message( 'Checking for '+libname+'...' )
	
	if varprefix==None:
		varprefix = libname.upper()
	
	keep = {}
	for k in ['LIBS','LIBPATH','CPPPATH']:
		if context.env.has_key(k):
			keep[k] = context.env[k]
	
	libpath_add = []
	if context.env.has_key(varprefix+'_LIBPATH'):
		libpath_add = [env[varprefix+'_LIBPATH']]

	cpppath_add = []
	if context.env.has_key(varprefix+'_CPPPATH'):
		cpppath_add = [env[varprefix+'_CPPPATH']]
	
	context.env.Append(
		LIBS = libname
		, LIBPATH = libpath_add
		, CPPPATH = cpppath_add
	)
	ret = context.TryLink(cunit_test_text,ext)

	for k in keep:
		context.env[k]=keep[k];

	context.Result( ret )
	return ret

cunit_test_text = """
#include <CUnit/Cunit.h>
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
}
"""

def CheckCUnit(context):
	return CheckExtLib(context
		,'cunit'
		,cunit_test_text
	)

		
#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
		'CheckSwigVersion' : CheckSwigVersion
		, 'CheckCUnit' : CheckCUnit
#		, 'CheckIsNan' : CheckIsNan
#		, 'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
	, config_h = "config.h"
)

if not conf.CheckSwigVersion():
	print 'SWIG version is not OK'
	Exit(1)

# Math library
if not conf.CheckLibWithHeader(['m','c','libc'], 'math.h', 'C'):
	print 'Did not find libm.a or m.lib, exiting!'
	Exit(1)

# Where is 'isnan'?

if not conf.CheckFunc('isnan'):
	print "Didn't find isnan"
	Exit(1)

# Tcl/Tk
if not conf.CheckHeader('tcl.h'):
	with_tcltk_gui = False

if not conf.CheckHeader('tk.h'):
	with_tcltk_gui = False

if not conf.CheckLib('tcl'):
	with_tcltk_gui = False

if not conf.CheckLib('tk'):
	with_tcktk_gui = False

# Python... obviously we're already running python, so we just need to
# check that we can link to the python library OK:

if platform.system()=="Windows":
	#conf.env.Append(LIBPATH='c:\Python24\libs')
	#conf.env.Append(CPPPATH='c:\Python24\include')
	#python_header='Python.h'
	python_lib='python24'
	#python_libpath=['c:\\Python24\\libs']
	#python_cpppath=['c:\\Python24\\include']
else:
	#python_header='python2.4/Python.h'
	python_lib='python2.4'
	#python_libpath=[]
	#python_cpppath=['/usr/include/python2.4']

#if not conf.CheckLibWithHeader(python_lib,python_header,'C'
#		, LIBPATH=[distutils.sysconfig.PREFIX+"/libs"]
#		, CPPPATH=[distutils.sysconfig.get_python_inc()]
#):
#	print "Didn't find Python 2.4 ("+python_lib+")"
#	with_python = False
#else:

# SWIG version

if platform.system()=="Windows":
	#env['SWIG']=['c:\\msys\\1.0\\home\\john\\swigwin-1.3.29\\swig.exe']
	env['ENV']['SWIGFEATURES']='-O'
else:
	env['ENV']['SWIGFEATURES']='-O'	

# CUnit

if with_cunit_tests:
	conf.CheckCUnit()

# TODO: -D_HPUX_SOURCE is needed

# TODO: check size of void*

# TODO: detect if dynamic libraries are possible or not

conf.Finish()

env.Append(PYTHON_LIBPATH=[distutils.sysconfig.PREFIX+"/libs"])
env.Append(PYTHON_LIB=[python_lib])
env.Append(PYTHON_CPPPATH=[distutils.sysconfig.get_python_inc()])
print "PYTHON_LIBPATH =",env['PYTHON_LIBPATH']
print "PYTHON_CPPPATH =",env['PYTHON_CPPPATH']

if not with_python:
	print "Can't build python interface"
	Exit(1)

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
# SUBDIRECTORIES....


env.Append(CPPPATH=['..'])

env.SConscript(['base/generic/general/SConscript'],'env')

env.SConscript(['base/generic/utilities/SConscript'],'env')

env.SConscript(['base/generic/compiler/SConscript'],'env')

env.SConscript(['base/generic/solver/SConscript'],'env')

env.SConscript(['base/generic/packages/SConscript'],'env')

if with_tcltk_gui:
	env.SConscript(['tcltk98/generic/interface/SConscript'],'env')
else:
	print "Skipping... Tcl/Tk GUI isn't being built"

if with_python:
	env.SConscript(['pygtk/interface/SConscript'],'env')
else:
	print "Skipping... Python GUI isn't being built"

if with_cunit_tests:
	testdirs = ['general','solver','utilities']
	for testdir in testdirs:
		path = 'base/generic/'+testdir+'/test/'
		env.SConscript([path+'SConscript'],'env')
	env.SConscript(['test/SConscript'],'env')
	env.SConscript(['base/generic/test/SConscript'],'env')
	
	
else:
	print "Skipping... CUnit tests aren't being built"

#------------------------------------------------------
# INSTALLATION

# TODO: add install options
