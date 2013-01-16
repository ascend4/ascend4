from fprops import *

fa = fluid('water','helmholtz')
fb = fluid('water','pengrob')

print "a type =",fa.type
print "b type =",fb.type

T = 298.15
rho = 997.5
print "pa = %f bar" % (fa.set_Trho(T,rho).p/1e5)

# calculating same with PR is failing...
print "pb = %f bar" % (fb.set_Trho(T,rho).p/1e5)
