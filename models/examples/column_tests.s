
# This file is part of the Ascend modeling library to
# demonstrate column_tests.asc and is released under the GNU
# Public License as noted at the beginning of column_tests.asc.

# $Id: column_tests.s,v 1.6 1996/11/08 17:06:20 rv2a Exp $

#	column_tests.s by Robert S. Huss
#
#	Script to run column examples found in column_tests.lib.
#	First test runs a two-component, methanol water column
#	through the basic column models in column.lib.  Since this
#	is a binary mixture, I added a model to do Mccabe Thiele plot.
#	For multiple components, plots of the type shown by plotx
#	are needed.  
#	The second test runs the same system, but uses Murphree
#	Vapor Efficiencies.  The Mccabe Thiele PLOT {doesn't show
#	an actual equilibrium line for this example.  It only
#	shows the pseudo-equilibrium.  But if you compare
#	plots with lower and higher efficiencies, you can see
#	that the equilibrium curve moves.  
#
#	This script can be run through all at once, but I
#	recommend doing the commands one line at a time to 
#	observe what ASCEND is doing.  

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/H_G_thermodynamics.lib;
READ FILE $libraries/plot.lib;
READ FILE $libraries/stream.lib;
READ FILE $libraries/flash.lib;
READ FILE $libraries/column.lib;
READ FILE column_tests.asc;

# two component column example 
COMPILE {tc} OF {td_test_column1};
BROWSE {tc};
RUN {tc.values};
RUN {tc.reset};
RUN {tc.scale};
SOLVE {tc};
PLOT {tc.plotx} temp1.plot;
SHOW LAST;
PLOT {tc.mcplot} temp2.plot;
SHOW LAST;

REFINE {tc.col} TO {equilibrium_column};
RUN {tc.reset};
RUN {tc.scale};
SOLVE {tc};
PLOT {tc.plotx} temp1.plot;
SHOW LAST;
PLOT {tc.mcplot} temp2.plot;
SHOW LAST;
RUN {tc.col.heat_balance};
RUN {tc.col.zero_Q};
SOLVE {tc};
PLOT {tc.plotx} temp1.plot;
SHOW LAST;
PLOT {tc.mcplot} temp2.plot;
SHOW LAST;

# two-component example with murphree efficiencies 


COMPILE {tc2} OF {td_test_column1};
BROWSE {tc2};
RUN {tc2.values};
RUN {tc2.reset};
RUN {tc2.scale};
SOLVE {tc2};
PLOT {tc2.plotx} temp1.plot;
SHOW LAST;
PLOT {tc2.mcplot} temp2.plot;
SHOW LAST;
# Example using Murphree models 
REFINE {tc2} TO {murph1};
RUN {tc2.reset};
SOLVE {tc2};
PLOT {tc2.plotx} temp1.plot;
SHOW LAST;
PLOT {tc2.mcplot} temp2.plot;
SHOW LAST;
RUN {tc2.col.heat_balance};
RUN {tc2.col.reduce_Q};
SOLVE {tc2};
RUN {tc2.col.zero_Q};
SOLVE {tc2};
PLOT {tc2.plotx} temp1.plot;
SHOW LAST;
PLOT {tc2.mcplot} temp2.plot;
SHOW LAST;
RUN {tc2.increase_murph};
SOLVE {tc2};
RUN tc2.murph_plot_values;
PLOT {tc2.plotx} temp1.plot;
SHOW LAST;
PLOT {tc2.mcplot} temp2.plot;
SHOW LAST;









