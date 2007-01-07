/*	ASCEND Solver Interface
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//** @file
	This file wraps up the routines for making a data structure
	containing system incidence matrix information, which can then be
	plotted or interactively queried using the GUI
*/

#ifndef ASC_INCIDENCE_H
#define ASC_INCIDENCE_H

#include <stdio.h>

/* The includes required just for this *header* to load */
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/var.h>
#include <solver/rel.h>

/* Note, removed 'own' attribute from mplotvars struct since it was unused */

typedef struct{
  int nprow;     /**< number of plotted rows  (== neqn always for now) */
  int neqn;      /**< number of relations. orgrows 0-neqns are relations */
  int npcol;     /**< number of plotted columns (== nfakevar) */
  int nvar;      /**< number of variables. orgcols 0-nvars-1 are variables */
  int nfakevar;  /**< nvar -nnincident + nslack (currently always == npcol) */
  int *pr2e;     /**< len nprow, mapping plot row -> eqn id */
  int *e2pr;     /**< len neqn, mapping eqn id -> plot row */
  int *pc2v;     /**< len npcol, mapping plot column -> var org col */
  int *v2pc;     /**< len nvar, mapping var org col -> plot col */
  char *vfixed;  /**< len nvar, fixed flags by var org col */

  struct var_variable **vlist; /** pointer to the array of variables in the system */
  struct rel_relation **rlist; /** pointer to the array of relations in the system */
} incidence_vars_t;

ASC_DLLSPEC int build_incidence_data(CONST slv_system_t sys, incidence_vars_t *pd);
/**<
	Fill in an mplotvars struct

	@return 0 if ok, 1 otherwise

	This function is in charge of setting the layout that will appear
	in the canvas (row and column ordering). right now it is doing
	its own arrangement based on the mtx structure associated with
	the solver. Eventually it would be much better if the solvers
	provided the display orderings themselves.
	Also this does a 1time query of all the var fixed flags.
*/


ASC_DLLSPEC void free_incidence_data(incidence_vars_t *pd);
/**<
	free memory in an mplotvars struct, if it is our memory
*/
#endif /* ASC_INCIDENCE_H */
