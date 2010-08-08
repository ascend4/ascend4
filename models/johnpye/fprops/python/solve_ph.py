from fprops import *
from pylab import *
import sys

D = helmholtz_data_water;

v = 1.005714e-03
T = 3.298951e+02
#rho = 1./v

#T = 3.092641655172e+02
#rho = 9.997925200316e+02
#T = 3.024735758794e+02
#rho = 9.973958154879e+02
#T = 6.727273e+02
#rho = 1./2.078561e-03
#T = 6.636364e+02
#rho = 1./2.609722e-03
#T = 7.242424242424e+02
#rho = 5.847226548308e+02
T = 4.484887272727e+02
rho = 2.765899020837e-03

print "T = %f, rho = %f" % (T,rho)
p = helmholtz_p(T,rho,D)
h = helmholtz_h(T,rho,D)
#p = 207.544081e5;
#h = 2722.928340e3
#p = 279.851966e5
#h = 1894.424202e3
#p = 1e5;
#h = 300e3;
sys.stderr.write("p = %f bar, h = %f kJ/kg\n" % (p/1e5, h/1e3))

#sys.stderr.write("p_raw(400,0.9) =", helmholtz_p_raw(400,0.9,D))

res, T, rho = fprops_solve_ph(p,h,0,D);

print "res = %d" % res

print "T = %f" % T
print "rho = %f" % rho

p_eval = helmholtz_p(T,rho,D)
h_eval = helmholtz_h(T,rho,D)
print "p(T,rho) = %f bar\t  (target: %f, err = %e)" % (p_eval/1e5, p/1e5, (p_eval - p))
print "h(T,rho) = %f kJ/kg\t  (target: %f, err = %e)" % (h_eval/1e3, h/1e3, (h_eval - h))

