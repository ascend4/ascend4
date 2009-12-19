# SCons support for bundled GTK installation
# Written by John Pye, Nov 2009


"""
This tool provides SCons support for installing the necessary files to allow
GTK to run in a standalone installation.

This approach is suitable for Windows and Mac where we often must assume that
GTK+ is not separately installed on the system, or can't assume that the version
will be compatible.

Ultimately, it would be desirable for a Mac 'framework' to be utilised for this
purpose, but that is not yet available.

To use it you must copy this file into the scons/SCons/Tools directory or use
the tooldir arg in the Tool function and put a line like 'env.Tool("InstallGTK")'
into your file. Then you can do 'env.InstallGTK(sourcebinary, targetdir)' which 
will read dependencies for sourcebinary and copy required libraries and
other files into targetdir.
"""

#import SCons.Builder
#import SCons.Util
#import SCons.Scanner
import os.path
import glob
import subprocess, sys, os.path, re, shutil

otool_re = re.compile(r"""\s+(.+)\s\(compatibility version (.*), current version (.*)\)""")

def find_deps(binary):
	deps = set()
	P = subprocess.Popen(['/usr/bin/otool','-L',binary],stdout=subprocess.PIPE)
	O = P.communicate()[0].strip()
	for o in O.split("\n")[1:]:
		m = otool_re.match(o)
		#print " -->",m.group(1)
		deps.add(m.group(1))
	return deps

def generate(env):
	pass

def exists(env):
	pass

if __name__ == "__main__":
	import modulefinder
	M = modulefinder.ModuleFinder()
	sys.path.append("..")
	script = os.path.normpath(os.path.join(sys.path[0],"../pygtk/gtkbrowser.py"))
	M.run_script(script)

	imports = set()
	ignore_paths = ['/System/Library/Frameworks/Python.framework'
		,'/System/Library/Frameworks/Carbon.framework'
		,'/System/Library/Frameworks/Cocoa.framework'
		,os.path.dirname(script)]
	for name, m in M.modules.items():
		if not m.__file__:
			continue
		ok = True
		for i in ignore_paths:
			if os.path.commonprefix([i,m.__file__]) == i:
				ok = False
		if ok is False:
			continue

		imports.add(m)
		print m
	
	print "\n\nChecking otool deps"

	files = set()
	pyfiles = set()
	for m in imports:
		files.add(m.__file__)

		if m.__file__[-3:] == ".so":
			print "FILE:",m.__file__
			files |= find_deps(m.__file__)

		elif m.__file__[-3:] == ".py":
			print "PY:",m.__file__
			pyfiles.add(m.__file__)
	
	print "\n\nOTOOL DEPS"
	for f in files:
		if f[-3:] != ".py":
			print f

	print "\n\nRemoving system libs from list"
	realimports = set()
	ignore_paths += ['/usr/lib']
	for f in files:
		ok = True
		for i in ignore_paths:
			if os.path.commonprefix([i,f]) == i:
				ok = False
		if ok is False:
			#print "REMOVE",f
			continue
		realimports.add(f)
		print f

	distpath = os.path.normpath(os.path.join(sys.path[0],"../dist"))
	if not os.path.exists(distpath):
		os.mkdir(distpath)
	targetpath=os.path.join(distpath,"gtk.bundle")
	if not os.path.exists(targetpath):
		os.mkdir(targetpath)

	pytarget=os.path.join(targetpath,"python")
	if not os.path.exists(pytarget):
		os.mkdir(pytarget)
	print "PYTHON TARGET =",pytarget


	print "\nCopying Python includes"
	pysite="/Library/Python/2.5/site-packages/"
	if pysite[-1:]!="/":
		raise RuntimeError("missing trailing slash in pysite")

	gtksite = os.path.expanduser("~/gtk/inst/")
	if gtksite[-1:]!="/":
		raise RuntimeError("missing trailing slash in gtksite")

	for f in pyfiles:
		if os.path.commonprefix([f,pysite]) == pysite:
			r = f[len(pysite):]
		else:
			raise RuntimeError("Unknown python file location '%s'" % f)	

		dir,f1 = os.path.split(r)
		if not os.path.exists(os.path.join(pytarget,dir)):
			print "Create directory '%s'"% dir
			os.makedirs(os.path.join(pytarget,dir))

		print "Copy %s --> %s" % (f, os.path.join(pytarget,r))
		shutil.copy(f,os.path.join(pytarget,r))

	print "\nCopying Python shared libraries"
	copied = set()
	for f in files:
		if os.path.commonprefix([f,pysite]) == pysite:
			r = f[len(pysite):]
		else:
			continue
		print "Copy %s --> %s" % (f, os.path.join(pytarget,r))

		dir,f1 = os.path.split(r)
		if not os.path.exists(os.path.join(pytarget,dir)):
			print "Create directory '%s'"% dir
			os.makedirs(os.path.join(pytarget,dir))

		shutil.copy(f,os.path.join(pytarget,r))
		copied.add(f)

	print "\nCopying GTK libraries"
	gtktarget = os.path.join(targetpath,"gtk")
	if not os.path.exists(gtktarget):
		os.mkdir(gtktarget)

	for f in files:
		if os.path.commonprefix([f,gtksite]) == gtksite:
			r = f[len(pysite):]
		else:
			continue

		if not os.path.exists(os.path.join(gtktarget,dir)):
			print "Create directory '%s'"% dir
			os.makedirs(os.path.join(gtktarget,dir))

		print "Copy %s --> %s" % (f, os.path.join(gtktarget,r))
		shutil.copy(f,os.path.join(gtktarget,r))
		copied.add(f)

	if copied != files:
		print "Some library files were not copied!"



#	ETC="$PREFIX/etc/gimp/ $PREFIX/etc/gtk-2.0/ $PREFIX/etc/pango/ $PREFIX/lib/gimp/2.0/python/*.py* $PREFIX/lib/gimp/2.0/environ $PREFIX/lib/gimp/2.0/interpreters $PREFIX/etc/fonts"

	# get the 'etc' files for GTK, Pango, fonts

	

#SHARE=`echo $PREFIX/share/gimp/ $PREFIX/share/locale/*/LC_MESSAGES/gimp*`

	# get the share files for 

