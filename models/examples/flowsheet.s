# This file is part of the Ascend modeling library to
# demonstrate flowsheet.asc and is released under the GNU
# Public License as noted at the beginning of flowsheet.asc.

# $Id: flowsheet.s,v 1.4 1996/09/04 19:09:51 ballan Exp $

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
#
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/thermodynamics.lib;
READ FILE $libraries/flowsheet.lib;
READ FILE flowsheet.asc;

COMPILE ft OF flash_test;
BROWSE ft;
RUN ft.values;
RUN ft.reset;
SOLVE ft;


