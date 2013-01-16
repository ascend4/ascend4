# vim: set fileencoding=utf-8
from fprops import *

P = fluid("oxygen","pengrob")
P.set_ref(REF_TRHS(73.15, P.M/53.3256, -6311.6e3/P.M, -20.87e3/P.M));
#P.set_ref(REF_TRHS(73.15, P.M/0.0232, -13501.4/P.M, -119.14/P.M));
#P.set_ref(REF_TRHS(47.85, P.M/0.0600, -7885.19/P.M, -72.01/P.M));

print "For comparison with TABLE 7.5-1 from Sandler 4e"
for T in range(-200,-129,10)+[-125,-120,P.T_c-273.15]:
	print "T = %f °C" % T
	state = P.set_Tx(T+273.15, 0)
	Zf = state.p*state.v/P.R/state.T
	Hf = state.h * P.M/1000
	Sf = state.s * P.M/1000
	Vf = state.v * P.M
	state = P.set_Tx(T+273.15, 1)
	Zg = state.p*state.v/P.R/state.T
	Hg = state.h * P.M/1000
	Sg = state.s * P.M/1000
	Vg = state.v * P.M

	print "\tZg = %12.4f\t\tZf = %12.4f" % (Zg, Zf)
	print "\tVg = %12.4f\t\tVf = %12.4f" % (Vg, Vf)
	print "\tHg = %12.2f\t\tHf = %12.2f" % (Hg, Hf)
	print "\tSg = %12.2f\t\tSf = %12.2f" % (Sg, Sf)

print

#--------------
# Fig 7.5-3 

from pylab import *
TT1 = linspace(100/1.4, P.T_c, 100)
TT = []
pp = []
pp2 = []
for T in TT1:	
	print "T =",T
	try:
		pp2.append(P.psat_T_acentric(T) / 1e5)
		p = P.set_Tx(T,0).p / 1e5
		TT.append(T)
		pp.append(p)
	except ValueError,e:
		print "ERROR:",e
		continue

semilogy(100/array(TT1),pp2,'b-',label="acentric")
semilogy(100/array(TT),pp,'rx',label="Peng-Robinson")
title("Fig 7.5-3 of Sandler 4e (Oxygen)")
legend()

#--------------
# Sandler Fig 7.5-2

P = fluid("n_butane","pengrob")

TT1 = linspace(1000/5.5, P.T_c, 100)
TT = []
pp = []
pp2 = []
for T in TT1:	
	print "T =",T
	try:
		pp2.append(P.psat_T_acentric(T) / 1e5)
		p = P.set_Tx(T,0).p / 1e5
		TT.append(T)
		pp.append(p)
	except ValueError,e:
		print "ERROR:",e
		continue

figure()
semilogy(1000/array(TT1),pp2,'b-',label="acentric")
semilogy(1000/array(TT),pp,'rx',label="Peng-Robinson")
title("Fig 7.5-2 of Sandler 4e (n-Butane)")
legend()
show()

#---------------
# Compare butane with Helmholtz data

P1 = fluid("butane","helmholtz")
P2 = fluid("n_butane","pengrob")

