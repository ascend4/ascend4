from fprops import *
fb = fluid('methane','rpp','pengrob')
fa = fluid('methane','iapws','pengrob')


print "a type =",fa.type
print "b type =",fb.type

fa.set_ref(REF_NBP)

T = 150
rho = 5
print "pressure helmholtz = %f" % (fa.p(T,rho))

# calculating same with PR is failing...
print "pressure pengrob = %f" % (fb.p(T,rho))
