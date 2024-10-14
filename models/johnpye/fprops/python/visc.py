from pylab import *
from fprops import *
import math

D = fluid("hydrogen","helmholtz")

print(D.name)

T = 25+273.15
#rho = 2.144
rho = 71.09
S = D.set_Trho(T,rho)

print("VISCOSITY (expect 160.02 g/cm/s) ")
print(S.mu)

A = [(0,  0.958737511232263)
	,(1, -0.0537931756541833)
	,(2, -0.0417130264021347)
	,(3,  0.00401309021911078)
	,(4,  1.1146015668052e-5)]
	
for i,a in A:
	print(f"i={i}, a_i={a}")

eps_on_k = 30.41

Tstar = T / (eps_on_k)

print(f"Tstar = {Tstar}")

lnpsi = 0
for i,a in A:
	lnpsi += a * math.log(Tstar)**i
print(f"lnpsi = {lnpsi}")

psi = math.exp(lnpsi)
print(f"psi = {psi}")

mu0 = 1.0069 * math.sqrt(T) / psi
print(f"mu0 = {mu0} uPa·s")
