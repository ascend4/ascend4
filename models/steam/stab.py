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
print "STEADY-STATE SOLUTION..."
M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
M.run(T.getMethod('configure_dynamic'))
M.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())

M.run(T.getMethod('free_states'))
# here is the peturbation...
print "CREATING PETURBATION..."
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
I.setParameter('calcic','YA_YDP')
I.setInitialSubStep(0.001)
I.setReporter(ascpy.IntegratorReporterConsole(I))
I.setLogTimesteps(ascpy.Units("s"), 0.001, 0.002, 10)
I.analyse()
F = file('gz.mm','w')
I.writeMatrix(F,'dg/dz')
F = file('gx.mm','w')
I.writeMatrix(F,'dg/dx')
F = file('fz.mm','w')
I.writeMatrix(F,'df/dz')
F = file('fx.mm','w')
I.writeMatrix(F,'df/dx')
F = file('fxp.mm','w')
I.writeMatrix(F,"df/dx'")
#I.solve()

from scipy import io
from scipy import linalg

gz = io.mmread('gz.mm')
gx = io.mmread('gx.mm')
fz = io.mmread('fz.mm')
fx = io.mmread('fx.mm')
fxp = io.mmread('fxp.mm')

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

print "ROOT RANGE-----------"
print "max re(e)",max(e.real)
print "min re(e)",min(e.real)
print "max im(e)",max(e.imag)
print "min in(e)",min(e.imag)
sys.stdout.flush()

#I.solve()

import pylab, sys
sys.stderr.write("about to plot...")
pylab.plot(e.real,e.imag,'rx')
pylab.xlabel('Real axis')
pylab.ylabel('Imaginary axis')
pylab.show()
sys.stderr.write("DONE\n")

I.setLogTimesteps(ascpy.Units("s"), 0.0005, 3600, 10)
I.setParameter('calcic','Y')
I.solve()

