# This file is part of the Ascend modeling library to
# demonstrate distill.asc and is released under the GNU
# Public License as noted at the beginning of distill.asc.

# $Id: distill.s,v 1.4 1996/09/04 19:09:48 ballan Exp $

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
#
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/thermodynamics.lib;
READ FILE $libraries/flowsheet.lib;
READ FILE $libraries/plot.lib;
READ FILE distill.asc;

COMPILE d OF distill;
BROWSE d;
RUN d.values;
RUN d.clear;
RUN d.relative_volatility;
RUN d.constant_overflow;
SOLVE d;

REFINE {d.feed['side'].state} TO equilibrium_mixture;

RUN d.equilibrium;
RUN d.adiabatic;
SOLVE d;


