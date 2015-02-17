from fprops import *
from pylab import *
from time import clock
import sys

fb = fluid('methane','pengrob')
fa = fluid('methane','helmholtz')

T = 190.394
rho = linspace(1,300,50000)

sys.stderr.write("Please wait ~30 seconds to complete...\n")
a=clock()
ppa=[fa.p(T,i) for i in rho]
b=clock()

c=clock()
ppb=[fb.p(T,i) for i in rho]
d=clock()

print "peng rob pressure time =",d-c
print "helmholtz pressure time =",b-a



