from fprops import *

D = helmholtz_data_carbondioxide;

from pylab import *
hold(1)

T_min = 206.
TT = linspace(T_min, D.T_c, 1000)

rhog = array([fprops_rhog_T_chouaieb(T,D) for T in TT])
rhof = array([fprops_rhof_T_rackett(T,D) for T in TT])
psat = array([fprops_psat_T_xiang(T,D) for T in TT])

rhof1 = []
rhog1 = []
psat1 = []

TT2 = linspace(T_min, D.T_c, 300)
TT1 = []
for T in TT2:
	res, p1, rf1, rg1 = fprops_sat_T(T,D)
	if res:
		print "error in saturation function T =",T
		continue
	rhof1.append(rf1)
	rhog1.append(rg1)
	psat1.append(p1)
	#print "T=%f, psat=%f bar, rhof=%f, rhog=%f" % (T,p1/1e5,rf1,rg1)
	TT1.append(T)

TT = array(TT)
TT1 = array(TT1)
psat1 = array(psat1)
psat = array(psat)

plot(rhog,TT,label="vapour (Chouaieb)")
plot(rhof,TT,label="liquid (Rackett)")

plot(rhog1,TT1,'rx',label="vapour (Maxwell)")
plot(rhof1,TT1,'bx',label="liquid (Maxwell)")


legend(loc=8)
xlabel('Density')
ylabel('Temperature')
#legend(L)
#axis([10,1200,1.,1e6 * D.p_c])
#axis([0,1000,0,100e6])

figure()
hold(1)

plot(TT,psat/1e5,label="Xiang")
plot(TT1,psat1/1e5,'rx',label="Maxwell")

show()

