/*
 *  Utility functions for Ascend
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: checkdim.h,v $
 *  Date last modified: $Date: 1997/07/18 11:48:39 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Programming System
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph James Zaher
 *  Copyright (C) 1994 Benjamin Andrew Allan, Joseph James Zaher
 *
 *  The Ascend Programming System is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Programming System is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  This module defines the dimensionality derivation and some other
 *  auxillaries for Ascend.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "compiler/dimen.h"
 *                #include "solver/mtx.h"
 *                #include "solver/linsol.h"
 *                #include "solver/var.h"
 *                #include "solver/rel.h"
 *                #include "solver/slv_client.h"
 *  </pre>
 *  @todo checkdim.h appears to be a work in progress - complete or remove.
 */

#ifndef chkdim__already_included
#define chkdim__already_included

#define REIMPLEMENT 0

/* See relman.h for the definition of list of relations/variables */

typedef struct chkdim_system_structure {
   linsol_system_t sys;
   mtx_matrix_t mtx;              /**< Coefficient matrix                 */
   real64 *(rhs[NUM_DIMENS]);     /**< RHS's (one for each fundamental
                                       dimension)                         */
   struct rel_relation **rlist;   /**< List of relations corresponding to
                                       each equation in the linear system */
   struct var_variable **vlist;   /**< List of (wild) variables corresponding
                                       to each variable in the linear
                                       system                             */
   int rhs_cap;                   /**< Capacity of the RHS arrays         */
   int rlist_len,vlist_len;       /**< Length of rlist and vlist resp.    */
   int rlist_cap,vlist_cap;       /**< Capacity of rlist and vlist resp.  */
} chkdim_system_t;

#if REIMPLEMENT

extern chkdim_create_system();
/**<
 *  <!--  chkdim_create_system(chk,rlist)                              -->
 *  <!--  chkdim_system_t *chk;                                        -->
 *  <!--  struct rel_relation **rlist;                                 -->
 *
 *  Initializes the structure and appends the relations appearing in rlist
 *  in the sense of chkdim_append_rel().
 */

extern chkdim_destroy_system();
/**<
 *  <!--  chkdim_destroy_system(chk)                                   -->
 *  <!--  chkdim_system_t *chk;                                        -->
 *
 *  Destroys everything in the structure.  The structure itself is not
 *  deallocated since chkdim_create_system() did not allocate it.
 */

extern chkdim_append_expr();
extern chkdim_append_rel();
/**<
 *  <!--  chkdim_append_expr(chk,expr)                                 -->
 *  <!--  chkdim_append_rel(chk,rel)                                   -->
 *  <!--  chkdim_system_t *chk;                                        -->
 *  <!--  expr_t expr;                                                 -->
 *  <!--  struct rel_relation *rel;                                    -->
 *
 *  Adds the given expression/relation (i.e. all dimensional equations
 *  implied by the expression/relation) to the system of dimensional
 *  equations.  In the case of expressions, the dimensional equations
 *  are tagged with the enclosing relation (there always is one in ascend
 *  version).
 */

extern chkdim_assign_dimensions();
/**<
 *  <!--  chkdim_assign_dimensions(chk,p)                              -->
 *  <!--  chkdim_system_t *chk;                                        -->
 *  <!--  slv_parameters_t *p;                                         -->
 *
 *  Solves the dimensional equations and assigns the dimensions to the
 *  wild-dimensioned variables as directed by the solution, writing the
 *  assignments to p->output.more_important.  No regard is given to
 *  consistency.
 */

extern int chkdim_check_dimensions();
/**<
 *  <!--  nerrs = chkdim_check_dimensions(chk,p)                       -->
 *  <!--  int nerrs;                                                   -->
 *  <!--  chkdim_system_t *chk;                                        -->
 *  <!--  slv_parameters_t *p;                                         -->
 *
 *  Solves the dimensional equations and checks the solution for
 *  consistency, writing messages to p->output.more_important.  Returns
 *  the number of errors (non-zero residuals).
 */

#else

#define chkdim_create_system(a,b)     0
#define chkdim_destroy_system(a)      0
#define chkdim_append_expr(a,b)       0
#define chkdim_append_rel(a,b)        0
#define chkdim_assign_dimensions(a,b) 0
#define chkdim_check_dimensions(a,b)  0

#endif /* REIMPLEMENT  */

#endif  /* chkdim__already_included  */

