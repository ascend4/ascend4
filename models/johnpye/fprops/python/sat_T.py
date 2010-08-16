from fprops import *
from pylab import *

D = helmholtz_data_carbondioxide;
T = D.T_c - 0.0001

res, p1, rf1, rg1 = fprops_sat_T(T,D)

if res:
	print "ERROR in calculation of psat(T)"
	print "res = %d" % res

print "psat(T = %f) = %.12e bar" % (T, p1/1e5)

print "rhof = %f, rhog = %f" % (rf1, rg1);
