import os, commands, platform

#------------------------------------------------------
# OPTIONS

env = Environment(ENV=os.environ)
opts = Options(['options.cache', 'config.py'])
print "PLATFORM = ",platform.system()

# TODO: add install options

# TODO: OTHER OPTIONS?

# TODO: flags for optimisation

# Package linking option
opts.Add(EnumOption(
	'PACKAGE_LINKING'
	, 'Style of linking for external libraries'
	, 'DYNAMIC_PACKAGES'
    , ['DYNAMIC_PACKAGES', 'STATIC_PACKAGES', 'NO_PACKAGES']
))

opts.Add(BoolOption(
	'WITHOUT_TCLTK_GUI'
	,"Set to True if you don't want to build the original Tcl/Tk GUI."
	, False
))

opts.Add(BoolOption(
	'WITHOUT_PYTHON'
	,"Set to True if you don't want to build Python wrappers."
	, False
))

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

with_tcltk_gui = (env['WITHOUT_TCLTK_GUI']==False)

with_python = (env['WITHOUT_PYTHON']==False)

#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
#		'CheckIsNan' : CheckIsNan
#		,'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
	, config_h = "config.h"
)

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

if platform.system()=="Windows":
	conf.env.Append(LIBPATH='c:\Python24\libs')
	conf.env.Append(CPPPATH='c:\Python24\include')
	python_header='Python.h'
	python_lib='python24'
	python_libpath='c:\\Python24\\libs'
else:
	python_header='python2.4/Python.h'
	python_lib='python2.4'
	python_libpath='/usr/include/python2.4'

# Python
if not conf.CheckLibWithHeader(python_lib,python_header,'C'):
	print "Didn't find Python 2.4 ("+python_lib+")"
	with_python = False
else:
	env.Append(PYTHON_LIBPATH=[python_libpath])
	env.Append(PYTHON_LIB=python_lib)

# TODO: -D_HPUX_SOURCE is needed

# TODO: check size of void*

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

