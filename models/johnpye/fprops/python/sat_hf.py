from fprops import *

hf=2072.932419e3

D = helmholtz_data_water;

print "hf = %.12e" % hf

res, Tsat, psat, rhof, rhog = fprops_sat_hf(hf,D);

if res:
	print "ERROR %d in fprops_sat_hf(hf = %.12e" % res, hf

print "Tsat =",Tsat
print "psat =",psat/1e5," bar"
print "rhof = %f, rhog = %f" %(rhof,rhog)

