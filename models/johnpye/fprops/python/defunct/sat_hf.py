from fprops import *

hf=80400

F = fluid("carbondioxide")

print "hf = %.12e" % hf

try:
	Tsat, psat, rhof, rhog = F.sat_hf(hf);
except Exception,e:
	print "\nERROR %s from fprops_sat_hf(hf = %.12e)" % (str(e), hf)
	exit(e)

print "Tsat =",Tsat
print "psat =",psat/1e5," bar"
print "rhof = %f, rhog = %f" %(rhof,rhog)

print "\nAt this point, hf = %.12e" % F.h(Tsat,rhof)

exit(res)
