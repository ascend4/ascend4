# vim: set et sw=3 tw=0 fo=awqorc ft=python:
#
# Astxx, the Asterisk C++ API and Utility Library.
# Copyright (C) 2005, 2006  Matthew A. Nicholson
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

def DistTar(target, source, env):
	"""tar archive builder"""

	import tarfile
	import os

	env_dict = env.Dictionary()

	if env_dict.has_key("DISTTAR_FORMAT") and env_dict["DISTTAR_FORMAT"] in ["gz", "bz2"]:
		tar_format = env_dict["DISTTAR_FORMAT"]
	else:
		tar_format = ""

	# split the target directory, filename, and stuffix
	base_name = str(target[0]).split('.tar')[0]
	(target_dir, dir_name) = os.path.split(base_name)

	# create the target directory if it does not exist
	if target_dir and not os.path.exists(target_dir):
		os.makedirs(target_dir)

	# open our tar file for writing
	tar = tarfile.open(str(target[0]), "w:%s" % (tar_format,))

	# write sources to our tar file
	for item in source:
		item = str(item)
		print "Adding '%s/%s'" % (dir_name, item)
		tar.add(item, '%s/%s' % (dir_name, item))

	# all done
	tar.close()

def DistTarSuffix(env, sources):
	"""tar archive suffix generator"""

	env_dict = env.Dictionary()
	if env_dict.has_key("DISTTAR_FORMAT") and env_dict["DISTTAR_FORMAT"] in ["gz", "bz2"]:
		return ".tar." + env_dict["DISTTAR_FORMAT"]
	else:
		return ".tar"

def generate(env):
	"""
	Add builders and construction variables for the DistTar builder.
	"""
	env.Append(BUILDERS = {
	    'DistTar': env.Builder(
	        action = DistTar,
	        suffix = DistTarSuffix,
	        target_factory = env.fs.Entry,
	    ),
	})

	env.AppendUnique(
	    DISTTAR_FORMAT = 'gz',
	)

def exists(env):
	"""
	Make sure this tool exists.
	"""
	try:
		import os
		import tarfile
	except ImportError:
		return False
	else:
		return True
