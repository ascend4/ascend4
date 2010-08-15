from fprops import *

hf=80400

D = helmholtz_data_carbondioxide;

print "hf = %.12e" % hf

res, Tsat, psat, rhof, rhog = fprops_sat_hf(hf,D);

if res:
	print "\nERROR %d from fprops_sat_hf(hf = %.12e)" % (res, hf)

print "Tsat =",Tsat
print "psat =",psat/1e5," bar"
print "rhof = %f, rhog = %f" %(rhof,rhog)

print "\nAt this point, hf = %.12e" % helmholtz_h_raw(Tsat,rhof,D)

exit(res)
