This folder contains the python wrapper for fprops.  To modify or add any
functions, you must modify the SWIG interface file fprops.i and recompile.
Check out SWIG for more information on this.  Once compiled, the fprops
functions in the interface file will be available from within python.  Again,
check the interface file for some info on proper formatting of arguments.

Right now, there are two options for fluid properties and solution.  The
standard helmholtz properties, and the rpp file.  When calling a fluid for use
bear a few things in mind:

The peng robinson routines can now use both the iapws and rpp type files.
There are then three arguments to a fluid constructor.  They are:
(fluid,datatype,eostype) . If you plan to use the iapws data, remember that you
must enter the name of the fluid as it is in the iapws file respectively. For
example carbon dioxide is 'carbon_dioxide' in the rpp file and 'carbondioxide'
for the iapws fluid.  A call such as:

a=fluid('carbondioxide','rpp','pengrob') will not work correctly in that fprops
will look for carbon dioxide within the rpp file and not the helmholtz fluids.

I'm going to fix this next, so be patient and we'll have a consistent naming
convention.

Also remember the rpp data cannot be used in the helmholtz correlation! It is
strictly for cubics and similar correlations that use this info. There are a
limited number of possible combinations of options currently.  Using carbon
dioxide as an example (there are many more!) They are:

a=fluid('carbon_dioxide','rpp','pengrob') for using the rpp data in the preos.
a=fluid('carbondioxide','iapws','helmholtz') for using the iapws data in the
helmholtz correlations.  a=fluid('carbondioxide','iapws','pengrob') for using
the iapws data in the preos.

The three formats above are the only formats that will work currently.






Some example python scripts for testing are contained here.  Look at these for
examples of how to use python to access the fprops routines.  Some information
for each follows:

dpdrho.py: Get the value of the derivative of pressure with respect to specific
volume at given T

dpdrhoplo1.py: Plot the value of f the derivative of pressure with respect to
specific volume at given T across densities.  Clock time to complete.

pv.py: plot pressure vs. density for both a pengrob and helmholtz fluid under
the same conditions.  Default is carbon dioxide.

refpropscompare1.py : compare some fprops values to those output by refprops.
Some values have already been included in src.txt.  These are actual values
computed with refprops.  The first column is fluid, second is saturation
pressure in kPa, third is liquid density in kg/m3, fourth is vapor density in
kg/m3.  This script will output values in out.txt which will include both the
fprops and refprops values for comparison.

sat.py: a comparison of fprops pv values with correlation values

satcompare.py: head to head comparison of pressure values from both helmholtz
and pengrob correlations.  modify the call to change the property desired.

satcompare1.py: plot values of different properties vs density of both
helmholtz and pengrob fluids

satcvgc.py : a test of convergence of the cubic saturation routine






