from fprops import *
from pylab import *


D = helmholtz_data_carbondioxide;
p = fprops_pc(D)
res, p, rhof, rhof = fprops_triple_point(D)

print "p = %.12e" % p

res, T1, rf1, rg1 = fprops_sat_p(p,D)

print "res = %d" % res

print "Tsat(p = %f bar) = %f" % (p/1e5, T1)

print "rhof = %f, rhog = %f" % (rf1, rg1);
