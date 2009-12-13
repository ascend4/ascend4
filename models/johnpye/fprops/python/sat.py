from fprops import *

D = helmholtz_data_carbondioxide;

from pylab import *
hold(1)

TT = linspace(206.,D.T_c,1000)

rhog = array([fprops_rhog_T_chouaieb(T,D) for T in TT])
rhof = [fprops_rhof_T_rackett(T,D) for T in TT]

plot(rhog,TT,label="vapour (Chouaieb)")
plot(rhof,TT,label="liquid (Rackett)")

legend(loc=8)
xlabel('Density')
ylabel('Temperature')
#legend(L)

#axis([10,1200,1.,1e6 * D.p_c])
#axis([0,1000,0,100e6])
show()

