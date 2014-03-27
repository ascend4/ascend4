from fprops import *

D = fluid("carbondioxide")

print D.name

S = D.set_Trho(220,2.5)

print "VISCOSITY"
print S.mu

print "CONDUCTIVITY"
print S.lam

