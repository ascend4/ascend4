#!/usr/bin/python
import sys, dl, re
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

import ascend
L = ascend.Library()

L.load("extfntest.a4c")
t = L.findType("test_extfntest")
sim = t.getSimulation("S")

print chr(27)+"[31;1mEXTERNAL FUNCTIONS:"+chr(27)+"[0m"
ff = L.getExtFns()
for f in ff:
	fn = f.getName()

	fh = f.getHelp()
	if not fh:
		fh = '[no help]'
	else:
		mlre = re.compile("\\n")
		fh = re.sub(mlre,"\n    ",fh)

	print chr(27)+"[31;1m"+fn+chr(27)+"[31;2m: "+fh+chr(27)+"[0m"


sim.build()
