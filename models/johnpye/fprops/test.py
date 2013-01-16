#!/usr/bin/env python
import subprocess, sys, os.path, platform

try:
	species = sys.argv[1]
except:
	print "Run './test.py speciesname' to run its embedded test suite"
	exit(1)

src = 'fluids/%s.c'%species
if not os.path.exists(src):
	print "No file named '%s' found in current directory" % src
	exit(1)


CC = "gcc"
if os.environ.get('HOST_PREFIX'):
	CC = "%s-%s" % (os.environ['HOST_PREFIX'],"gcc")

CFLAGS = "-g"
if os.environ.get('GCOV'):
	CFLAGS += " -fprofile-arcs -ftest-coverage"

# get the GSL libs...
# gslflags = "-lgsl -lgslcblas -lm"
gslflags = "-lm"
#if platform.system() == "Windows":
#	if platform.architecture()[0] == "64bit":
#		gslflags = "-LC:\\mingw64\\lib -lgsl -lgslcblas"
#	else:
#		gslflags = "-LC:\\mingw\\lib -lgsl -lgslcblas"

s = '%s %s color.c refstate.c ideal.c cp0.c helmholtz.c pengrob.c sat.c fprops.c zeroin.c test.c cubicroots.c fluids/%s.c -Wall -DTEST -o fluids/test-%s %s' % (CC,CFLAGS,species,species,gslflags)

print s
p = subprocess.Popen(s.split(' '))

p.wait()

if p.returncode:
	sys.exit(p.returncode)

s = 'fluids/test-%s' % species
if os.environ.get("GDB"):
	s = 'gdb --args %s' % s
print s
p = subprocess.Popen(s.split(' '))
p.wait()

if p.returncode:
	print "ERROR CODE %d" % p.returncode
sys.exit(p.returncode)


