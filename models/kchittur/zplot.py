import extpy
from pylab import *
from solverreporter import *

def pvplot(self):

	browser = extpy.getbrowser()
	ioff()
	figure()
#
# I have chosen several temperatures 
#

	TT = [190,210,230,250,270,290,310,330,350,370,400,420]
	for T in TT:
		self.T.setRealValue(T)
#
# collect the data for plotting in two sets of arrays (one for X, one for Y)
# I have one set here - for P versus V at different T's  
#
		XX1 = []
		YY1 = []
#
# there has to be a space between "in" and "["
#

		for P in [8,9,10,15,20,25,30,40,50,60,70,80,100,120,140,150,170,190]:
		    self.P.setRealValueWithUnits(P,"bar")
#
# send the pair of values P, T to the solver
# and append the Volume and Pressure from the solver to the arrays 
		    try:
				browser.sim.solve(browser.solver,SimpleSolverReporter(browser,message="T = %f, P = %f" % (T,P)))
				XX1.append(float(self.V))
				YY1.append(float(self.P))
		    except:
		        browser.reporter.reportError('Failed to solve for P = %f' % P)
		        continue
## plot the data 

		plot(XX1,YY1,label=("%d K" % T))

		hold(1)

	legend()	
	xlabel("Molar volume")
	ylabel("Presure (Pa)")
	title("p-V curves at varying temperatures")
	ion()
	show()

extpy.registermethod(pvplot)

def zplot(self):

	browser = extpy.getbrowser()
	ioff()
	figure()
	axes([0.1,0.1,0.71,0.8])
#
# I have chosen three temperatures 
#
	TT = [190,210,230,250,270,290,310,330,350,370]
	for T in TT:
		self.T.setRealValue(T)
#
# collect the data for plotting in two sets of arrays (one for X, one for Y)
# I have two sets here - one for P versus y and other for P versus x 
#
		PPr1 = []
		ZZ1 = []
#
# change x1 from 0 to 1.0 
# there has to be a space between "in" and "["
#

		for P in [10,11,12,13,15,20,25,30,40,50,60,70,80,100,120,140,150,170,175,180,185,190]:
		    self.P.setRealValueWithUnits(P,"bar")
#
# send the pair of values T x1 to the solver
# and append the Pressure and y1 (from the solver) to the arrays 
# the x's are also appended  
		    try:
				browser.sim.solve(browser.solver,SimpleSolverReporter(browser,message="T = %f, P = %f" % (T,P)))
				PPr1.append(float(self.Pr))
				ZZ1.append(float(self.Z))
		    except:
		        browser.reporter.reportError('Failed to solve for P = %f' % P)
		        continue
## plot the data 

		plot(PPr1,ZZ1,label="%d K" % T)
		hold(1)

# after all the plots are done, add some labels and a legend	
	xlabel("Reduced pressure, p/p_crit")
	ylabel("Generalised compressibility, z")
	legend(loc=(1.03,0.2))
	ion()
	show()

extpy.registermethod(zplot)
# the above method can be called using "EXTERNAL zplot(self)" in ASCEND.
# if you want to see the azeotrope clearly, restrict the calculation to one
# temperature

