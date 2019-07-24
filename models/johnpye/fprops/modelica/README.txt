FPROPS for OpenModelica
-----------------------

This directory contains some initial experiments for connecting FPROPS to
OpenModelica. So far it is just proof of concept showing how FPROPS property
calculations can be called from Modelica; it is by no means a comprehensive
wrapper for fully-fledged simulation work.

To run the example,

svn co http://svn.ascend4.org/trunk/models/johnpye/fprops
cd fprops
scons WITH_ASCEND=0
cd modelica
scons
export LD_LIBRARY_PATH=..
omc ExternalLibraries.mos
python simpleplot.py

The .mos script loads and runs a trivial simulation in which a pressurised
mass of CO₂ is heated, resulting in a gradual temperature rise.

The 'simpleplot.py' script finds the resulting *_res.mat file and displays its
contents as a plot.

It is intended that this code will be extended slightly to allow different
fluids to be selected from Modelica. Currently the code is hard-wired for 
'carbondioxide' (Helmholtz correlation).


Useful resources
----------------

Modelica By Example -- External Functions
https://mbe.modelica.university/behavior/functions/external/
* this is mostly reference materials from the Modelica Language Specification

Interoperability -- C and Python (OpenModelica)
https://www.openmodelica.org/doc/OpenModelicaUsersGuide/latest/interop_c_python.html
* this has some nice examples of external functions, but is sparse on details
  about how to make it all work. The Python stuff is not relevant here.

Modelica Language Specification version 3.4
https://www.modelica.org/documents/ModelicaSpec34.pdf#page=165
* This gives some more formal description of how external functions are declared
  including documentation of the 'annotations' that relate to linking with
  external functions. Modelica can compile external functions itself, but
  in this work, we are doing that outside Modelica, using 'scons'.

SCons Documentation
https://scons.org/documentation.html
* SCons is a tool for compiling software and other similar tasks.

FPROPS
https://fprops.org/
* FPROPS is a small library for calculating the thermodynamic and thermophysical
  properties of pure fluids using accurate Helmholtz correlations as well as
  less accurate correlations such as Peng-Robinson. It has wrappers to allow
  its use with Python and the modelling language ASCEND (https://ascend4.org/)

-- 
John Pye
Jul 2019

