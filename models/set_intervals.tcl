# $Revision: 1.1 $
# $Date: 1997/09/26 16:40:22 $
# $Author: ballan $
# Interval list creation functions
# Copyright 1996 Benjamin Allan (ballan@edrc.cmu.edu)
#
# This file defines several functions for creating lists of
# numbers and passing them to the integration interface in
# ASCEND 4. These lists are the sample points at which we
# wish to record values during integration.
#
# We have functions for:
# - evenly spaced intervals starting at 0.0
#	(set_int)
# - evenly spaced intervals starting at 0.0 with
#   intermediate sample points at the 2nd order lagrange locations
#	(set_lagrangeint)
#
# These functions should take a starting point instead of assuming 0 but don't.
#
# We want functions for:
#  logarithmically spaced points, starting at some number and ending
#  at some much larger number, for sampling growing functions.
#
#  points spaced at multiple frequencies
#
#  building lists of sample times
#
#  taking a list, inserting a small initial sample to trick the
#  integrator into thinking the problem stiff, and then generating
#  whatever the desired sampling schedule is after that.
#
# Bugs:
#  Poor design. There should be a suite of commands for building
#  sample lists up, then a single command that takes such a list
#  and sends it to the integrator.
#
# proc set_int {nsteps step units}
#-------------------------------------------------------------------------
# this proc sets intervals incrementally for blsode
#-------------------------------------------------------------------------
proc set_int {nsteps step units} {
    set SetIntervals "integrate_set_samples $units";
    for {set i 0} {$i < $nsteps} {incr i} {
	lappend SetIntervals [expr $i*$step];
    }
    eval $SetIntervals;
}

#
# proc set_lagrangeint {nsteps step units}
#-------------------------------------------------------------------------
# this proc sets intervals incrementally for blsode
# adding the lagrange nodes within each regularly spaced step
#-------------------------------------------------------------------------
proc set_lagrangeint {nsteps step units} {
    set SetIntervals "integrate_set_samples $units";
    for {set i 0} {$i < $nsteps} {incr i} {
	lappend SetIntervals [expr $i*$step];
        lappend SetIntervals [expr $i*$step+0.21132486*$step];
        lappend SetIntervals [expr $i*$step+0.7886751*$step];
    }
    eval $SetIntervals;
}

