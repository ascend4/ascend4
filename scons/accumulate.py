# AccumulateBuilder
#
# Based on code from http://www.scons.org/wiki/AccumulateBuilder
#
# Converted to SCons 'tool' format by John Pye, 10 Sept 2007.

import os, os.path, shutil

def packagetree(src,dest, symlinks=False):
	"""Recursive copy of files listed in a local file named PACKAGE
	
	Look in the current directory for a file named PACKAGE. If found,
	copy the files listed there.

	Then recurse into subdirectories.

	Behaviour is intended to facilitate packaging of a user-contributed
	code library, where many files are 'experimental' but some files are
	tested and intended to distribution. Suitable for software plugins,
	code examples, documentation fragments, etc.
	"""

	files = os.listdir(src)
	if 'PACKAGE' in files:
		plistfile = os.path.join(src,'PACKAGE')
		plist = file(plistfile)
		pfiles = []
		for line in plist:
			l = line.strip()
			if l[0]=="#":
				continue
			if l in files:
				print "Package '%s'" % l
				pfiles.append(l)
									
	
def copytree(src, dest, symlinks=False):
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
			use_package_list = True

		if use_package_list:
			plistfile = os.path.join(src,'PACKAGE')
			plist = file(plistfile)
			for line in plist:
				if line[0]=="#":
					continue
				pfiles
				...

		for item in files:
			srcPath = os.path.join(src, item)
			if os.path.isdir(srcPath):
				srcBasename = os.path.basename(srcPath)
				destDirPath = os.path.join(dest, srcBasename)
				if not os.path.exists(destDirPath):
					os.makedirs(destDirPath)
				copyItems(srcPath, destDirPath
					, use_package_list=use_package_list
				)
			elif os.path.islink(item) and symlinks:
				linkto = os.readlink(item)
				os.symlink(linkto, dest)
			else:
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
  if not os.path.exists(destDir):
      os.makedirs(destDir)
  for s in source:
      s = str(s)
      if os.path.isdir(s):
          myShutil.copytree(s, destDir, symlinks = False)
      else:
          shutil.copy2(s, destDir)


##
## Zipper.py
##
import distutils.archive_util

def zipperFunction(target, source, env):
        """Function to use as an action which creates a ZIP file from the arguments"""
        targetName = str(target[0])
        sourceDir = str(source[0])
        distutils.archive_util.make_archive(targetName, 'zip', sourceDir)


##
## register the above builders...
##

def generate(env):

	# add builder to accumulate files
	accuBuilder = env.Builder(action=AccumulatorAction.accumulatorFunction,
	    source_factory=SCons.Node.FS.default_fs.Entry,
	    target_factory=SCons.Node.FS.default_fs.Entry,
	    multi=1)
	env['BUILDERS']['Accumulate'] = accuBuilder

	# add builder to zip files
	zipBuilder = env.Builder(action=Zipper.zipperFunction,
	   source_factory=SCons.Node.FS.default_fs.Entry,
	   target_factory=SCons.Node.FS.default_fs.Entry,
	   multi=0)
	env['BUILDERS']['Zipper'] = zipBuilder

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

