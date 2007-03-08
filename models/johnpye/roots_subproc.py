# this python script is called from roots.py to work around a crash in scipy

import sys
from scipy import io
from scipy import linalg

if len(sys.argv)<6:
	print "missing arguments"
	sys.exit(2)

fff = {
	'dg/dz' : sys.argv[1]
	,'dg/dx' : sys.argv[2]
	,'df/dz' : sys.argv[3]
	,'df/dx' : sys.argv[4]
	,"df/dx'": sys.argv[5]
}

print "COMPUTING..."

gz = io.mmread(fff['dg/dz'])
gx = io.mmread(fff['dg/dz'])
fz = io.mmread(fff['df/dz'])
fx = io.mmread(fff['df/dx'])
fxp =io.mmread(fff["df/dx'"])

print "gz", gz.shape
print "gx", gx.shape
print "fz", fz.shape
print "fx", fx.shape
print "fxp", fxp.shape

if fxp.shape[0]==0:
	print "fxp is empty (0 rows)"
	sys.exit(1)

if gz.shape[0]==0:
	if gx.shape[0]==0:
		sys.stderr.write("pure differential system\n")
		if fz.shape[1]!=0:
			print "pure differential system but with fz nonzero"
			sys.exit(2)
		
		invfxp = linalg.inv(fxp.todense())
		D = - invfxp * fx
	else:
		print "gz is empty but gx is not!"
		sys.exit(1)

else:
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
print "min im(e)",min(e.imag)

import pylab, sys
sys.stderr.write("about to plot...")
pylab.plot(e.real,e.imag,'rx')
pylab.title("Roots of dx'/dx for DAE system")
pylab.xlabel("Real")
pylab.ylabel("Imaginary")
pylab.show()
sys.stderr.write("DONE\n")

sys.exit(0)

