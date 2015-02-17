from fprops import *

fa = fluid('methane','helmholtz')
fb = fluid('methane','pengrob','RPP')

print "a type =",fa.type
print "b type =",fb.type

T = 190.38
rho = 500

print "dpdrho helmholtz = %f " % (fa.dpdrho_T(T,rho))
print "dpdrho pengrob = %f " % (fb.dpdrho_T(T,rho))

