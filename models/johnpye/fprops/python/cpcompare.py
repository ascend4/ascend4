from fprops import *

fa = fluid('methane','helmholtz')
fb = fluid('methane','pengrob')

print "a type =",fa.type
print "b type =",fb.type

T = 150
rho = 5
print "cp helmholtz = %f " % (fa.cp(T,rho))

# calculating same with PR is failing...
print "cp pengrob = %f " % (fb.cp(T,rho))

