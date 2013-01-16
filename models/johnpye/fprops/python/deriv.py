from fprops import *

for t in ["helmholtz","pengrob"]:

	print "=======================\n",t

	F = fluid("carbondioxide",t)

	T = 373
	rho = 959.3494898636712
	S = F.set_Trho(T,rho)

	cp = S.cp

	print "cp =",cp/1e3,"kJ/kgK"

	dhdT_p = S.deriv("hTp")

	print "dhdT_p =", dhdT_p/1e3,"kJ/kgK\n"

	cv = S.cv

	print "cv =",cv/1e3,"kJ/kgK"

	dudT_v = S.deriv("uTv")

	print "dudT_v =", dudT_v/1e3,"kJ/kgK"

	u1 = S.u
	T2 = T + 0.01
	S2 = F.set_Trho(T2,rho)
	u2 = S2.u
	dudT_v_approx = (u2 - u1)/(S2.T - S.T)

	print "cv = dudT_v(approx) =", dudT_v_approx/1e3,"kJ/kgK"

	print "R = ",F.R/1e3,"kJ/kgK"

	alphap = S.alphap;

	print "alphap =",alphap	

	print "betap(via deriv) =",-1/S.p * S.deriv('pvT')

	betap = S.betap;

	print "betap =",betap

	p1 = S.p
	v2 = S.v * 0.9999
	S2 = F.set_Trho(S.T,1/v2)
	p2 = S2.p
	dpdv_T_approx = (S2.p - S.p)/(S2.v - S.v)
	
	print "betap(approx) =",-1/S.p * dpdv_T_approx

