# This file is part of the Ascend modeling library to
# demonstrate _tests.asc and is released under the GNU
# Public License as noted at the beginning of simple_pot.asc.
# $Id: simple_pot.s,v 1.11 1998/01/27 14:51:41 ballan Exp $

set examples $env(ASCENDDIST)/models/examples
source $examples/set_intervals.tcl

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries

READ FILE $libraries/ivpsystem.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/lsode.lib;
READ FILE $libraries/plot.lib;
READ FILE $examples/simple_pot.asc;

COMPILE pot OF boiling_pot;

set_int 100 1 {s};
BROWSE {pot};
RUN {pot.values};
RUN {pot.specify};
SOLVE {pot} WITH QRSlv;
INTEGRATE {pot} WITH BLSODE;

# plotting the results:
#Now, open the ascplot Tool
#load y.dat, 
#select that data set,
#select all 3 vars
#move right to plot list with the  >>,
#hit execute.viewplot

# or just run the next statement
ASCPLOT y.dat
# when done tweaking the graph, run this statement
# or just push the close button on the ascplot file menu.
ASCPLOT CLOSE;




