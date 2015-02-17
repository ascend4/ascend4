import fprops

w = fprops.fluid('water','iapws','helmholtz');

from pylab import *
figure()

TT = arange(273.16,573.15,10)
rrho = linspace(1000,1,400)
##p = array(len(TT),len(rrho))

for T in TT:
	print "T = %f"%T
	psat,rhof,rhog = w.sat_T(T)
	rrho = linspace(rhof, 0.01*rhog)
	p = [w.p(T,rho)/1e5 for rho in rrho]
	a = [w.a(T,rho)/1e3 for rho in rrho]
	plot(p,a)
	hold(1)

title("a(p,T) for %s" % w.name)
xlabel("Pressure $p$ / [bar]")
ylabel("Helmholtz energy $a$ / [kJ/kg]")
#axis([0,100e3,-1e9,0])
#axis([0,1000,0,100e6])
show()

