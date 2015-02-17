from fprops import *
from pylab import *

F = fluid('carbon_dioxide','pengrob','RPP')

TT = linspace(F.T_t + 0.0001, 1000)

figure()
hold(1)

for T in TT:
	rr = linspace(10,1200,100)
	pp = [F.p(T,r) for r in rr]
	plot(rr,pp)

axis([10,1200,0,100e6])




G = fluid('carbondioxide','helmholtz')

TT = linspace(G.T_t + 0.0001, 1000)

figure(2)
hold(1)

for T in TT:
	rr = linspace(10,1200,100)
	pp = [G.p(T,r) for r in rr]
	plot(rr,pp)

axis([10,1200,0,100e6])



show()	
