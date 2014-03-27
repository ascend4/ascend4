from pylab import *
from fprops import *

D = fluid("carbondioxide")

print D.name

S = D.set_Trho(220,2.5)

print "VISCOSITY"
print S.mu

print "CONDUCTIVITY"
print S.lam

hh = linspace(100e3, 2000e3,100)
pp = [1.5e6,2.5e6, 5e6, 6e6,7e6,8e6,9e6,10e6, 20e6,50e6, 100e6]
figure()
hold(1)
for p in pp:
	print p
	ll = []
	TT = []
	for h in hh:
		S = D.set_ph(p,h)
		lam = S.lam
		T = S.T
		rho = S.rho
		ll.append(lam)
		TT.append(T)
		print "p=%f, h=%f -> T=%f, rho=%f, lam=%f"%(p,h,T,rho,lam)
	plot(TT,ll)

show()
