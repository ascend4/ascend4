#!/usr/bin/python
from fprops import *

#Load fluid, specify correlation and source or ommit second an third arguments
hydrogen=fprops_fluid("hydrogen")
water_peng=fprops_fluid("water")#, FPROPS_PENGROB)
water_helm=fprops_fluid("water")#, FPROPS_HELMHOLTZ, "span")
error=fprops_get_err_pointer()

#Set temperature and density
T=400
rho=0.3

#Calculate properties at this point
p_helm=fprops_p(T,rho,water_helm,error)
p_peng=fprops_p(T,rho,water_peng,error)
p_hyd=fprops_p(T,rho,hydrogen,error)
#etc...

if p_helm : print "Helmholtz Pressure: ",p_helm
if p_peng : print "PengRobinson Pressure: ",p_peng
if p_hyd : print "Hydrogen Pressure: ",p_hyd