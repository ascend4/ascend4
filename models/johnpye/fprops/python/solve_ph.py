from fprops import *
from pylab import *

D = helmholtz_data_water;

p = 1e5;
h = 4000e3;
print "p = %f bar, h = %f kJ/kg" % (p/1e5, h/1e3)

res, T, rho = fprops_solve_ph(p,h,0,D);

print "res = %d" % res

print "T = %f" % T
print "rho = %f" % rho

print "p(T,rho) =", helmholtz_p(T,rho,D)
print "h(T,rho) =", helmholtz_h(T,rho,D)

