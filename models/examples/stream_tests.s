
# This file is part of the Ascend modeling library to
# demonstrate stream_tests.asc and is released under the GNU
# Public License as noted at the beginning of stream_tests.asc.

# $Id: stream_tests.s,v 1.6 1997/06/09 19:39:22 ballan Exp $
#
#	stream_tests.s by Robert S. Huss
#
#	This script file runs the examples in stream_tests.asc.
#	These examples demonstrate the models in stream.asc,
#	as well as the characteristics of the thermodynamics library.
#
#	This script can be run completely through at once, except for
#            the resume compilation in the first model,  but I 
#	recommend doing the commands one line at a time to see
#	what ASCEND is doing.

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
#
READ FILE $libraries/system.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/H_G_thermodynamics.lib;
READ FILE $libraries/stream.lib;
READ FILE stream_tests.asc;

#  liquid stream test

COMPILE {test1} OF {test};
BROWSE {test1};
RUN {test1.values};
RUN {test1.reset};
RUN {test1.scale};
PROBE current {test1.stream} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
SOLVE {test1};

# use these three statements, or REFINE a, b, and c to your choice 
REFINE {test1.stream.data['a']} TO {acetone};
REFINE {test1.stream.data['b']} TO {chloroform};
REFINE {test1.stream.data['c']} TO {benzene};

REFINE {test1.stream} TO {liquid_stream};
RESUME;
RUN {test1.stream.reset};
SOLVE {test1.stream};

#   vapor stream test 


COMPILE {test2} OF {test};
BROWSE {test2};
RUN {test2.values};
RUN {test2.reset};
RUN {test2.scale};
PROBE new {test2.stream} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
SOLVE {test2};
REFINE {test2} TO {td_test};
REFINE {test2.stream} TO {vapor_stream};
RUN {test2.stream.reset};
SOLVE {test2.stream};

# vapor/liquid stream test 

COMPILE {test3} OF {vl_test};
BROWSE {test3};
RUN {test3.values};
RUN {test3.reset};
PROBE new {test3.stream} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
SOLVE {test3};
REFINE {test3.stream.state} TO {equilibrium_mixture};
RUN {test3.reset};
SOLVE {test3};
