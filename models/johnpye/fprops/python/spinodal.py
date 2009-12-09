import fprops

D = fprops.helmholtz_data_carbondioxide;

from pylab import *
hold(1)

TT = array([217, 230,240,250,260,280,300])
L = []

for T in TT:
	rr = logspace(log10(10),log10(1200), 200)
	print rr
	pp = [max(fprops.helmholtz_p(T,r,D),1.) for r in rr]
	
	semilogy(rr,pp)
	L.append("T = %f K" % T)

xlabel('Density')
ylabel('Pressure')
#legend(L)

axis([10,1200,1.,1e6 * D.p_c])
#axis([0,1000,0,100e6])
show()

