from fprops import *
from pylab import *
import sys

D = fluid("toluene");
if not D:
	print "ERROR couldn't access fluid definition"
	sys.exit(1)

T = D.T_c - 1e-8

res, p1, rf1, rg1 = sat_T(T,D)

if res:
	print "ERROR in calculation of psat(T)"
	print "res = %d" % res

print "psat(T = %f) = %.12e bar" % (T, p1/1e5)

print "rhof = %f, rhog = %f" % (rf1, rg1);

hf = helmholtz_h(T,rf1,D)
hg = helmholtz_h(T,rg1,D)

print "hf = %f, hg = %f" % (hf, hg)

sf = helmholtz_s(T,rf1,D)
sg = helmholtz_s(T,rg1,D)

print "sf = %f, sg = %f" % (sf, sg)
