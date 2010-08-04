from fprops import *
from pylab import *

D = helmholtz_data_water;

p = 1e5;
h = 200e3;
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

