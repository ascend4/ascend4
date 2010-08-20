from fprops import *
from pylab import *
import sys

D = fprops_fluid('carbondioxide');

print "SOLVING TRIPLE POINT..."

res, p_t, rhof_t, rhog_t = fprops_triple_point(D)
if res:
	print "failed to solve triple point"
	sys.exit(1)

pmax = 100e6

Tmin = D.T_t
Tmax = 2 * D.T_c
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
		p = helmholtz_p(T,rho,D)
		if p > pmax:
			continue
		h = helmholtz_h(T,rho,D)
		#print "    p = %f bar, h = %f kJ/kg" % (p/1e5,h/1e3)
		if(h > 8000e3):
			continue

		res, T1, rho1 = fprops_solve_ph(p,h,0,D);
		if res or isnan(T1) or isnan(rho1):
			print "Error at T1 = %f, rho1 = %f (T = %.12e, rho = %.12e)" % (T1, rho1,T,rho)
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
TTs = linspace(D.T_t, D.T_c, 300)
TT1 = []
vf1 = []
vg1 = []
for T in TTs:
	res, p, rhof, rhog = fprops_sat_T(T,D)
	if not res:
		TT1.append(T)
		vf1.append(1./rhof)
		vg1.append(1./rhog)

semilogx(vf1,TT1,"b-")
semilogx(vg1,TT1,"b-")
axis([vmin,vmax,Tmin,Tmax])
xlabel("specific volume")
ylabel("temperature")

show()

