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

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

with_tcltk_gui = (env['WITHOUT_TCLTK_GUI']==False)

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

# Where is tcl.h ?
if not conf.CheckHeader('tcl.h'):
	with_tcltk_gui = False

if not conf.CheckHeader('tk.h'):
	with_tcltk_gui = False

if not conf.CheckLib('tcl'):
	with_tcltk_gui = False

if not conf.CheckLib('tk'):
	with_tcktk_gui = False

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

