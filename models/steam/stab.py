# this is a script that computes the stability of the DAE system of equations
# by using the sparse matrix routines in scipy and plotting with matplotlib.
#
# you could get fancy and produce a root locus using this technique...

import ascpy

L = ascpy.Library()
L.load('steam/dsgsat3.a4c')
T = L.findType('dsgsat3')
M = T.getSimulation('sim',False)
M.run(T.getMethod('on_load'))
M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
M.run(T.getMethod('configure_dynamic'))
M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
T = L.findType('dsgsat3')
M.run(T.getMethod('free_states'))
# here is the peturbation...
M.qdot_s.setRealValueWithUnits(6000,"W/m")
# IDA has its own initial conditions solver, so no need to call QRSlv here
I = ascpy.Integrator(M)
I.setEngine('IDA')
I.setParameter('linsolver','DENSE')
I.setParameter('safeeval',True)
I.setParameter('rtol',1e-4)
I.setParameter('atolvect',False)
I.setParameter('atol',1e-4)
I.setParameter('maxord',3)
I.setInitialSubStep(0.001)
I.setReporter(ascpy.IntegratorReporterConsole(I))
I.setLogTimesteps(ascpy.Units("s"), 0.001, 3600, 10)
I.analyse()
F = file('ga.mm','w')
I.writeMatrix(F,'dg/dya')
F = file('gd.mm','w')
I.writeMatrix(F,'dg/dyd')
F = file('fa.mm','w')
I.writeMatrix(F,'df/dya')
F = file('fd.mm','w')
I.writeMatrix(F,'df/dyd')
F = file('fdp.mm','w')
I.writeMatrix(F,'df/dydp')
#I.solve()

from scipy import io
from scipy import linalg

gz = io.mmread('ga.mm')
gx = io.mmread('gd.mm')
fz = io.mmread('fa.mm')
fx = io.mmread('fd.mm')
fxp = io.mmread('fdp.mm')

print "gz", gz.shape
print "gx", gx.shape
print "fz", fz.shape
print "fx", fx.shape
print "fxp", fxp.shape

#import pylab

# dg/dy_a

#pylab.spy2(ga.todense())
#pylab.title("${dg}/{dy_a}$")
#pylab.show()

invgz = linalg.inv(gz.todense())

#pylab.figure()
#pylab.spy(invgz)
#pylab.title("$({dg}/{dy_d})^{-1}$")
#pylab.show()

# dg/dy_d

#pylab.figure()
#pylab.spy2(gd.todense())
#pylab.title("${dg}/{dy_d}$")
#pylab.show()

# df/dyd'

#pylab.figure()
#pylab.spy2(fdp.todense())
#pylab.title("${df}/{d\dot{y}_d}$")
#pylab.show()

invfxp = linalg.inv(fxp.todense())

#pylab.spy2(invfdp)
#pylab.title("$({df}/{dy_dp})^{-1}$")
#pylab.show()

dya_dyd = invgz * gx

print "gz^-1 gx",dya_dyd.shape

#pylab.spy2(dya_dyd.todense())
#pylab.title("${dy_a}/{dy_d}$")
#pylab.show()

B = fz * invgz * gx

print "fz gz^1 gz",B.shape
#pylab.spy2(fad.todense())
#pylab.title("${df}/{dy_a} * {dy_a}/{dy_d}$")
#pylab.show()

C = fx + B

D = - invfxp * C

e,v = linalg.eig(D.todense())

#print e

print "max re(e)",max(e.real)
print "min re(e)",min(e.real)

print "max im(e)",max(e.imag)
print "min in(e)",min(e.imag)

I.solve()

