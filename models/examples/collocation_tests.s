# parameterized collocation test script
# by  Ken Tyner, May 15, 1997.

set lib $env(ASCENDDIST)/models/libraries
set ex $env(ASCENDDIST)/models/examples

DELETE TYPES;
READ FILE "$lib/system.lib";
READ FILE "$lib/atoms.lib";
READ FILE "$lib/components.par.lib";
READ FILE "$lib/propertyoptions.par.lib";
READ FILE "$lib/HGthermo.par.lib";
READ FILE "$lib/stream.par.lib";
READ FILE "$lib/flash.par.lib";
READ FILE "$lib/collocation.par.lib"

COMPILE tc OF mw_demo_column;
BROWSE {tc};

RUN {tc.mw_column.Column.reset_to_massbal};
RUN {tc.values};
SOLVE {tc.mw_column} WITH QRSlv;

RUN {tc.mw_column.Column.reset_to_fullthermo};
SOLVE {tc.mw_column} WITH QRSlv;

RUN {tc.mw_column.Column.reset_to_adiabatic};
RUN {tc.mw_column.Column.zero_Q};
SOLVE {tc.mw_column} WITH QRSlv;

