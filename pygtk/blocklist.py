import ascpy

L = ascpy.Library()

L.load('johnpye/rankine.a4c')

D = L.getAnnotationDatabase()

M = L.getModules()

blocktypes = set()

for m in M:
	T = L.getModuleTypes(m)
	for t in T:
		N = D.getNotes(t)
		for n in N:
			i = str(n.getId())
			x = str(n.getText())
			print "%s [%s::%s] %s" % (n.getLanguage(), n.getType(), n.getId(), n.getText())
			if x[0:3] == "in:" or x[0:4] == "out:":
				blocktypes.add(t)

print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:
	print t.getName()

print "-----"

T = L.findType("pump_simple")
print D.getNoteForVariable(T,ascpy.SymChar("inlet"),ascpy.SymChar("inline"));

print "-----"
blocktypes = set()

for m in M:
	T = L.getModuleTypes(m)
	for t in T:
		# 'block' types are only those which are parametric.
		if not t.hasParameters():
			continue
		x = str(D.getNoteForVariable(t,ascpy.SymChar("inlet"),ascpy.SymChar("inline")))
		if x[0:max(3,len(x))] == "in:" or x[0:max(4,len(x))] == "out:":
			blocktypes.add(t)

print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:
	print t.getName()

