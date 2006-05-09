# Ben Allan, Livermore, California, Feb 10, 2004
# GPL
#
# This feature is not loaded in ascend by default,
# as it breaks the design of the GUI in fundamental ways--
# it deprives the interactive user of immediate feedback.
#
# A new script function to handle re-solving efficiently
# for QRSLV.
# This function can be used to re-solve a model already
# in the solver if and only if:
# 1) The model has been solved (at least partially) once.
# 2) The .fixed flags on variables and .included flags on relations
#    have not been changed. (Thus the matrix structure is unchanged).
# 3) The choice of objective is not changed.
#
# We are allowed to change:
#    Bounds on variables (.lower_bound, .upper_bound)
#    Scaling values on variables (.nominal)
#    Values of input variables (those with .fixed == true)
#    Numerical solver controls like convergence tolerances.
#    
# Side effects:
# This function updates Tcl variables related to the solver,
# but does not update the GUI. To update the GUI, one will
# have to hit the GUI Solve button or call the script SOLVE again.
#
# Unlike other solver processes, the tracing functions
# are not used, in the interest of speed. (entertrace, leavetrace
# commented out). 
# The GUI interrupt is not going to work.
# The solver retains control for all iterations, with no tcl
# in between blocks.

#
# proc RESOLVE_NO_GUI {}
#----------------------------------------------------------------------------
# Execute resolve internals  
#----------------------------------------------------------------------------
proc RESOLVE_NO_GUI {} {
# entertrace
  if {![slv_checksys]} { return }
# needs better sanity checking and interrupt checking.
  global ascSolvVect ascSolvStatVect ascSolv32767Vect ascSolv3Vect
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  if {[slv_checksys]} {
    set ascSolvStatVect(ready2solve) 1
  }
  Solve_Downdate_ParmBox
  slv_resolve
  slv_solve
  Solve_Update_StatVect
  puts "converged= $ascSolvStatVect(converged)"
  puts "max err= $ascSolvStatVect(maxblockerr)"
#leavetrace
}

#
# Force an update of the gui
# after the resolve is all over.
#
proc RESOLVE_GUI {} {
  RESOLVE_NO_GUI
  Solve_Update_StatusBox 1
}

#
# Example of intended use (pseudo code):
# 1) SOLVE sim;
# 2) set someFixedVariable to a new value;
# RESOLVE_NO_GUI
# if {! $ascSolvStatVect(converged) } { die } else { do something else }
