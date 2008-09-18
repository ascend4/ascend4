#!/usr/bin/env python
import subprocess, sys, os.path

try:
	species = sys.argv[1]
except:
	print "Run './precalc.py speciesname' to run the precalculation routine (if applicable)"
	exit(1)

src = '%s.c'%species
if not os.path.exists(src):
	print "No file named '%s' found in current directory" % src
	exit(1)

s = 'gcc precalc.c %s.c -DPRECALC -o precalc -lm' % (species)

print s
p = subprocess.Popen(s.split(' '))

p.wait()

if not p.returncode:
	print s
	p = subprocess.Popen(s.split(' '))
	


