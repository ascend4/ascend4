
missing = file('missing.txt').read().strip().split('\n')
print "FIXING",len(missing),"SYMBOL DECLARATIONS"

import re
pattern = '^extern (.*)\\b('+"|".join(missing)+')\\b'
#pattern = '^extern (.*) (BitListEmpty)\('
patt = re.compile(pattern, re.M)


print "PATTERN =",pattern

import glob
import os.path

files = [os.path.normpath(p) for p in glob.glob("../../../base/generic/*/*.h")]
#files = ['testfile.h']

import fileinput

n=0
for f in files:
	s = file(f).read()
	if patt.search(s):
		print "MATCHED IN",f	
		s = patt.sub('ASC_DLLSPEC(\\1) \\2',s)
		n += 1
		fp = open(f,'w')
		fp.write(s)
		fp.close()

print "MATCHED",n,"FILES"
