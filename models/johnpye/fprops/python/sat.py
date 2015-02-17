from fprops import *

D = helmholtz_data_methane;

from pylab import *
hold(1)

T_min = D.T_t
TT = linspace(T_min, D.T_c, 1000)

rhog = array([fprops_rhog_T_chouaieb(T,D) for T in TT])
rhof = array([fprops_rhof_T_rackett(T,D) for T in TT])
psat = array([fprops_psat_T_xiang(T,D) for T in TT])
psata = array([fprops_psat_T_acentric(T,D) for T in TT])

rhof1 = []
rhog1 = []
psat1 = []
rhof2 = []
rhog2 = []
psat2 = []
TT2 = []

TT_src = linspace(T_min, D.T_c, 4000)
TT1 = []
failcount = 0
for T in TT_src:
	res, p1, rf1, rg1 = fprops_sat_T(T,D)
	#print "T=%f, psat=%f bar, rhof=%f, rhog=%f" % (T,p1/1e5,rf1,rg1)
	if res:
		print "error %d in %s saturation function T = %0.10e " % (res,D.name,T)
		failcount += 1
		rhof1.append(rf1)
		rhog1.append(rg1)
		psat1.append(p1)
		TT1.append(T)
		continue
	rhof2.append(rf1)
	rhog2.append(rg1)
	psat2.append(p1)
	TT2.append(T)

print "failcount =",failcount

TT = array(TT)
TT1 = array(TT1)
psat1 = array(psat1)
psat2 = array(psat2)
psat = array(psat)
psata = array(psata)

plot(rhog,TT,label="vapour (Chouaieb)")
plot(rhof,TT,label="liquid (Rackett)")

plot(rhog1,TT1,'rx',label="vapour (unconverged)")
plot(rhof1,TT1,'bx',label="liquid (unconverged)")
plot(rhog2,TT2,'r.',label="vapour (OK, converged)")
plot(rhof2,TT2,'b.',label="liquid (OK, converged)")


legend(loc=8)
xlabel('Density')
ylabel('Temperature')
#legend(L)
#axis([10,1200,1.,1e6 * D.p_c])
#axis([0,1000,0,100e6])

figure()
hold(1)

plot(TT,psat/1e5,label="Xiang")
plot(TT,psata/1e5,label="Acentric")
plot(TT1,psat1/1e5,'rx',label="Maxwell (error)")
plot(TT2,psat2/1e5,'g.',label="Maxwell (OK)")
legend(loc=2)

show()

