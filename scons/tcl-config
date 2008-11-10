#! /usr/bin/env python

tclconfigfile = None

import sys, platform, os.path
if platform.system()=="Windows":
	# check for ActiveState Tcl in Windows registry
	try:
		import _winreg
		x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
		y= _winreg.OpenKey(x,r"SOFTWARE\ActiveState\ActiveTcl")
		_regversion,t = _winreg.QueryValueEx(y,"CurrentVersion")
		z= _winreg.OpenKey(x,r"SOFTWARE\ActiveState\ActiveTcl\%s" % str(_regversion))
		_regpath,t = _winreg.QueryValueEx(z,None)
		_winreg.CloseKey(y)
		_winreg.CloseKey(z)
		_winreg.CloseKey(x)
		# typically, c:\Tcl\lib\tclConfig.sh.
		_regconfig = os.path.join(_regpath,os.path.join("lib","tclConfig.sh"))
		if os.path.exists(_regconfig):
			# if the file exists, good...
			tclconfigfile = _regconfig
	except:
		pass

if tclconfigfile is None:
	# use a 'tclsh' script to find location of tclConfig.sh
	tclscript = """
	 foreach d [concat \
		  [list $tcl_library \
		        [lindex $tcl_pkgPath 0]] \
		  $auto_path \
		  [list [file dirname $tcl_library] \
		        [file dirname [lindex $tcl_pkgPath 0]] \
		        [file dirname [file dirname $tcl_library]] \
		        [file dirname [file dirname [lindex $tcl_pkgPath 0]]] \
		        [file dirname [file dirname [file dirname $tcl_library]]] \
		        [file dirname [file dirname [file dirname [lindex $tcl_pkgPath 0]]]]] \
	 ] {
		 if {[file exists [file join $d tclConfig.sh]]} {
		     puts "[file join $d tclConfig.sh]"
		     exit
		 }
	 }
	"""

	import subprocess
	output = subprocess.Popen(["tclsh"], stdin=subprocess.PIPE, stdout=subprocess.PIPE).communicate(input=tclscript)[0]
	if os.path.exists(output.strip()):
		# only report the file if it actually exists...
		tclconfigfile = output.strip()

if tclconfigfile is None:
	print "Unable to locate tclConfig.sh"
	sys.exit(1)

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
	#print "MATCHING",s
	if m:
		#print "MATCH!"
		if d.has_key(m.group(1)):
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

	d[k] = expand(v,d)

# output the variable that the user requests

import getopt, sys

def usage(progname):
	print "%s [--cflags] [--libs] [--var=TCL_VAR_NAME] [--vars]" % progname
	print "Output configuration variables for the Tcl script interpreter."
	print "Options:"
	print "\t--cflags           Compiler flags for C code that uses Tcl"
	print "\t--libs             Linker flags for code that uses Tcl"
	print "\t--vars             List all variables defined in tclConfig.sh"
	print "\t--var=VARNAME      Output the value of a specific variable"
	print "\nSee http://ascendwiki.cheme.cmu.edu/Tcl-config for more info."

try:
    opts, args = getopt.getopt(sys.argv[1:], "h",
		["help", "cflags", "libs","var=","vars"]
	)
except getopt.GetoptError, err:
    print str(err)
    usage(sys.argv[0])
    sys.exit(2)

for o, a in opts:
	if o == "-h" or o == "--help":
		usage(sys.argv[0])
		sys.exit()
	elif o == "--cflags":
		print d['TCL_INCLUDE_SPEC']
	elif o == "--libs":
		print d['TCL_LIB_SPEC']
	elif o == "--var":
		if d.has_key(a):
			print d[a]
		else:
			raise RuntimeError("Unknown variable '%s'" % a)
	elif o == "--vars":
		for k in sorted(d.keys()):
			print k
	else:
		assert False, "unhandled option"



