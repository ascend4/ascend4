from fprops import *
from pylab import *
import sys

D = helmholtz_data_water;

T = 3.221585e+02
v = 1.000208e-03

rho = 1./v
print "T = %f, rho = %f" % (T,rho)
p = helmholtz_p(T,rho,D)
h = helmholtz_h(T,rho,D)
#p = 207.544081e5;
#h = 2722.928340e3
#p = 279.851966e5
#h = 1894.424202e3
#p = 1e5;
#h = 300e3;
print "p = %f bar, h = %f kJ/kg" % (p/1e5, h/1e3)

print "p_raw(400,0.9) =", helmholtz_p_raw(400,0.9,D)

res, T, rho = fprops_solve_ph(p,h,0,D);

print "res = %d" % res

print "T = %f" % T
print "rho = %f" % rho

p_eval = helmholtz_p(T,rho,D)
h_eval = helmholtz_h(T,rho,D)
print "p(T,rho) = %f bar\t  (target: %f, err = %e)" % (p_eval/1e5, p/1e5, (p_eval - p))
print "h(T,rho) = %f kJ/kg\t  (target: %f, err = %e)" % (h_eval/1e3, h/1e3, (h_eval - h))

