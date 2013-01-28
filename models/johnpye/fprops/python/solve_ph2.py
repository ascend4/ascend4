from fprops import *

F = fluid("isohexane","pengrob","Chem. Eng. Data")

Tt = F.T_t
pt,rhoft,rhogt = F.triple_point()

hft = F.set_Tx(Tt,0).h
hgt = F.set_Tx(Tt,1.).h

print "rhoft = %f, rhogt = %e" % (rhoft, rhogt)
print "hft = %f, hgt = %f" % (hft, hgt)

h = 0.4 * hft + 0.6 * hgt
S = F.set_ph(pt, h)

print "S.rho =",S.rho

print "---"

h = F.set_Trho(Tt,1).h

S = F.set_ph(pt, h)

print "S.rho =",S.rho
print "S.h =",S.h

print "\n---"

T = 128.4465
rho = 1

print "Testing h(T,rho) for T = %f, rho = %f" % (T,rho)

hf = F.set_Tx(T,0).h
hg = F.set_Tx(T,1.).h

rhof = F.set_Tx(T,0).rho
rhog = F.set_Tx(T,1.).rho

print "rhof = %f, rhog = %e" % (rhof, rhog)
print "hf = %f, hg = %f" % (hf, hg)

S = F.set_Trho(T, rho)

print "S.T =",S.T
print "S.rho =",S.rho

print "S.h =",S.h
print "S.x =",S.x
print "S.p =",S.p

print "by h, should be...",(S.h - hf)/(hg - hf)
print "by rho, should be...",(1/rho - 1/rhof)/(1/rhog - 1/rhof)

print "\n---"
print "Solving (p,h) for T = %f, rho = %f" % (T,rho)

p = S.p
h = S.h
S = F.set_ph(p,h)

print "S.T =",S.T
print "S.rho =",S.rho



