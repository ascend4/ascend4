/*
 *  Logical Relation Manipulator Module
 *  Created: 04/97
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: logrelman.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:41 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
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
 *
 */

/** @file
 *  Logical Relation Manipulator Module.
 *  <pre>
 *  Contents:     Logical Relation manipulator module
 *
 *  Authors:      Vicente Rico-Ramirez based on relman.[ch]
 *
 *  Dates:        04/97 - original version
 *
 *  Description:  This module will provide supplemental operations for
 *                lofical relations such as evaluation.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "mtx.h"
 *                #include "discrete.h"
 *                #include "logrel.h"
 *  </pre>
 */

#ifndef logrelman__already_included
#define logrelman__already_included

extern void logrelman_get_incidence(struct logrel_relation *lrel,
                                    dis_filter_t *filter,
                                    mtx_matrix_t matrix);
/**<
 *  <!--  logrelman_get_incidence(lrel,filter,matrix)                  -->
 *  <!--  struct logrel_relation *lrel;                                -->
 *  <!--  dis_filter_t *filter;                                        -->
 *  <!--  mtx_matrix_t matrix;                                         -->
 *
 *  Upon return, coefficient (logrel_n,disvar_n)(using original row and
 *  column numbers) is non-zero if and only if the logical relation lrel
 *  with index logrel_n depends on a discrete variable with index disvar_n.
 */

extern int32 logrelman_eval(struct logrel_relation *lrel, int32 *status);
/**<
 *  <!--  logresidual = logrelman_eval(lrel,status)                    -->
 *  <!--  struct logrel_relation *lrel;                                -->
 *  <!--  int32 *status;                                               -->
 *  <!--  int32 logresidual;                                           -->
 *
 *  The residual of the logical relation is calculated and returned.
 *  In addition to returning the residual, the residual field of the
 *  relation is updated. The status value can be monitored to
 *  determine if any calculation errors were encountered.  It will be set
 *  0 if a calculation results in an error.
 *  The logresidual field of the logical relation is not updated when
 *  an error occurs.
 */

extern boolean logrelman_calc_satisfied(struct logrel_relation *lrel);
/**<
 *  <!--  satisfied = logrelman_calc_satisfied(lrel)                   -->
 *  <!--  boolean satisfied;                                           -->
 *
 *  <!--  logrelman_calc_satisfied:                                    -->
 *  Returns TRUE or FALSE depending on whether the logical relation, whose
 *  boolean residual has been previously calculated, is satisfied based on
 *  the value stored in the residual field.  The satisfied field of the
 *  logical relation is also updated.
 */

extern int32 *logrelman_directly_solve(struct logrel_relation *lrel,
                                       struct dis_discrete *solvefor,
                                       int *able, int *nsolns, int perturb,
                                       struct gl_list_t *insts);
/**<
 *  <!--  solution_list = logrelman_directly_solve(                    -->
 *  <!--               lrel,solvefor,able,nsolns,perturb,insts)        -->
 *  <!--  int32 *solution_list;                                        -->
 *  <!--  struct logrel_relation *lrel;                                -->
 *  <!--  struct dis_discrete *solvefor;                               -->
 *  <!--  int *able;                                                   -->
 *  <!--  int *nsolns;                                                 -->
 *  <!--  int perturb;                                                 -->
 *  <!--  struct gl_list_t *insts;                                     -->
 *
 *  Attempts to solve the given logical equation for the given variable.
 *  If this function is able to determine the solution set, then *able
 *  is set to TRUE and a newly allocated solution list is returned:
 *  *nsolns will be set to the length of this array.
 *  Otherwise *able is FALSE and NULL is returned. NULL may also be
 *  returned if the solution set is empty.
 *  The flag perturb and the gl_list are used to change the truth
 *  value of some boundaries. This is sometimes useful in
 *  conditional modeling.
 */

extern char *logrelman_make_string_infix(slv_system_t sys,
                                         struct logrel_relation *lrel,
                                         int style);
/**< See discussion under logrelman_make_string_postfix(). */
extern char *logrelman_make_string_postfix(slv_system_t sys,
                                           struct logrel_relation *lrel,
                                           int style);
/**<
 *  <!--  string = logrelman_make_string_infix(sys,lrel,style)         -->
 *  <!--  string = logrelman_make_string_postfix(sys,lrel,style)       -->
 *  <!--  int style = FALSE;                                           -->
 *  <!--  char *string;                                                -->
 *  <!--  struct logrel_relation *lrel;                                -->
 *
 *  Creates a sufficiently large string (you must free it when you're
 *  done with it) into which it converts a logical relation.
 *  The string will be terminated with a '\0' character.
 *  The name of a discrete var is context dependent, so you have to
 *  provide the slv_system_t from which you got the logical relation.
 */

#endif /*  logrelman__already_included */


