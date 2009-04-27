import fprops

w = fprops.helmholtz_data_water;

from pylab import *
hold(1)

TT = array([273.15 + 20*i for i in range(10)])
rrho = array([1000-10*i for i in range(99)])
#p = array(len(TT),len(rrho))

pp = array([1e5 + i*1e5 for i in range(100)])

for T in TT:
	p = [fprops.helmholtz_p(T,r,w) for r in rrho]
	a = [fprops.helmholtz_a(T,r,w) for r in rrho]
	plot(p,a);


#axis([0,100e3,-1e9,0])
#axis([0,1000,0,100e6])
show()

