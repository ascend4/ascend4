# This file is part of the Ascend modeling library to
# demonstrate optimization.asc and is released under the GNU
# Public License as noted at the beginning of optimization.asc.

# $Id: optimization.s,v 1.5 1998/04/26 23:42:54 ballan Exp $

DELETE TYPES
#set libraries $env(ASCENDDIST)/models/libraries
#set examples $env(ASCENDDIST)/models/examples
#
#READ FILE $libraries/system.lib;
#READ FILE $libraries/atoms.lib;
READ FILE optimization.asc;

COMPILE l OF lew;
BROWSE I;
RUN l.reset;
SOLVE l WITH rSQP;

COMPILE o OF opt;
BROWSE o;
RUN o.reset;
SOLVE o WITH rSQP;

COMPILE s OF sonic_flow;
BROWSE s;
RUN s.reset;
SOLVE s WITH rSQP;

COMPILE f OF find_Y;
BROWSE f;
RUN f.reset;
SOLVE f WITH rSQP;



