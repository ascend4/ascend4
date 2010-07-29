from fprops import *
from pylab import *

C = helmholtz_data_carbondioxide;

TT = linspace(C.T_t + 0.0001, 1073)

figure()
hold(1)

for T in TT:
	rr = linspace(10,1000,100)
	pp = [helmholtz_p(T,r,C) for r in rr]
	plot(rr,pp)

axis([10,1000,0,100e6])
show()	
