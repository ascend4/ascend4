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
	for T in [190,210,230,250,270,290,310,330,350,370,400, 420]:
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

		plot(XX1,YY1)

		hold(1)
	
##	legend()
	ion()
	show()

extpy.registermethod(pvplot)

def zplot(self):

	browser = extpy.getbrowser()
	ioff()
	figure()
#
# I have chosen three temperatures 
#
	for T in [190,210,230,250,270,290,310,330,350,370]:
		self.T.setRealValue(T)
#
# collect the data for plotting in two sets of arrays (one for X, one for Y)
# I have two sets here - one for P versus y and other for P versus x 
#
		XX1 = []
		PP1 = []
#
# change x1 from 0 to 1.0 
# there has to be a space between "in" and "["
#

		for P in [10,15,20,25,30,40,50,60,70,80,100,120,140,150,170,190]:
		    self.P.setRealValueWithUnits(P,"bar")
#
# send the pair of values T x1 to the solver
# and append the Pressure and y1 (from the solver) to the arrays 
# the x's are also appended  
		    try:
				browser.sim.solve(browser.solver,SimpleSolverReporter(browser,message="T = %f, P = %f" % (T,P)))
				XX1.append(float(self.Pr))
				PP1.append(float(self.Z))
		    except:
		        browser.reporter.reportError('Failed to solve for P = %f' % P)
		        continue
## plot the data 

		plot(XX1,PP1)

		hold(1)
	
##	legend()
	ion()
	show()

extpy.registermethod(zplot)
#the above method can be called using "EXTERNAL vleplot(self)" in ASCEND.
# if you want to see the azeotrope clearly, restrict the calculation to one
# temperature 
