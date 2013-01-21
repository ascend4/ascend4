"""
This file plots fugacity versus pressure for the liquid and vapour roots of the
cubic compressibility factor equation derived from the Peng-Robinson
equation of state. We see that the cubic roots are well behaved. This was
at odds with the result found from the previous cubicroots.c code, indicating
some problem with the polynomial solver code we had imported from the GNU
Scientifi Library (GSL). We should report that problem, or look into it further.
"""
from pylab import *
import numpy as np
from fprops import *
from math import sqrt

F = fluid("toluene","pengrob","RPP")
T_c = F.T_c
p_c = F.p_c
rho_c = F.rho_c
omega = F.omega
R = F.R

aTc = 0.45724 * (R * T_c)**2 / p_c;
b = 0.07780 * R * T_c / p_c;
kappa = 0.37464 + (1.54226 - 0.26992 * omega) * omega;

T = 178.15;

pp = []
fff = []
ffg = []

def Zroots(p):
	sqrtalpha = 1 + kappa * (1 - sqrt(T / T_c));
	a = aTc * sqrtalpha**2;

	A = a * p / (R*T)**2;
	B = b * p / (R*T);

	poly1 = array([1, -(1.-B), A - 3.*B**2 - 2.*B, -(A*B - B**2*(1.+B))])

	#print poly1.size
	x = linspace(0,1)

	#plot(x,polyval(poly1,x))

	Z = np.roots(poly1)
	Z.sort()
	return Z,A,B
prange = [0, 0.000001*p_c]

for p in linspace(prange[0], prange[1],1000):

	Z,A,B = Zroots(p)

	#print "Z =", Z

	if Z.size != 3:
		raise RuntimeError("Wrong number of roots")

	if imag(Z[2]):
		print "imaginary root found..."
		continue

	if imag(Z[0]):
		print "imaginary root found..."
		continue


	vg = Z[2]*R*T / p;
	vf = Z[0]*R*T / p;

	def fug(Z):
		return exp( (Z-1) - log(Z - B) - A/(2*sqrt(2)*B)*log( (Z + (1+sqrt(2)) * B) / (Z + (1-sqrt(2)) * B)) )

	fg = fug(Z[2])
	ff = fug(Z[0])

	pp.append(p)
	ffg.append(fg)
	fff.append(ff)

axis([prange[0],prange[1],0,2])
plot(pp,ffg,'r+',label="vapour")
plot(pp,fff,'b+',label="liquid")

Z_c = p_c / rho_c / R / T_c;
plot(p_c,fug(Z_c),'yo',label='critical')

p1 = p_c * 10**(-7./3 * (1.+omega) * (T_c / T - 1.));
Z1,A,B = Zroots(p1)
plot(p1,fug(Z1[0]),'yo',label='acentric')

legend()

show()

