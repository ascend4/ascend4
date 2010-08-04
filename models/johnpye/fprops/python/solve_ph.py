from fprops import *
from pylab import *

D = helmholtz_data_water;

p = 1e5;
h = 3000e3;
print "p = %f bar, h = %f kJ/kg" % (p/1e5, h/1e3)

print "p_raw(400,0.9) =", helmholtz_p_raw(400,0.9,D)

res, T, rho = fprops_solve_ph(p,h,0,D);

print "res = %d" % res

print "T = %f" % T
print "rho = %f" % rho

print "p(T,rho) = %f\t  (target: %f)" % (helmholtz_p(T,rho,D), p)
print "h(T,rho) = %f\t  (target: %f)" % (helmholtz_h(T,rho,D), h)

