# SCons support for bundled GTK installation
# Written by John Pye, Nov 2009


"""
This tool installs the necessary files to allow GTK to run in a standalone
installation.

This approach is suitable for Mac (and maybe for Windows too) where we often
must assume that GTK+ is not separately installed on the system, or can't assume
that the version will be compatible.

Ultimately, it would be desirable for a Mac 'framework' to be utilised for this
purpose, but that option is not yet available.

Currently, this tool is a command-line thing that you must run after ASCEND
has been compiled, but before you run 'scons install'. It will copy all the
necessary files into dist/PyGTK.bundle; these files will then be copied by SCons
into their files (.dmg preparation folder) location.
"""

import os.path
import glob
import subprocess, sys, os.path, re, shutil
from distutils.dir_util import copy_tree

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

def reroute_deps(lib,gtksite,pysite):
	deps = find_deps(lib)
	rpath_used = False
	for d in deps:
		oldpath = d
		r = os.path.commonprefix([oldpath,gtksite])
		if r == gtksite:
			# this dependency is pointing into the GTK installation location
			dir, f = os.path.split(oldpath)
			newpath = "@loader_path/"+f
		else:
			r = os.path.commonprefix([oldpath,pysite])
			if r == pysite:
				# this dependency is pointing to a file installed inside Python
				newpath = "python/" + oldpath[len(r):]
			else:
				continue

		cmd = ['/usr/bin/install_name_tool','-change',oldpath,newpath,lib]
		#print "RELINK %s: %s --> %s" % (lib,oldpath,newpath)
		#print " ".join(cmd)
		P = subprocess.Popen(cmd,stdout=subprocess.PIPE)
		P.communicate()
		rpath_used = True

def find_and_replace(fname, pattern, replace):
	t = open(fname).read()
	t1 = pattern.sub(replace,t)
	f = open(fname,'w')
	f.write(t1)
	f.close()

if __name__ == "__main__":
	import modulefinder
	M = modulefinder.ModuleFinder()
	sys.path.append("..")
	script = os.path.normpath(os.path.join(sys.path[0],"../pygtk/gtkbrowser.py"))
	M.run_script(script)

	pysite="/Library/Python/2.5/site-packages/"
	if pysite[-1:]!="/":
		raise RuntimeError("missing trailing slash in pysite")

	gtksite = os.path.expanduser("~/gtk/inst/")
	if gtksite[-1:]!="/":
		raise RuntimeError("missing trailing slash in gtksite")

	imports = set()
	ignore_paths = ['/System/Library/Frameworks/Python.framework'
		,'/System/Library/Frameworks/Carbon.framework'
		,'/System/Library/Frameworks/Cocoa.framework'
		,'/usr/lib'
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

	print "\nAdd pixbuf loaders to list..."
	loaders = glob.glob(gtksite+"lib/gtk-2.0/*/loaders/libpixbufloader-*.so")
	for l in loaders:
		print l
		files.add(l)

	print "\n\nFinding all GTK libs in the list"
	gtkimports = set()
	for f in files:
		if os.path.commonprefix([gtksite,f]) == gtksite:
			gtkimports.add(f)
			print f
	
	print "\n\nFinding all Python site-packages stuff in the list"
	pyimports = set()
	for f in files:
		if os.path.commonprefix([pysite,f]) == pysite:
			pyimports.add(f)
			print f

	# define the paths into which we'll copy all the stuff
	distpath = os.path.normpath(os.path.join(sys.path[0],"../dist"))
	targetpath = os.path.join(distpath,"PyGTK.bundle")
	pytarget = os.path.join(targetpath,"python")
	gtktarget = targetpath
	gtklibtarget = os.path.join(gtktarget,"lib")

	print "\n\nCopying all Python files into our bundle"
	for f in pyfiles | pyimports:
		if not os.path.commonprefix([f,pysite]) == pysite:
			raise RuntimeError("Unknown python import in list")
		r = f[len(pysite):]
		dir, f1 = os.path.split(r)
		if not os.path.exists(os.path.join(pytarget,dir)):
			os.makedirs(os.path.join(pytarget,dir))
		print r
		dest = os.path.join(pytarget,r)
		shutil.copy(f, dest)
		if dest[-3:] == ".so":
			print "  rerouting dependencies..."
			reroute_deps(dest,gtksite,pysite)
	
	print "\n\nCopy all GTK libs into our bundle"
	if not os.path.exists(gtklibtarget):
		os.makedirs(gtklibtarget)
	for f in gtkimports:
		if not os.path.commonprefix([f, gtksite]) == gtksite:
			raise RuntimeError("Unknown gtk import in list")
		r = f[len(gtksite):]
		dir, f1 = os.path.split(r)
		dest = os.path.join(gtklibtarget, f1)
		print dest
		shutil.copy(f, dest)
		reroute_deps(dest,gtksite,pysite)

	print "\nCopying GTK related files"
	reldirs = ["etc/gtk-2.0", "etc/pango", "etc/fonts"]
	for d in reldirs:
		print "%s --> %s" % (os.path.join(gtksite,d),os.path.join(gtktarget,d))
		copy_tree(os.path.join(gtksite,d),os.path.join(gtktarget,d))
	
	print "\nFixing pixbuf loader paths"
	loaderre = re.compile("%slib/gtk-2\\.0/[^/]+/loaders/" % re.escape(gtksite))
	find_and_replace("%s/etc/gtk-2.0/gdk-pixbuf.loaders"%gtktarget,loaderre,"@loader_path/")

	print "\nFixing absolute path sin GTK etc files"
	etcfiles = ["%s/etc/%s" % (gtktarget, f) for f in 
		['gtk-2.0/gtk.immodules', 'pango/pango.modules'] #'pango/pangorc',
	]
	etcre = re.compile("%s"%re.escape(gtksite))
	for e in etcfiles:
		print e

	print "\nTODO: copy localisation files (not yet implemented)"
	#SHARE=`echo $PREFIX/share/locale/*/LC_MESSAGES/gimp*`





