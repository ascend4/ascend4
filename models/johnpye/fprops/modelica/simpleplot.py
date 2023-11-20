import DyMat
import sys
import glob

if len(sys.argv) < 2:
	g = glob.glob("*_res.mat");
	if len(g) == 1:
		print "Opening file '%s'" % g[0]
		fn = g[0]
	else:
		print "Multiple _res.mat files, specify one on the command line."
else:
	fn = sys.argv[1]

D = DyMat.DyMatFile(fn)

plots_made = 0
for B in D.blocks():
	print "block %d" % (B,)
	print D.names(B)
	t, tname, tdesc = D.abscissa(2)
	print "indep",tname
	print "size",D.size(B)
	if D.size(B) <= 2:
		for i,n in enumerate(D.names(B)):
			print "  %s =" % (n), D[n]
	else:
		from pylab import *
		subplots(len(D.names(B)),1)
		for i,n in enumerate(D.names(B)):
			subplot(len(D.names(B)),1,i+1)
			plot(t,D[n],label=n)
			legend()
			plots_made = 1

if plots_made:
	xlabel(tname)
	show()

#print "Loaded file '%s'" % fn
#print D.names()


