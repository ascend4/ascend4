from fprops import *
from pylab import *
fb = fluid('carbon_dioxide','rpp','pengrob')
fa = fluid('carbondioxide','iapws','helmholtz')


print "a type =",fa.type
print "b type =",fb.type
fa.set_ref(REF_NBP)

T = 298
rho = linspace(1,1500)

figure()
hold(1)
ppa=array([fa.p(T,i) for i in rho])
ppb=array([fb.p(T,i) for i in rho])

plot(rho,ppa/1e3,label = 'helmholtz')
plot(rho,ppb/1e3, label = 'pengrob')
title(r'Comparison of p(T,rho) for T=%f K'%T)
xlabel(r'Density $\rho$ / [kg/m${}^3$]')
ylabel(r'Pressure $p$ / kPa')
legend(loc=2)

hha=array([fa.h(T,i) for i in rho])
ha=hha[0]
hhb=array([(fb.h(T,i)) for i in rho])
hb=hhb[0]

figure(2)
hold(1)
plot(rho,hha/1.e3,label = 'helmholtz h')
plot(rho,hhb/1.e3, label = 'pengrob h')
title(r'Comparison of h(T,rho) for T=%f K'%T)
xlabel(r'Density $\rho$ / [kg/m${}^3$]')
ylabel(r'Enthalpy $h$ / [kJ/kg]')
legend(loc=9)

ssa=array([fa.s(T,i) for i in rho])
sa=ssa[0]
ssb=array([(fb.s(T,i)) for i in rho])
sb=ssb[0]

figure(3)
hold(1)
plot(rho,ssa/1e3,label = 'helmholtz s')
plot(rho,ssb/1e3, label = 'pengrob s')
title(r'Comparison of s(T,rho) for T=%f K'%T)
xlabel(r'Density $\rho$ / [kg/m${}^3$]')
ylabel(r'Enthalpy $s$ / [kJ/kgK]')
legend()

aaa=array([fa.a(T,i) for i in rho])
aab=array([(fb.a(T,i)) for i in rho])

figure(4)
hold(1)
plot(rho,aaa/1e3,label = 'helmholtz a')
plot(rho,aab/1e3, label = 'pengrob a')
title(r'Comparison of a(T,rho) for T=%f K'%T)
xlabel(r'Density $\rho$ / [kg/m${}^3$]')
ylabel(r'Helmholtz energy $a$ / [kJ/kg]')
legend(loc=4)

print "difference in h = %f " % (abs(ha-hb))
print "difference in s = %f " % (abs(sa-sb))

show()	

