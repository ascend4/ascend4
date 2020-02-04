from fprops import *
from math import fabs
import sys

D = fluid('sodium');

# a python version of the sodium.c tests, more or less....
p = 1e5

S = D.set_pT(15e5,1000.)
print "S.T =",S.T
print "S.p =",S.p
print "S.rho =",S.rho
print "D.name =",D.name

assert abs(S.T - 1000.) < 1e-9
assert abs(S.rho - 781.) < 0.5
assert abs(S.h - 1020e3) < 0.5e3

class TC:
	def __init__(self,T,rho,lam,h,s,mu):
		self.T=T; self.rho=rho; self.lam=lam;self.h=h; self.s=s;self.mu=mu;
	def test_rho(self):
		state = D.set_pT(p,self.T)
		assert abs(state.rho - self.rho) < 0.5
	def test_lam(self):
		state = D.set_pT(p,self.T)
		lam = state.lam
		#print "    lam = %f, expecting %f, diff = %f" %(lam,self.lam,(lam-self.lam))
		assert abs(state.lam - self.lam) < 0.008
	def test_h(self):
		state = D.set_pT(p,self.T)
		assert abs(state.h - self.h) < 0.5e3
	def test_s(self):
		state = D.set_pT(p,self.T)
		s = state.s
		#print "    s = %f, expecting %f" %(s,self.s)
		assert abs(state.s - self.s) < 0.0000005e3
	def test_mu(self):
		state = D.set_pT(p,self.T)
		assert abs(state.mu - self.mu) < 0.0000005e3
	def test_all(self):
		self.test_rho()
		self.test_lam()
		self.test_h()
		self.test_mu()
			
	
tests = [
	TC(400.,   919.,   87.22, 247e3,  0.53326716242e3, 5.99e-4)
	,TC(600.,  874.,   73.70, 514e3,  1.07537491829e3, 3.21e-4)
	,TC(800.,  828.,   62.90, 769e3,  1.44336831041e3, 2.27e-4)
	,TC(1000., 781.,   54.24, 1020e3, 1.72313707816e3, 1.81e-4)
	,TC(1200., 732.,   47.16, 1273e3, 1.95341568058e3, 1.53e-4)
	,TC(1400., 680.,   41.08, 1534e3, 2.15497925873e3, 1.35e-4)
	,TC(1800., 568.,   29.68, 2113e3, 2.51730964576e3, 1.12e-4)
]

for t in tests:
	print "At T = %f..." %(t.T,)
	t.test_all()

if 0:
	p = 5e5;
	h = 1020;
	print "p = %f bar, h = %f kJ/kg\n" % (p/1e5, h/1e3)

	state = D.set_pT(p,T);

	print "T = %f" % state.T
	print "rho = %f" % state.rho

	assert fabs(state.T - 1000.) < 0.5

	p_eval = state.p
	h_eval = state.h
	print "p(T,rho) = %f bar\t  (target: %f, err = %e)" % (p_eval/1e5, p/1e5, (p_eval - p))
	print "h(T,rho) = %f kJ/kg\t  (target: %f, err = %e)" % (h_eval/1e3, h/1e3, (h_eval - h))

	del D
