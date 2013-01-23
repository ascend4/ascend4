from fprops import *

D = fluid("ethanol")

from pylab import *
hold(1)

T_min = D.T_t
TT = linspace(T_min, D.T_c, 1000)

rhog = array([D.rhog_T_chouaieb(T) for T in TT])
rhof = array([D.rhof_T_rackett(T) for T in TT])
psat = array([D.psat_T_xiang(T) for T in TT])
psata = array([D.psat_T_acentric(T) for T in TT])

rhof2 = []
rhog2 = []
psat2 = []
TT2 = []

TT_src = linspace(T_min, D.T_c, 4000)
TT1 = []
failcount = 0
for T in TT_src:
	try:
		p1, rf1, rg1 = D.sat_T(T)
	except Exception,e:
		print "error '%s' in %s saturation function T = %0.10e " % (str(e),D.name,T)
		failcount += 1
		TT1.append(T)
		continue
	rhof2.append(rf1)
	rhog2.append(rg1)
	psat2.append(p1)
	TT2.append(T)

print "failcount =",failcount

TT = array(TT)
TT1 = array(TT1)
psat2 = array(psat2)
psat = array(psat)
psata = array(psata)

plot(rhog,TT,label="vapour (Chouaieb)")
plot(rhof,TT,label="liquid (Rackett)")

#plot(rhog1,TT1,'rx',label="vapour (unconverged)")
#plot(rhof1,TT1,'bx',label="liquid (unconverged)")
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
#plot(TT1,psat1/1e5,'rx',label="Maxwell (error)")
plot(TT2,psat2/1e5,'g.',label="FPROPS (OK)")
legend(loc=2)

show()

