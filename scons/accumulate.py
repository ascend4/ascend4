# Accumulate
#
# Based on code from http://www.scons.org/wiki/AccumulateBuilder

# The goal here was to be able to distribute a source code tarball that
# contains all the required source code to build our program, plus a LIMITED
# subset of our test/sample code, because we have many sample files that are
# not yet ready for distribution. We wanted to manage our list of distributable
# files using a simple plain text-file system that would be informative to 
# end users and not tied to a particular build system (SCons).

# This tool faciltates creation of tarballs with some files excluded using
# a convenient text-file based mechanism. By default, the tool collects copies
# of the files you list using the syntax below, and places them in your
# 'destination' directory:
#
#   env.Accumulate('dist/temp/src',"src")
#   env.Accumulate('dist/temp',['prog1.exe','prog2.exe']) 
#
# There is a special exception however. If, while recursing into directories
# listed in the Accumulate source list, a file named 'PACKAGE' is found, then
# only files listed therein will be copied to the destination directory.
# The 'PACKAGE' file can contain comment lines; they must begin with a '#'.
# Once a directory has been found to contain a 'PACKAGE' file, subdirectories
# of that directory will not be recursed into unless they also contain a 
# 'PACKAGE' file.

# Converted to SCons 'tool' format by John Pye, 10 Sept 2007.

import os, os.path, shutil
import SCons.Node.FS

def copypackaged(src,dest, symlinks=False):
	"""Recursive copy of files listed in a local file named PACKAGE
	
	Look in the current directory for a file named PACKAGE. If found,
	copy the files listed there.

	Then recurse into subdirectories.

	Behaviour is intended to facilitate packaging of a user-contributed
	code library, where many files are 'experimental' but some files are
	tested and intended to distribution. Suitable for software plugins,
	code examples, documentation fragments, etc.
	"""

	print "Entering directory '%s'" % src

	files = os.listdir(src)

	pfiles = []
	if 'PACKAGE' in files:
		plistfile = os.path.join(src,'PACKAGE')
		plist = file(plistfile)
		for line in plist:
			l = line.strip()
			if not len(l) or l[0]=="#":
				continue
			if l in files:
				if os.path.isdir(os.path.join(src,l)):
					print "Package file '%s' ignored (is a directory)" % l
					continue
				print "Package '%s'" % l
				pfiles.append(l)
			else:
				print "Not found: '%s'" % l

	# copy any listed files in the current directory first		
	if pfiles:
		if not os.path.exists(dest):
			os.makedirs(dest)
		for f in pfiles:
			srcPath = os.path.join(src, f)
			if os.path.islink(srcPath) and symlinks:
				linkto = os.readlink(f)
				os.symlink(linkto, dest)
			else:
				shutil.copy2(srcPath, dest)		
									
	# now recurse into subdirectories
	for f in files:
		srcPath = os.path.join(src, f)
		if os.path.isdir(srcPath):
			# a directory must be recursed into, but defer creating the dir
			srcBasename = os.path.basename(srcPath)
			destDirPath = os.path.join(dest, srcBasename)
			copypackaged(srcPath, destDirPath, symlinks)

	print "Leaving directory '%s'" % src

	
def my_copytree(src, dest, env, symlinks=False):
	"""My own copyTree which does not fail if the directory exists.

	Recursively copy a directory tree using copy2().

	If the optional symlinks flag is true, symbolic links in the
	source tree result in symbolic links in the destination tree; if
	it is false, the contents of the files pointed to by symbolic
	links are copied.

	Behavior is meant to be identical to GNU 'cp -R'.    
	"""
	def copyItems(src, dest, symlinks=False):
		"""Function that does all the work.

		It is necessary to handle the two 'cp' cases:
		- destination does exist
		- destination does not exist

		See 'cp -R' documentation for more details
		"""
		files = os.listdir(src)
		if 'PACKAGE' in files:
			copypackaged(src,dest,symlinks)
			return

		for item in files:
			srcPath = os.path.join(src, item)
			if os.path.isdir(srcPath):
				print "DIR = %s" % srcPath
				srcBasename = os.path.basename(srcPath)
				destDirPath = os.path.join(dest, srcBasename)
				if not os.path.exists(destDirPath):
					#print "CREATE DIR %s" % destDirPath
					os.makedirs(destDirPath)
				#print "RECURSE INTO %s" % srcPath
				copyItems(srcPath, destDirPath
					, symlinks
				)
			elif os.path.islink(item) and symlinks:
				print "LINK = %s" % srcPath
				linkto = os.readlink(item)
				os.symlink(linkto, dest)
			else:
				print "FILE = %s" % srcPath
				shutil.copy2(srcPath, dest)		

	# case 'cp -R src/ dest/' where dest/ already exists
	if os.path.exists(dest):
	    destPath = os.path.join(dest, os.path.basename(src))
	    if not os.path.exists(destPath):
	        os.makedirs(destPath)
	# case 'cp -R src/ dest/' where dest/ does not exist
	else:
	    os.makedirs(dest)
	    destPath = dest
	# actually copy the files
	copyItems(src, destPath)


##
## AccumulatorAction.py
##

def accumulatorFunction(target, source, env):
	"""Function called when builder is called"""
	destDir = str(target[0])
	#print "DEST DIR = %s" % destDir
	if not os.path.exists(destDir):
		#print "CREATE DIR %s" % destDir
		os.makedirs(destDir)
	#print "SOURCES: %s" % source
	for s in source:
		s = str(s)
		if os.path.isdir(s):
			#print "COPYTREE from source %s" % s
			my_copytree(s, destDir, env, symlinks = False)
		else:
			#print "COPY FILE from source %s" % s
			shutil.copy2(s, destDir)

##
## register the above builders...
##

def generate(env):

	# add builder to accumulate files
	accuBuilder = env.Builder(action=accumulatorFunction,
	    source_factory=SCons.Node.FS.default_fs.Entry,
	    target_factory=SCons.Node.FS.default_fs.Entry,
	    multi=1)
	env['BUILDERS']['Accumulate'] = accuBuilder

def exists(env):
	"""
	Make sure this tool exists.
	"""
	try:
		import distutils.archive_util
		import shutil
		import os, os.path
	except ImportError:
		return False
	else:
		return True

