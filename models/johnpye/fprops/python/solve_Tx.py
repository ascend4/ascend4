import fprops,sys

D = fprops.fluid("toluene");

T = 300;
x = 0.5;

res, rho = fprops.solve_Tx(T,x, D);

if res:
	print "failed to solve state, exiting."
	sys.exit(1);

print "solved state"

print "T = %f, x = %f" % (T,x)

h = fprops.helmholtz_h(T,rho,D);
u = fprops.helmholtz_u(T,rho,D);
s = fprops.helmholtz_s(T,rho,D);

print "--> rho = %f, h = %f, s = %f, u = %f" % (rho,h,u,s)

print "\nchecking reverse solve:"

res, psat, rhof, rhog = fprops.sat_T(T,D)

x1 = (1./rho - 1./rhof) / (1./rhog - 1./rhof);

print "--> x = %f" % x1

sys.exit(0)

