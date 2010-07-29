import fprops

D = fprops.helmholtz_data_carbondioxide;

from pylab import *
hold(1)

# temperature array
TT = array([217, 230,240,250,260,280,290,300,310,350],'float')

# density array
rr = logspace(log10(10),log10(1200), 200) 

# legend strings
L = []

subplot(3,1,1)
for T in TT:
	pp = [max(fprops.helmholtz_p(T,r,D),1.) for r in rr]
	semilogy(rr,pp)
	axis([10,1200,1.,1e6 * D.p_c])
	L.append("T = %f K" % T)
xlabel('Density')
ylabel('Pressure')
#legend(L)
axis([min(rr),max(rr),1e5,300e5])

L = []
subplot(3,1,2)
for T in TT:
	pp = [fprops.helmholtz_dpdrho_T(T,r,D) for r in rr]
	plot(rr,pp)
	L.append("T = %f K" % T)
ylabel(r'$\left. \frac{\partial p}{\partial \rho} \right|_T$')	
#legend(L)
axis([min(rr),max(rr),-1e5,1e5])

L = []
subplot(3,1,3)
for T in TT:
	pp = [fprops.helmholtz_d2pdrho2_T(T,r,D) for r in rr]
	plot(rr,pp)
	L.append("T = %f K" % T)
ylabel(r'$\left. \frac{\partial^2 p}{\partial \rho^2} \right|_T$')	
#legend(L)
axis([min(rr),max(rr),-1e14,1e14])

show()

