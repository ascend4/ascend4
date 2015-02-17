from fprops import *
from pylab import *
from time import clock
fb = fluid('methane','rpp','pengrob')
fa = fluid('methane','iapws','helmholtz')


print "a type =",fa.type
print "b type =",fb.type

T = linspace(100,190,100)
rho = 5

figure()
hold(1)
a=clock()
ppa=[fa.p(i,rho) for i in T]
b=clock()

c=clock()
ppb=[fb.p(i,rho) for i in T]
d=clock()


plot(T,ppa,label = 'helmholtz')
plot(T,ppb, label = 'pengrob')
legend()

axis([100,190,0,1e6])


print "peng rob pressure time =",d-c
print "helmholtz pressure time =",b-a

show()	

