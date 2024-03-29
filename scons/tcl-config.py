#! /usr/bin/env python


import sys, os.path, platform

def find_tclConfigsh(tclsh = None):
	"""
	Do platform-specific tricks to located the tclConfig.sh file. On Windows, we
	assume the ActiveState distribution is  used, and hunt for its registry keys to
	locate the installed files. On other platforms, we use the program 'tclsh' to
	locate the file, and allow the path to tclsh to be specified, in order to allow
	the tcl/tk version to be selected if multiple versions are present.
	"""
	tclconfigfile = None

	if sys.platform.startswith("win") and tclsh is not None:
		# check for ActiveState Tcl in Windows registry
		try:
			import winreg
			x=winreg.ConnectRegistry(None,winreg.HKEY_LOCAL_MACHINE)
			y= winreg.OpenKey(x,r"SOFTWARE\ActiveState\ActiveTcl")
			_regversion,t = winreg.QueryValueEx(y,"CurrentVersion")
			z= winreg.OpenKey(x,r"SOFTWARE\ActiveState\ActiveTcl\%s" % str(_regversion))
			_regpath,t = winreg.QueryValueEx(z,None)
			winreg.CloseKey(y)
			winreg.CloseKey(z)
			winreg.CloseKey(x)
			# typically, c:\Tcl\lib\tclConfig.sh.
			_regconfig = os.path.join(_regpath,os.path.join("lib","tclConfig.sh"))
			if os.path.exists(_regconfig):
				# if the file exists, good...
				tclconfigfile = _regconfig
		except:
			pass

	if tclsh is None:
		tclsh = "tclsh"

	if tclconfigfile is None:
		import subprocess
		# first check that tclsh can be run
		tclshcheck = """exit 0"""
		res = 1
		try:
			p = subprocess.Popen([tclsh],stdin=subprocess.PIPE, stdout=subprocess.PIPE)
			p.communicate(input=tclshcheck)
			res = p.returncode
			assert res == 0
		except Exception as e:
			if tclsh != "tclsh":
				print("Unable to locate 'tclsh' in PATH. Suspect tcl/tk not installed, or PATH not correctly set. (%s)" % str(e), file=sys.stderr)
			else:
				print("The specifed tclsh, '%s' appears to be unrunnable. Check your Tcl/Tk installation and path.", file=sys.stderr)
			sys.exit(1)

		# use a 'tclsh' script to find location of tclConfig.sh
		# location of tclConfig.sh is canonical tcl_pkgPath
		tclscript = """
			#puts stderr "searching for tclConfig.sh ..."
			foreach d [concat \
			  [concat $tcl_library $tcl_pkgPath ] \
			  $auto_path \
			  [list [file dirname $tcl_library] \
				    [file dirname [lindex $tcl_pkgPath 0]] \
				    [file dirname [file dirname $tcl_library]] \
				    [file dirname [file dirname [lindex $tcl_pkgPath 0]]] \
				    [file dirname [file dirname [file dirname $tcl_library]]] \
				    [file dirname [file dirname [file dirname [lindex $tcl_pkgPath 0]]]]\
			   ] \
			] {
			  if {[file exists [file join $d tclConfig.sh]]} {
				puts "[file join $d tclConfig.sh]"
				#puts stderr "found $d : [file join $d tclConfig.sh]"
				exit 1
			  } else {
				#puts stderr "not in $d"
			  }
			}
		"""

		output = subprocess.Popen([tclsh], stdin=subprocess.PIPE, stdout=subprocess.PIPE).communicate(input=tclscript)[0]
		# print output
		# print output.strip()
		    # tcl will not return it if not there; already checked
		if os.path.exists(output.strip()):
			# print "path exists"
			# only report the file if it actually exists...
			tclconfigfile = output.strip()
			# print tclconfigfile

		# print tclconfigfile
		if tclconfigfile is None:
			print("Unable to locate tclConfig.sh.", file=sys.stderr)
			if platform.system()=="Linux":
				print("It is likely that you do not have tcl-devel or equivalent package installed on your system.", file=sys.stderr)
			else:
				print("On non-Linux platforms, the free ActiveTcl distribution for activestate.com is recommended.", file=sys.stderr)
			sys.exit(1)

		return tclconfigfile

def get_tcl_options(tclconfigfile):
	"""
	Given the tclConfig.sd file location, open the file and parse its options into
	a dictionary.
	"""

	# parse the file to determine the names of the variables it contains

	f = file(tclconfigfile)

	# default build environment variables that are assumed to already be defined
	# note that these are normally given default values in GNU Make, but we're
	# not inside make, so we might not have these.
	# FIXME: could try importing these from the environment?
	d = {
		"CC":"gcc"
		,"CFLAGS":""
		,"LDFLAGS":""
		,"AR":""
		,"LIBS":""
		,"VERSION":""
		,"DBGX":""
		,"NODOT_VERSION":"" # required for ActiveState Tcl 8.4 on Windows
	}

	# regular expression used for variable substitution
	import re
	r = re.compile(r'\$\{([A-Z_0-9]+)\}')

	# variable substitution/expansion function
	def expand(s,d):
		m = r.search(s)
		# print "MATCHING",s
		if m:
			# print "MATCH!"
			if m.group(1) in d:
				return expand(s[:m.start()] + d[m.group(1)] + s[m.end():],d)
			else:
				raise RuntimeError("Missing variable '%s'" % m.group(1))
		return s

	for l in f:
		ls = l.strip()
		if ls == "" or ls[0]=="#":
			continue
		k,v = ls.split("=",1)
		if len(v) >= 2 and v[0] == "'" and v[len(v)-1] == "'":
			v = v[1:len(v)-1]

		try:
			d[k] = expand(v,d)
		except RuntimeError as err:
			# print str(err),"probably unneeded"
			pass

	return d

# output the variable that the user requests

import getopt, sys

def usage(progname):
	print("%s [--cflags] [--libs] [--var=TCL_VAR_NAME] [--vars]" % progname)
	print("Output configuration variables for the Tcl script interpreter.")
	print("Options:")
	print("\t--cflags           Compiler flags for C code that uses Tcl")
	print("\t--libs             Linker flags for code that uses Tcl")
	print("\t--vars             List all variables defined in tclConfig.sh")
	print("\t--var=VARNAME      Output the value of a specific variable")
	print("\nSee http://ascendwiki.cheme.cmu.edu/Tcl-config for more info.")

try:
    opts, args = getopt.getopt(sys.argv[1:], "h",
		["help", "cflags", "libs", "var=","vars","tclsh="]
	)
except getopt.GetoptError as err:
    print(str(err))
    usage(sys.argv[0])
    sys.exit(2)

tclsh = None
for o, a in opts:
	if o == "--tclsh":
		tclsh = a

tclconfigfile = find_tclConfigsh(tclsh)
d = get_tcl_options(tclconfigfile)

for o, a in opts:
	if o == "-h" or o == "--help":
		usage(sys.argv[0])
		sys.exit()
	elif o == "--cflags":
		print(d['TCL_INCLUDE_SPEC'])
	elif o == "--libs":
		print(d['TCL_LIB_SPEC'])
	elif o == "--var":
		if a in d:
			print(d[a])
		else:
			raise RuntimeError("Unknown variable '%s'" % a)
	elif o == "--vars":
		for k in sorted(d.keys()):
			print(k)
	elif o == "--tclsh":
		pass
	else:
		assert False, "unhandled option"



