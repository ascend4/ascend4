
ASCEND Tool for SCons
=====================

This folder contains a 'tool' suitable for use with the software build tool,
SCons. You can use it on Windows and Linux, and possibly Mac, to detect
where ASCEND is installed, and what its compile-time settings were.

If you use SCons for your own software project, this tool makes it easier to 
link to ASCEND and gives you the correct file location for installing
external libraries so that they will be automatically loadable in ASCEND models.

Suggested usage
---------------

The following gives a suggested 'SConstruct' file for your external library
project:

tools = ['ascend',...]
env = Environment(toolpath="#/scons",tools=['ascend'])
env['INSTALL_ROOT'] = "" # (for use when building RPM/DEB packages only)

if env['HAVE_ASCEND']:
	asclib_env = env.Clone()
	# add settings to #include from and link to libascend...
	asclib_env.Append(
		LIBS=env.get('ASCEND_LIBS')
		, LIBPATH=env.get('ASCEND_LIBPATH')
		, CPPPATH=env.get('ASCEND_CPPPATH')
		lib = libenv.SharedLibrary("freesteam",["freesteam_ascend.c"]
			,SHLIBSUFFIX = env['ASCEND_EXTLIB_SUFFIX']
			,SHLIBPREFIX = env['ASCEND_EXTLIB_PREFIX']		
		)

	)
	# build your ASCEND external library...
	lib = asclib_env.SharedLibrary("mylib",["mylib_ascend.c"]
		,SHLIBSUFFIX = env['ASCEND_EXTLIB_SUFFIX']
		,SHLIBPREFIX = env['ASCEND_EXTLIB_PREFIX']		
	)
	# install your shared library in the correct place...
	env.InstallLibrary("$INSTALL_ROOT$ASCEND_MODELS",[lib])

-- 
John Pye
Apr 2009.

