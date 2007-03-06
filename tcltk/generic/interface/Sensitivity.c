/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
 * This file attempts to implement the extraction of dy_dx from
 * a system of equations. If one considers a black-box where x are
 * the input variables, u are inuut parameters, y are the output
 * variables, f(x,y,u) is the system of equations that will be solved
 * for given x and u, then one can extract the sensitivity information
 * of y wrt x.
 * One crude but simple way of doing this is to finite-difference the
 * given black box, i.e, perturb x, n\subx times by delta x, resolve
 * the system of equations at the new value of x, and compute
 * dy/dx = (f(x\sub1) - f(x\sub2))/(x\sub1 - x\sub2).
 * This is known to be very expensive.
 *
 * The solution that will be adopted here is to formulate the Jacobian J of
 * the system, (or the system is already solved, to grab the jacobian at
 * the solution. Develop the sensitivity matrix df/dx by exact numnerical
 * differentiation; this will be a n x n\subx matrix. Compute the LU factors
 * for J, and to solve column by column to : LU dz/dx = df/dx. Here
 * z, represents all the internal variables to the system and the strictly
 * output variables y. In other words J = df/dz.
 *
 * Given the solution of the above problem we then simply extract the rows
 * of dz/dx, that pertain to the y variables that we are interested in;
 * this will give us dy/dx.
*//*
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.31 $
 *  Version control file: $RCSfile: Sensitivity.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
 */

#include <math.h>
#include <tcl.h>

#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <general/list.h>

#include <linear/mtx.h>
#include <linear/linutils.h>

#include <system/calc.h>
#include <system/relman.h>
#include <system/slv_client.h>
#include <system/system.h>

#include <packages/sensitivity.h>

#include "HelpProc.h"
#include "Sensitivity.h"
#include "HelpProc.h"
#include "SolverGlobals.h"
#include <general/mathmacros.h>

#ifndef lint
static CONST char SensitivityID[] = "$Id: Sensitivity.c,v 1.31 2003/08/23 18:43:08 ballan Exp $";
#endif

#define DOTIME FALSE
#define DEBUG 1

#ifdef THIS_MAY_BE_UNUSED_CODE
static real64 *zero_vector(real64 *vec, int size)
{
  int c;
  for (c=0;c<size;c++) {
    vec[c] = 0.0;
  }
  return vec;
}
#endif

/*
 *********************************************************************
 * The following bit of code does the computation of the matrix
 * dz/dx. It accepts a slv_system_t and a list of 'input' variables.
 * The matrix df/dx now exists and sits to the right of the main
 * square region of the Jacobian. We will do the following in turn
 * for each variable in this list:
 *
 * 1) 	Get the variable index, and use it to extract the required column
 *    	from the main gradient matrix, this will be stored in a temporary
 *    	vector.
 * 2) 	We will then clear the column in the original matrix, as we want to
 *	store the caluclated results back in place.
 * 3)	Add the vector extracted in 1) as rhs to the main matrix.
 * 4)	Call linsol solve on this rhs to yield a solution which represents
 * 	a column of dx/dx.
 * 6)	Store this vector back in the location cleared out in 2).
 * 7)	Goto 1.
 *
 * At the end of this procedure we would have calculated all the columns of
 * dz/dx, and left them back in the main matrix.
 *********************************************************************
 */

int Asc_MtxNormsCmd(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  slv_system_t sys;
  linsolqr_system_t linqr_sys = NULL;
  mtx_matrix_t mtx;
  mtx_region_t reg;
  double norm;
  int solver;

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if (argc!=1) {
    Tcl_SetResult(interp, "wrong # args: Usage __mtx_norms", TCL_STATIC);
    return TCL_ERROR;
  }
  sys = g_solvsys_cur;
  if (sys==NULL) {
    Tcl_SetResult(interp, "__mtx_norms called with slv_system", TCL_STATIC);
    return TCL_ERROR;
  }
  solver = slv_get_selected_solver(sys);
  switch(solver) {
  default:
  case 3:
  case 5:
    linqr_sys = slv_get_linsolqr_sys(sys);
    mtx = linsolqr_get_matrix(linqr_sys);
    reg.col.low = reg.row.low = 0;
    reg.col.high = reg.row.high = mtx_symbolic_rank(mtx);
    norm = linutils_A_1_norm(mtx,&reg);
    FPRINTF(stderr,"A_1_norm = %g\n",norm);
    norm = linutils_A_infinity_norm(mtx,&reg);
    FPRINTF(stderr,"A_infinity_norm = %g\n",norm);
    norm = linutils_A_Frobenius_norm(mtx,&reg);
    FPRINTF(stderr,"A_Frobenius_norm = %g\n",norm);
    norm = linutils_A_condqr_kaa(linqr_sys,mtx,NULL);
    FPRINTF(stderr,"A_condition # = %g\n",norm);
    break;
  }
  return TCL_OK;
}
