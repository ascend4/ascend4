#include <stdlib.h>
#include "ascendEmbedded.h"

void die(
	 char * message)
{
  printf ("%s\n", message);
  exit (1);
}


/**
   Integrates DAE models using "DASSYL" like concepts

   @param qkfdid          stands for qualified id (this name occurs often in ASCEND
                          tcl code) 
   @param method          must have value Am or Bdf and is used to set integration
                          method
   @param relativeError   is the desired relative integration error, e.g., 1.0e-3.
   @param maxNumberSolves is max number of model solves the integrator is allowed
                          to perform before forcibly stopping, e.g., 1000.
   @param xUnits          are the units in which all following parameters are given for
                          the independent variable X, e.g., second.
   @param initDeltaX      is the initial value to use for the stepsize for X.
   @param maxDeltaX       is the maximum value for X the integrator should use when
                          stepping.
   @param stopX           is the stopping value for X.
*/

  /*  *** proc IVP.AWW.Integrator {qlfdid method relativeError maxNumberSolves xUnits initDeltaX maxDeltaX stopX} 
 */

void IVP_AWW_Integrator(
			char * qlfdid, 
			enum IntegrationMethod method, 
			double relativeError,
			int maxNumberSolves,
			char * xUnits,
			double initDeltaX,
			double maxDeltaX,
			double stopX)

{

   A4ptr ascendWorld = NULL;
   int status = 0;


   ascendWorld = createAscendEnvironment();


   /* ***
      READ FILE "ivpStep.testModel.AWW.a4c";
      COMPILE int OF ivpTest;
   */

   status = readAscendFile(ascendWorld,"ivpStep.testModel.AWW.a4c");
   status = compileAscendInstance(ascendWorld, "int", "ivpTest");


   /* Solve initial point
    */

   /* ***
     RUN $qlfdid.valuesForInitializing;
     RUN $qlfdid.specifyForInitializing;
   */

   status = runAscendMethod(ascendWorld, "int.valuesForInitializing");
   status = runAscendMethod(ascendWorld, "int.specifyForInitializing");





  /* ***
     SOLVE $qlfdid.currentPt WITH QRSlv;
  */

   SlvSystemPtr slvObj = NULL;
   slvObj = createAscendSolverSystem(ascendWorld, "QRSlv", "int.currentPt");
   status = solveAscendSlvSystem(slvObj);


  /* ***
     global ascSolvStatVect;
     if { !$ascSolvStatVect(converged) } {
         error "Initial point solve: Equations for model did not converge";
     }
   */

   int isConverged = 0;
   isConverged = getSolverBooleanResult(slvObj, "converged");
   if ( !isConverged ) 
     {
       die ("Initial point did not converge for model");
     }




   /* Prepare to take first integration step
    */

   /* ***
     DELETE SYSTEM;
   */

   destroyAscendSolverSystem(ascendWorld, slvObj);
   slvObj = NULL;

   /* ***
     RUN $qlfdid.valuesForStepping;
     RUN $qlfdid.specifyForStepping;
   */

   status = runAscendMethod(ascendWorld, "int.valuesForStepping");
   status = runAscendMethod(ascendWorld, "int.specifyForStepping");



   /* Run one of the following to set the method
    */

   /* ***
     switch $method {
         {Am}  -
         {Bdf} {}
         default {
     	     error "Expected Am or Bdf as method";
         }
     }
     RUN $qlfdid.setUseMethodTo$method;
   */


   switch (method)
     {
     case AM:
     case BDF:
       break;
     default:
       die("Integration method not AM or BDF");
     }


   /* Set initial step size, integration error, stopping
      point
   */

   /* ***
     ASSIGN $qlfdid.deltaX $initDeltaX $xUnits;
     ASSIGN $qlfdid.maxDeltaX $maxDeltaX $xUnits;
     ASSIGN $qlfdid.maxNominalSteppingError $relativeError;
     ASSIGN $qlfdid.stopX $stopX $xUnits;

     set numberSolves          1;
     set ascStopCondHit        0;
     set ascThisIsTheFinalStep 0;
   */

   status = setRealValue(ascendWorld, "int.deltaX", initDeltaX, xUnits);
   status = setRealValue(ascendWorld, "int.maxDeltaX", maxDeltaX, xUnits);
   status = setRealValue(ascendWorld, "int.maxNominalSteppingError", relativeError, xUnits);
   status = setRealValue(ascendWorld, "int.stopX", stopX, xUnits);

   int numberSolves = 1;
   int ascStopCondHit = 0;
   int ascThisIsTheFinalStep = 0;

   /* Integrate
    */


   /* ***
     while {$numberSolves < $maxNumberSolves} {
   */

   while (numberSolves < maxNumberSolves)
     {

       /* ***
         incr numberSolves;
         set ascPolyOrder [u_getval $qlfdid.usePolyOrder];
         set ascPolyOrderValue [lindex $ascPolyOrder 0];
         set ivp_steps $ascPolyOrderValue;
       */

       int ascPolyOrderValue;
       int ivp_steps;



       numberSolves++;
       status = getIntValue(ascendWorld, "int.usePolyOrderValue", &ascPolyOrderValue);
       ivp_steps = ascPolyOrderValue;

       /* ***
         for {set ivp_i 1} {$ivp_i <= $ivp_steps} {incr ivp_i} {
       */

       int ivp_i;
       for (ivp_i = 1; ivp_i <= ivp_steps; ivp_i++)
	 {

	   /* ***
             if {[expr ($ascThisIsTheFinalStep == 0)]} {
	   */

	   if (!ascThisIsTheFinalStep)
	     {

	       /* ***
                 if {[expr ($ascStopCondHit == 0)]} {
	       */

	       if (!ascStopCondHit)
		 {

		   /* ***
                     RUN $qlfdid.stepX;
		   */

		   status = runAscendMethod(ascendWorld, "int.stepX");
                 }

	       /* ***
 	 	 SOLVE $qlfdid WITH QRSlv;
	       */

#if 0
******************************
  /* ***
     SOLVE $qlfdid.currentPt WITH QRSlv;
  */

   SlvSystem slvObj = NULL;
   slvObj = createAscendSolverSystem(ascendWorld, "QRSlv", "int.currentPt");
   status = solveAscendSlvSystem(slvObj);


  /* ***
     global ascSolvStatVect;
     if { !$ascSolvStatVect(converged) } {
         error "Initial point solve: Equations for model did not converge";
     }
   */

   int converged = 0;
   isConverged = getSolverBooleanResult(slvObj, "converged");
   if ( !isConverged ) 
     {
       die ("Initial point did not converge for model");
     }




   /* Prepare to take first integration step
    */

   /* ***
     DELETE SYSTEM;
   */

   destroyAscendSolverSystem(ascendWorld, slvObj);
   slvObj = NULL;

*********************************

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
#endif /* end of big deleted code */

}

}}
