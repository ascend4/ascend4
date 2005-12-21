#!/usr/bin/python
import sys

import dl
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)

import ascend

import gtkbrowser

print "python: creating new library object\n";

#----errror callback function-----
def error_reporter(sev,filename,line,msg):
	if(sev==1):
		leftstr = ""
		rightstr = "\n"
		typestr = "Note: "
	elif(sev==2):
		leftstr = chr(27)+"[33;2m"
		rightstr = chr(27)+"[0m\n"
		typestr = "Warning: "
	elif(sev==3):
		leftstr = chr(27)+"[31;1m"
		rightstr = chr(27)+"[0m\n"
		typestr = "Error: "
	elif(sev==4):
		# this is the default case, so keep it quiet:
		leftstr = chr(27)+"[33;2m"
		rightstr = chr(27)+"[0m"
		typestr = ""
	elif(sev==5):
		leftstr = chr(27)+"[33;2m"
		rightstr = chr(27)+"[0m\n"
		typestr = "PROGRAM WARNING: "
	elif(sev==6):
		leftstr = chr(27)+"[31;1m"
		rightstr = chr(27)+"[0m\n"
		typestr = "PROGRAM ERROR: "
	else:
		typestr = "";
		leftstr = rightstr = ""
	
	if(filename):
		outputstr = "%s%s:%d: %s%s\n" % (leftstr,filename,line,msg.strip(),rightstr)
	else:
		outputstr = "%s%s%s%s" % (leftstr,typestr,msg,rightstr)
	
	sys.stderr.write(outputstr)
	return len(outputstr)

#---------output model hierarchy---------------
def show(i,level=0):
	sys.stderr.write(("    "*level)+i.getName().toString()+" IS_A "+i.getType().getName().toString())

	if i.isCompound():
		if i.isChildless():
			sys.stderr.write(": no children)\n")
		else:
			sys.stderr.write(":\n");
			for c in i.getChildren():
				show(c,level+1)
	elif i.isRelation() or i.isWhen():
		sys.stderr.write("\n")
	elif i.isSet():
		if i.isSetInt():
			set = i.getSetIntValue()
		elif i.isSetString():
			set = i.getSetStringValue()
		#sys.stderr.write("[%d]:" % set.length())
		sys.stderr.write(" = %s\n" % set);

	elif ( i.isAtom() or i.isFund() ) and not i.isDefined():
		sys.stderr.write(" (undefined)\n")
	elif i.isBool():
		sys.stderr.write(" = "); sys.stderr.write("%s" % i.getBoolValue()); sys.stderr.write("\n")
	elif i.isInt():
		sys.stderr.write(" = "); sys.stderr.write("%d" % i.getIntValue()); sys.stderr.write("\n")
	else:
		if i.getType().isRefinedSolverVar():
			if i.isFixed():
				sys.stderr.write(" = "+chr(27)+"[1;43;37m"); sys.stderr.write("%f" % i.getRealValue()); sys.stderr.write(chr(27)+"[0m\n")
			else:
				sys.stderr.write(" = "+chr(27)+"[1m"); sys.stderr.write("%f" % i.getRealValue()); sys.stderr.write(chr(27)+"[0m\n")
		else:
			sys.stderr.write(" = "); sys.stderr.write("%f" % i.getRealValue()); sys.stderr.write("\n")
#-------------------------------

reporter = ascend.getReporter()
reporter.setPythonErrorCallback(error_reporter)

#reporter.reportError("STUFF")

l = ascend.Library()

l.listModules()

#t = l.findType("boolean")

l.load("simple_fs.a4c")

mv = l.getModules()
m = mv[0]
tv = l.getModuleTypes(m)

#for m in mv:
#	print "Module ", m.getName()
#	for t in l.getModuleTypes(m):
#		print " - Type ", t.getName()

#l.listModules();

#t = l.findType("boolean_var")
#t = l.findType("boolean")

t = l.findType("test_flowsheet")
# t = l.findType("my_water3")
sim = t.getSimulation("i")

sim.check()

print "Simulation instance kind:", sim.getKindStr()

pv = t.getMethods()
p_specify = 0
print "Listing methods of",t.getName,":"
for p in pv:
	print " *", p.getName()
	if p.getName()=="specify":
		p_specify = p

print "Running '"+p_specify.getName()+"'"
print p_specify
sim.run(p_specify)

sim.build()

sim.check()

print sim.getFixableVariables()

sim.solve()

ch = sim.getModel().getChildren()

print "Children of",sim.getName(),":"
print ch

print "Children of",ch[1].getName(),":"
print ch[1].getChildren()


show(sim.getModel())

b = gtkbrowser.Browser(sim)
gtkbrowser.gtk.main()

print "COMPLETED"

del l
