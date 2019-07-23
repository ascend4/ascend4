FPROPS for OpenModelica
-----------------------

This directory contains some initial experiments for connecting FPROPS to OpenModelica.

To run the example,

svn co http://svn.ascend4.org/trunk/models/johnpye/fprops
cd fprops
scons WITH_ASCEND=0
cd modelica
scons
omc ExternalLibraries.mos



