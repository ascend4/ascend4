from fprops import *
from pylab import *
from time import clock
fb = fluid('methane','rpp','pengrob')
fa = fluid('methane','iapws','helmholtz')


print "a type =",fa.type
print "b type =",fb.type

T = 190.394
rho = linspace(.01,153,5000)

figure()
hold(1)

c=clock()
ppb=[fb.dpdrho_T(T,i) for i in rho]
d=clock()


#plot(rho,ppa,label = 'helmholtz')
plot(rho,ppb, label = 'pengrob')
legend()

axis([.01,153,-3e8,0])


print "peng rob pressure time =",d-c


show()	

