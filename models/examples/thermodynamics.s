# This file is part of the Ascend modeling library to
# demonstrate thermodynamics..asc and is released under the GNU
# Public License as noted at the beginning of thermodynamics.asc.

# $Id: thermodynamics.s,v 1.4 1996/09/04 19:10:03 ballan Exp $

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set exampels $env(ASCENDDIST)/models/examples
#
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/thermodynamics.lib;
READ FILE thermodynamics.asc;

COMPILE ut OF UNIFAC_test;
RUN ut.values;
RUN ut.reset;
SOLVE ut;

COMPILE pt OF Pitzer_test;
RUN pt.values;
RUN pt.reset;
SOLVE pt;

COMPILE rt OF Rackett_test;
RUN rt.values;
RUN rt.reset;
SOLVE rt;


