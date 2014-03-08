#!/usr/bin/env python
import subprocess, sys, os.path, platform

try:
	name = sys.argv[1]
except:
	print "Run './test.py speciesname' or ./test.py testname' to run test code."
	exit(1)

dirn = 'fluids'
src = "%s/%s.c" %(dirn,name)
if not os.path.exists(src):
	print "No file named '%s.c' found in '%s' directory" % (name,dirn)
	exit(1)

CC = "gcc"
if os.environ.get('HOST_PREFIX'):
	CC = "%s-%s" % (os.environ['HOST_PREFIX'],"gcc")

CFLAGS = "-g"
if os.environ.get('GCOV'):
	CFLAGS += " -fprofile-arcs -ftest-coverage"


srcs = "color.c refstate.c ideal.c cp0.c helmholtz.c pengrob.c sat.c fprops.c zeroin.c test.c cubicroots.c visc.c"

ldflags = '-lm'


defs = ""
if dirn == "fluids":
	defs = "-DTEST"

s = '%s %s %s %s -Wall %s -o %s/test-%s %s' % (CC,CFLAGS,srcs,src,defs,dirn,name,ldflags)

print s
p = subprocess.Popen(s.split(' '))

p.wait()

if p.returncode:
	sys.exit(p.returncode)

s = '%s/test-%s' % (dirn,name)
if os.environ.get("GDB"):
	s = 'gdb --args %s' % s
print s
p = subprocess.Popen(s.split(' '))
p.wait()

if p.returncode:
	print "ERROR CODE %d" % p.returncode
sys.exit(p.returncode)


