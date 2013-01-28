from fprops import *
from pylab import *
import sys

D = fluid('carbondioxide','pengrob');

T = 220
rho = 1

sys.stderr.write("T = %f K, rho = %f m3/kg\n" % (T,rho))
print "T = %f, rho = %f" % (T,rho)
p = D.set_Trho(T,rho).p
h = D.set_Trho(T,rho).h

sys.stderr.write("p = %f bar, h = %f kJ/kg\n" % (p/1e5, h/1e3))

#sys.stderr.write("p_raw(400,0.9) =", helmholtz_p_raw(400,0.9,D))

state = D.set_ph(p,h);

#print "res = %d" % res

print "T = %f" % state.T
print "rho = %f" % state.rho

p_eval = state.p
h_eval = state.h
print "p(T,rho) = %f bar\t  (target: %f, err = %e)" % (p_eval/1e5, p/1e5, (p_eval - p))
print "h(T,rho) = %f kJ/kg\t  (target: %f, err = %e)" % (h_eval/1e3, h/1e3, (h_eval - h))

del D

