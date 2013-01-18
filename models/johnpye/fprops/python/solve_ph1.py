from fprops import *

F = fluid("water","helmholtz")

S = F.set_Trho(504.67544673214644, 836.3369774420968)

print "initial state: T = %f, rho = %f" % (S.T, S.rho)

p = S.p
h = S.h

print "        gives p = %f, h = %f\n" % (p,h)

print "attempt to recover initial state from (p,h)...\n"

S = F.set_ph(p,h)

print "T =", S.T

