from fprops import *
from numpy import *
from pylab import *
hold(1)

TT = arange(573.15,800,80)
rrho = arange(0.01,1000,10)
pp = array([1e5 + i*1e5 for i in range(100)])

x_h=fluid("xenon","helmholtz")
x_p=fluid("xenon","pengrob")

for T in TT:
	p = [x_h.p(T,r) for r in rrho]
	h = [x_h.h(T,r) for r in rrho]
	plot(p,h,'r-');

for T in TT:
	p = [x_p.p(T,r) for r in rrho]
	h = [x_p.h(T,r) for r in rrho]
	plot(p,h,'r:');

show()

