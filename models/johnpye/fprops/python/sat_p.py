from fprops import *
from pylab import *


D = helmholtz_data_hydrogen;
pc = fprops_pc(D)
res, pt, rhof, rhog = fprops_triple_point(D)

print "pt = %.12e" % pt
print "rhof_t = %f, rhog_t = %f" % (rhof, rhog);

p = pt;

print "p = %f" % p

res, T1, rf1, rg1 = fprops_sat_p(p,D)

print "res = %d" % res

print "Tsat(p = %f bar) = %f" % (p/1e5, T1)

print "rhof = %f, rhog = %f" % (rf1, rg1);
