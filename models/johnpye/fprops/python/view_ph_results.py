from fprops import *
from pylab import *
import ConfigParser, os

C = ConfigParser.ConfigParser()
C.readfp(open("pherr.ini"))
f1 = C.get("main","fluid")
t1 = C.get("main","type")
s1 = C.get("main","source")
# range of values of temperature and specific volume for plot
tmin = float(C.get("main","tmin"))
tmax = float(C.get("main","tmax"))
vmin = float(C.get("main","vmin"))
vmax = float(C.get("main","vmax"))
# lower limit for saturation curves
tt = float(C.get("main","tt"))
dataf = C.get("main","data")

A = loadtxt(dataf)

F = fluid(f1,t1,s1)

print "FLUID %s" % F.name
print A.size

Tc = F.T_c;

TT0 = linspace(tmin,Tc,150)
TT = []
vvff = []
vvgg = []
pp = []
for T in TT0:
	p,rhof,rhog = F.sat_T(T);
	TT += [T]
	vvff += [1./rhof]
	vvgg += [1./rhog]
	pp += [p]

TT = array(TT)
vvff = array(vvff)
vvgg = array(vvgg)
pp = array(pp)

semilogx(vvff,TT,'b-')
semilogx(vvgg,TT,'r-')
xlabel("v / [m3/kg]")
ylabel("T / [K]")
axis([vmin, vmax, tmin, tmax])

if A.size >= 2:
	scatter(A[:,1],A[:,0])
show()

