from fprops import *
from pylab import *

D = helmholtz_data_water;

TT = linspace(273.16, 1000, 20);
rr = logspace(log10(0.01), log10(900), 20);

goodT = []
goodrho = []
badT = []
badrho = []

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
			print "   +++ BAD RESULT"
			if not isnan(T) and not isnan(rho):
				badT.append(T); badrho.append(rho)
		else:
			goodT.append(T); goodrho.append(rho)
			print "   +++ GOOD RESULT T1 = %f, rho1 = %f" % (T1, rho1)

figure()
print badT
semilogx(badrho, badT, 'rx')
hold(1)
semilogx(goodrho, goodT, 'g.')
show()

