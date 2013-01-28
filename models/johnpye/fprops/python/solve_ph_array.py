from fprops import *
from pylab import *
import sys

#P = fluid('water','helmholtz');
#P = fluid('ammonia','pengrob');
P = fluid('carbondioxide','pengrob');

print "SOLVING TRIPLE POINT..."

print "Fluid: %s\nData source: %s" %(P.name, P.source)

try:
	p_t, rhof_t, rhog_t = P.triple_point()
except RuntimeError,e:
	print "failed to solve triple point"
	sys.exit(1)

pmax = 100e6

Tmin = P.T_t

if Tmin == 0:
	Tmin = 0.4 * P.T_c

Tmax = 2 * P.T_c
vmin = 1./rhof_t
vmax = 2./rhog_t
TT = linspace(Tmin, Tmax, 100);
vv = logspace(log10(vmin),log10(vmax), 100);

goodT = []
goodv = []
badT = []
badv = []

for T in TT:
	sys.stderr.write("+++ T = %f\r" % (T))
	for v in vv:
		rho = 1./v
		S = P.set_Trho(T,rho)
		p = S.p
		if p > pmax:
			continue
		h = S.h
		#print "    p = %f bar, h = %f kJ/kg" % (p/1e5,h/1e3)
		if(h > 8000e3):
			continue

		try:
			S = P.set_ph(p,h)
			T1 = S.T
			rho1 = S.rho
		except ValueError,e:
			print "ERROR %s at p = %f, h = %f (T = %.12e, rho = %.12e)" % (str(e),p, h,T,rho)
			badT.append(T); badv.append(v)
			continue	
		if isnan(T1) or isnan(rho1):
			print "ERROR at T1 = %f, rho1 = %f (T = %.12e, rho = %.12e)" % (T1, rho1,T,rho)
			badT.append(T); badv.append(v)
		else:
			goodT.append(T); goodv.append(v)
			#print "   +++ GOOD RESULT T1 = %f, rho1 = %f" % (T1, rho1)

figure()

print "i \tbad T    \tbad v"
for i in range(len(badT)):
	print "%d\t%e\t%e" % (i,badT[i], badv[i])

print "TOTAL %d BAD POINTS" % (len(badT))

print "AXIS =",axis()
semilogx(badv, badT, 'rx')
axis([vmin,vmax,Tmin,Tmax])
print "AXIS =",axis()
hold(1)
semilogx(goodv, goodT, 'g.')

# plot saturation curves
TTs = linspace(P.T_t, P.T_c, 300)
TT1 = []
vf1 = []
vg1 = []
for T in TTs:
	try:
		S = P.set_Tx(T,0)
		p = S.p
		rhof = S.rho
		S = P.set_Tx(T,1)
		rhog = S.rho
	except:
		continue;	
	TT1.append(T)
	vf1.append(1./rhof)
	vg1.append(1./rhog)

semilogx(vf1,TT1,"b-")
semilogx(vg1,TT1,"b-")
axis([vmin,vmax,Tmin,Tmax])
title("convergence of (p,h) solver for %s" % P.name)
xlabel("specific volume")
ylabel("temperature")

show()
ion()


