/*
 *  Interface to Karl Westerberg's Solver
 *  Tom Epperly
 *  Created: June, 1990
 *  Copyright (C) 1990 Thomas Guthrie Epperly
 *  Patched 1/94 for ASCEND3C -baa
 *  Only Solve is implemented in slv_interface.c
 *                            
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: slv_interface.h,v $
 *  Date last modified: $Date: 1997/07/18 12:17:10 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Interface to the Westerberg's SLV Solver.
 *  <pre>
 *  Requires      #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *                #include "compiler/instance_enum.h"
 *  </pre>
 *  @todo Clean junk out of solver/slv_interface.h.
 */

#ifndef slv_interface_module_loaded
#define slv_interface_module_loaded

extern void Solve(struct Instance *inst);
/**<
 *  This is the link that the command line interface should call.
 */

/* the rest of this is junk */
/* extern unsigned long NumberVars(); */
/*
 *  Returns the number of variables.
 */

/* extern unsigned long NumberEquations(); */
/*
 *  Returns the number of equations.
 */

/* extern unsigned long NumberJacobianElements(); */
/*
 *  Returns an upper bound on the number of non-zero elements in the
 *  Jacobian.
 */

/* extern void GetNominalValues(double *); */
/*
 *  void GetNominalValues(nom)
 *  double *nom;
 *  Return the nominal value of the variables from the Ascend data structure.
 *  If any of the variables have an undefined or zero nominal value,
 *  1 is used.
 */

/* extern void GetVarValues(double *,double *,double *); */
/*
 *  void GetVarValues(x,xl,xu)
 *  double *x,*xl,*xu;
 *  Get the values of the variables from the Ascend data structure and store
 *  them in the arrays x, xl, and xu which are assumed to be large enough.
 *  	Array		Description
 *  	x		Gets the value of the variable
 *  	xl		Gets a lower bound on the variable
 *  	xu		Gets an upper bound on the variable
 *  You may pass NULL parameters to this procedure, and it will not provide
 *  that information.
 */

/* extern void StoreVarValues(double *,double *,double *); */
/*
 *  void StoreXValues(x,xl,xu)
 *  double *x,*xl,*xu;
 *  Store the values found in the arrays in the Ascend data structure.
 *  	Array		Description
 *  	x		Holds the variable value
 *  	xl		Holds the lower bound on the variable value
 *  	xu		Holds the upper bound on the variable value
 *  You may pass NULL parameters to this procedure, and it will not store
 *  that information.
 */

/* extern void EvaluateFunction(double *); */
/*
 *  void EvaluateFunction(f);
 *  double *f;
 *  Evaluate the functions at the value current stored in the Ascend data
 *  structure.  The residual is stored in the Ascend data structure, and it
 *  is also return in the array f.  If f is NULL, this procedure will
 *  evaluate the function and the results only in the Ascend data structure.
 */

#define JACFUNC void (*)(int,int,double)
/**<
 *  Type definition for storing Jacobian elements.  The function definition
 *  is as follows:
 *
 *  void JacStore(row,col,value)
 *  int row,col;
 *  double value;
 * 
 *  @todo If still needed, should it be a typedef?
 */

#define SLOPEFUNC void (*)(int,int,double,double)
/**<
 *  Type definition for storing slope matrix elements.  The function
 *  definition is as follows:
 *
 *  void SlopeStore(row,col,low,high)
 *  int row,col;
 *  double low,high;
 *
 *  @todo If still needed, should it be a typedef?
 */

/* extern void EvaluateJac(double *,JACFUNC); */
/*
 *  void EvaluateJac(f,JacStore)
 *  double f[];
 *  void (*JacStore)(row,col,value);
 *  It is assumed that the Jacobian starts out empty and zero.
 */

/* extern void EvaluateAll(double *,double *,double *,double *,double *,
			JACFUNC,SLOPEFUNC); */
/*
 *  void EvaluateAll(f,fl,fu,JacStore,SlopeStore)
 *  double f[],
 *         fl[],
 *         fu[],
 *         flu[],
 *         fuu[];
 *  void (*JacStore)(row,col,value);
 *  void (*SlopeStore)(row,col,low,high);
 *  It is assumed that the Jacobian and slope matrix start out empty(the
 *  zero matrix).
 */

/* extern void EvaluateI(int,double *,double *,double *); */
/*
 *  void  EvaluateI(i,f,Jl,Ju)
 *  int i;
 *  double *f,*Jl,*Ju;
 *  This evaluates the i'th function at the value currently stored in the
 *  Ascend data structure.  The residual of the i'th function is stored in
 *  the Ascend data structure as well as in *f.
 *  
 *  It is assumed that Jl and Ju are filled with zeros on input!
 *  
 *  Outputs
 *  f		holds value of the function
 *  Jl		lower bound on the Jacobian for the i'th function(array)
 *  Ju		upper bound on the Jacobian for the i'th function(array)
 */

/* extern void VariableReport(FILE *,int); */
/*
 *  void VariableReport(f,level)
 *  FILE *f;
 *  int level;
 *  Print a report of all the variables in the system.  This prints the
 *  value of the variables currently found in the Ascend data structure.
 *  At this point level is ignored.
 *  
 *  level
 *  Always		Prints variable number, name and values.
 */

/* extern void FunctionReport(FILE *,int); */
/*
 *  void FunctionReport(f,level)
 *  FILE *f;
 *  int level;
 *  Print a report of all the equations found in the system.  This prints
 *  the residual of all the functions as currently found in the Ascend
 *  data structure.  This value could be inconsistent with the variable
 *  values because the residual is only updated by the evaluation routines.
 *  
 *  level
 *  Always		Prints equation number, name and residual.
 *   >= 1		Prints the equation
 */

#endif  /* slv_interface_module_loaded */

