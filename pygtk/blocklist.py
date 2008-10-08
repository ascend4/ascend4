import ascpy

L = ascpy.Library()

L.load('johnpye/rankine.a4c')

D = L.getAnnotationDatabase()

M = L.getModules()

blocktypes = set()

# convenience class for 'block' types in ASCEND
class BlockType:
	def __init__(self,typ):
		self.typ = typ
		self.inlets = set()
		self.outlets = set()
		nn = D.getNotes(typ,ascpy.SymChar("inline"),None)
		for n in nn:
			pass

	def get_inlets(self):
		pass

for m in M:
	T = L.getModuleTypes(m)
	for t in T:
		# 'block' types must not be parametric, because they must be able to
		# exist even without being connected, and parametric models impose
		# restrictions on the use of ARE_THE_SAME and similar.
		if t.hasParameters():
			continue
		x = D.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
		if x:
			blocktypes.add(t)

print "block types:"
if not blocktypes:
	print "NONE FOUND"
for t in blocktypes:
	print t.getName()
	if str(t.getName())!="condenser_simple":
		continue

	nn = D.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
	for n in nn: 
		print "\t%s" % n.getText()
	nn = D.getTypeRefinedNotesLang(t,ascpy.SymChar("inline"))

	inputs = []
	outputs = []
	for n in nn:
		t = n.getText()
		if t[0:min(len(t),3)]=="in:":
			inputs += [n]
		elif t[0:min(len(t),4)]=="out:":
			outputs += [n]

	print "\t\tinputs:",[t.getId() for t in inputs]
	for t in inputs:
		print "\t\t\t%s: %s" % (t.getId(),t.getText())
	print "\t\toutputs:",[t.getId() for t in outputs]
	for t in outputs:
		print "\t\t\t%s: %s" % (t.getId(),t.getText())


