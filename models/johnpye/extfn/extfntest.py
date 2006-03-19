#!/usr/bin/python
import platform
if platform.system() != "Windows":
	import sys, re, dl
	sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

import ascend
L = ascend.Library()

#--------------------------------
print "\n\n\n\nLOADING EXTERNAL TEST...\n\n\n"
L.load("extfntest.a4c")

#--------------------------------
#print "\n\n\n\nLOADING INTERNAL TEST...\n\n\n"
#L.load("intfntest.a4c")

#--------------------------------
print "\n\n\n\nINSTANTIATING TEST...\n\n\n"
t = L.findType("test_extfntest")
#t = L.findType("test_intfntest")
sim = t.getSimulation("S")

print "\n--------------------------\n"
raise RuntimeError("stop")
#--------------------------------
print "\n\n\n\nLISTING EXTERNAL METHODS...\n\n\n"
print chr(27)+"[31;1mEXTERNAL METHODS (!):"+chr(27)+"[0m"
ff = L.getExtMethods()
for f in ff:
	fn = f.getName()

	fh = f.getHelp()
	if not fh:
		fh = '[no help]'
	else:
		mlre = re.compile("\\n")
		fh = re.sub(mlre,"\n    ",fh)

	print chr(27)+"[31;1m"+fn+chr(27)+"[31;2m: "+fh+chr(27)+"[0m"

#--------------------------------
print "\n\n\n\nBUILDING...\n\n\n"
sim.build()
