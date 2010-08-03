from fprops import *
from pylab import *

p = 1e5;

D = helmholtz_data_water;

res, T1, rf1, rg1 = fprops_sat_p(p,D)

print "res = %d" % res

print "Tsat(p = %f bar) = %f" % (p/1e5, T1)

print "rhof = %f, rhog = %f" % (rf1, rg1);
