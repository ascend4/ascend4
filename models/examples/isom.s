# This file is part of the Ascend modeling library to
# demonstrate _tests.asc and is released under the GNU
# Public License as noted at the beginning of isom.asc.

# $Id: isom.s,v 1.5 1996/09/04 19:09:53 ballan Exp $

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
#
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/H_G_thermodynamics.lib;
READ FILE $libraries/plot.lib;
READ FILE $libraries/integration.lib;
READ FILE isom.asc;

COMPILE isom1 OF isomerization1;
BROWSE {isom1};
RUN isom1.values;
RUN isom1.clear;
RUN isom1.specify;
SOLVE isom1;
INTEGRATE isom1;
RUN isom1.plot;
PLOT {isom1.graph};
SHOW LAST;
RUN {isom1.plot_enthalpy};
PLOT {isom1.enthalpy_graph} ;
SHOW LAST;

COMPILE isom2 OF isomerization2;
BROWSE {isom2};
RUN isom2.values;
RUN isom2.clear;
RUN isom2.specify;
SOLVE isom2;
RUN isom2.plot;
PLOT {isom2.graph};
SHOW LAST;
RUN isom2.temperature_plot;
PLOT {isom2.temperature_graph};
SHOW LAST;

ASSIGN {isom2.initial.state.T.fixed} FALSE;
slv_slv0_set_independent 768;
set ascSolv0Vect(rho) 400.0;
set ascSolv32767Vect(update_frequency) 1;
Solve_Downdate_ParmBox;


Solve_do_Optimize;
RUN isom2.plot;
PLOT {isom2.graph};
SHOW LAST;
RUN isom2.temperature_plot;
PLOT {isom2.temperature_graph};
SHOW LAST;

