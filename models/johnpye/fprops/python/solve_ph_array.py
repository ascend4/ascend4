from fprops import *
from pylab import *

D = helmholtz_data_water;

TT = linspace(273.16, 1000, 100);
rr = logspace(log10(0.01), log10(900), 100);

goodT = []
goodv = []
badT = []
badv = []

for T in TT:
	for rho in rr:
		print "+++ T = %f, rho = %f" % (T,rho)
		p = helmholtz_p(T,rho,D)
		h = helmholtz_h(T,rho,D)
		print "    p = %f bar, h = %f kJ/kg" % (p/1e5,h/1e3)
		if(h > 8000e3):
			continue

		res, T1, rho1 = fprops_solve_ph(p,h,0,D);
		if res:
			print "   +++ BAD RESULT T1 = %f, rho1 = %f" % (T1, rho1)
			if not isnan(T) and not isnan(rho):
				badT.append(T); badv.append(1./rho)
		else:
			if not isnan(T) and not isnan(rho):
				goodT.append(T); goodv.append(1./rho)
			print "   +++ GOOD RESULT T1 = %f, rho1 = %f" % (T1, rho1)

figure()
print badT
semilogx(badv, badT, 'rx')
hold(1)
semilogx(goodv, goodT, 'g.')

# plot saturation curves
TT = linspace(D.T_t, D.T_c, 100)
TT1 = []
vf1 = []
vg1 = []
for T in TT:
	res, p, rhof, rhog = fprops_sat_T(T,D)
	if not res:
		TT1.append(T)
		vf1.append(1./rhof)
		vg1.append(1./rhog)

semilogx(vf1,TT1,"b-")
semilogx(vg1,TT1,"b-")
xlabel("specific volume")
ylabel("temperature")

show()

