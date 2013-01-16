from fprops import *
from pylab import *

helm = fluid('carbondioxide',"helmholtz")
pr = fluid('carbondioxide',"pengrob")

MYPLOT = semilogx

for F in [helm,pr]:
	F.set_ref(REF_IIR)

	pt, rhoft, rhogt = F.triple_point()

	TT = logspace(log10(F.T_t + 0.0001), log10(1.2 * F.T_c), 20)

	figure()
	hold(1)

	if 1:
		for T in TT:
			rr = linspace(10,rhoft,300)
			pp = array([F.set_Trho(T,r).p for r in rr])
			MYPLOT(1/rr,pp/1e5)

	if 1:
		pp = []
		rrf = []
		rrg = []
		for T in logspace(log10(F.T_t + 0.001), log10(F.T_c - 1e-5),300):
			try:
				SF = F.set_Tx(T,0)
				rf = SF.rho
				SG = F.set_Tx(T,1)
				rg = SG.rho
				p = SF.p
				rrf.append(rf)	
				rrg.append(rg)
				pp.append(p)
			except ValueError,E:
				print "ERROR T=",T,":",str(e)

		pp = array(pp)
		rrf = array(rrf)
		rrg = array(rrg)

		MYPLOT(1/rrf,pp/1e5,'b-')
		MYPLOT(1/rrg,pp/1e5,'r-')

	title("%s (%s)" % (F.name, F.type))
	xlabel("$v$")
	ylabel("$p$ / [bar]")
	axis([1/1000,1/10.,0,100])

show()
