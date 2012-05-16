proc IVP.AWW.Integrator {qlfdid method relativeError maxNumberSolves xUnits initDeltaX maxDeltaX stopX} {

# qkfdid          stands for qualified id (this name occurs often in ASCEND
#                 tcl code) 
# method          must have value Am or Bdf and is used to set integration
#                 method
# relativeError   is the desired relative integration error, e.g., 1.0e-3.
# maxNumberSolves is max number of model solves the integrator is allowed
#                 to perform before forcibly stopping, e.g., 1000.
# xUnits          are the units in which all following parameters are given for
#                 the independent variable X, e.g., second.
# initDeltaX      is the initial value to use for the stepsize for X.
# maxDeltaX       is the maximum value for X the integrator should use when
#                 stepping.
# stopX           is the stopping value for X.

     global ascSolvStatVect;

# Solve initial point

     RUN $qlfdid.valuesForInitializing;
     RUN $qlfdid.specifyForInitializing;
     SOLVE $qlfdid.currentPt WITH QRSlv;
     if { !$ascSolvStatVect(converged) } {
         error "Initial point solve: Equations for model did not converge";
     }

# Prepare to take first integration step

     DELETE SYSTEM;
     RUN $qlfdid.valuesForStepping;
     RUN $qlfdid.specifyForStepping;

# Run one of the following to set the method

     switch $method {
         {Am}  -
         {Bdf} {}
         default {
     	     error "Expected Am or Bdf as method";
         }
     }
     RUN $qlfdid.setUseMethodTo$method;

# Set initial step size, integration error, stopping
# point

     ASSIGN $qlfdid.deltaX $initDeltaX $xUnits;
     ASSIGN $qlfdid.maxDeltaX $maxDeltaX $xUnits;
     ASSIGN $qlfdid.maxNominalSteppingError $relativeError;
     ASSIGN $qlfdid.stopX $stopX $xUnits;

     set numberSolves          1;
     set ascStopCondHit        0;
     set ascThisIsTheFinalStep 0;

# Integrate

     while {$numberSolves < $maxNumberSolves} {

         incr numberSolves;
         set ascPolyOrder [u_getval $qlfdid.usePolyOrder];
         set ascPolyOrderValue [lindex $ascPolyOrder 0];
         set ivp_steps $ascPolyOrderValue;

         for {set ivp_i 1} {$ivp_i <= $ivp_steps} {incr ivp_i} {
             if {[expr ($ascThisIsTheFinalStep == 0)]} {
                 if {[expr ($ascStopCondHit == 0)]} {
                     RUN $qlfdid.stepX;
                 }
                 SOLVE $qlfdid WITH QRSlv;
	         if {!$ascSolvStatVect(converged)} {
                     error "Model solution $numberSolves: Equations for model did not converge.";
                 }
             }
             if  {[expr ($ascThisIsTheFinalStep == 1)]} {
                 error "Model solution $numberSolves: STOP condition reached";
             }

             RUN $qlfdid.setStopConditions;
             set ascStopCondHit [lindex [u_getval $qlfdid.stopCondHit] 0];
             set ascThisIsTheFinalStep [lindex [u_getval $qlfdid.thisIsTheFinalStep] 0];

         }

         RUN $qlfdid.computeMaxNominalStepsForEachVariable;

     }

     error "Model solution $numberSolves: Maximum solutions reached. Stopping.";

}


proc getBoolean {varName} {
         # proc for getting value of boolean atoms
         qlfdid $varName
         set bval [inst atomvalue search]
         return $bval
}


proc getAttribute {varName attrName} {
         # proc for getting value of atom attributes
         # in the case of attributes with units, does not return units.
         qlfdid $varName
         set cnames "[inst child search]"
         set cindex [lsearch $cnames $attrName]
         if {$cindex < 0} {
                 error "No child named $attrName"
         }
         # children index from 1,not 0, so the list position is cindex++
         incr cindex
         set cdata [brow_child_list search $cindex VALUE]
         # modify the next lines if you want to return units also
         set cval [lindex [lindex $cdata 0] 2]
         return $cval
}

