
# This file is part of the Ascend modeling library to
# demonstrate petlyuk.asc and is released under the GNU
# Public License as noted at the beginning of petlyuk.asc.

# $Id: petlyuk.s,v 1.4 1996/09/04 19:09:55 ballan Exp $


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
READ FILE petlyuk.asc;

COMPILE p OF petlyuk;
BROWSE p;
RUN p.values;
RUN p.clear;
RUN p.relative_volatility;
RUN p.constant_overflow;
SOLVE {p} WITH QRSlv;

REFINE {p.feed['middle'].state} TO equilibrium_mixture;
RUN p.clear;
RUN p.equilibrium;
RUN p.adiabatic;
SOLVE {p} WITH QRSlv;
