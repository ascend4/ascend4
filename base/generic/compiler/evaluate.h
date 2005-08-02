/*
 *  Expression Evaluation Routine
 *  by Tom Epperly
 *  Created: 1/16/90
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: evaluate.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:08 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
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
 *  Expression Evaluation Routine
 *  <pre>
 *  When #including evaluate.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "list.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *  </pre>
 */

#ifndef __EVALUATE_H_SEEN__
#define __EVALUATE_H_SEEN__

extern struct value_t EvaluateSet(CONST struct Set *sptr,
                                  struct value_t (*EvaluateName)());
/**<
 *  <!--  struct value_t EvaluateSet(sptr,EvaluateName)                -->
 *  <!--  const struct Set *sptr;                                      -->
 *  <!--  struct value_t (*EvaluateName)();                            -->
 *
 *  <!--  POINTERS TO FUNCTIONS                                        -->
 *  <!--  struct value_t EvaluateName(nptr)                            -->
 *  <!--  struct Name *nptr;                                           -->
 *  Return the value of a Set structure, which just might be a set.
 */

extern struct value_t EvaluateExpr(CONST struct Expr *expr,
                                   CONST struct Expr *stop,
                                   struct value_t (*EvaluateName)());
/**<
 *  <!--  struct value_t EvaluateExpr(expr,stop,EvaluateName)          -->
 *  <!--  const struct Expr *expr,*stop;                               -->
 *  <!--  struct value_t (*EvaluateName)();                            -->
 *
 *  <!--  POINTERS TO FUNCTIONS                                        -->
 *  <!--  struct value_t EvaluateName(nptr)                            -->
 *  <!--  struct Name *nptr;                                           -->
 *  Return the value of a name structure.
 *  In most cases stop = NULL.  stop can be used to evaluate just part of
 *  an expression.  If (stop!=NULL) the calling program, must know the
 *  the stack is correctly balanced.
 */

extern struct gl_list_t *EvaluateNamesNeeded(CONST struct Expr *expr,
                                             CONST struct Expr *stop,
                                             struct gl_list_t *list);
/**<
 *  <!--  list = EvaluateNamesNeeded(expr,stop,list);                     -->
 *  <!--  const struct Expr *expr,*stop;                               -->
 *  <!--  (Analogous to EvaluateExpr, but the global EvaluatingSets is irrelevant). -->
 *
 *  Appends all the externally defined names found in expr to list.
 *  (Loop variables defined in the expression are ignored.)
 *  If expr is not well formed wrt loops, you may get odd results back.
 *  If the list passed in is NULL, creates a list first.
 *  The list returned may be empty, but should not be NULL.
 *  When e_var are encountered in the expression, DOES dissect the
 *  subscripts separately.<br><br>
 *
 *  Doesn't actually evaluate anything, so EvaluateName function
 *  is not required. The list will be the set of names you must have
 *  values for in order for the expression to have a chance of
 *  evaluating to a result, including for loop variables, but not
 *  dummy indices defined in the expression.
 *  No duplicate names will be added to the list, but if you send in
 *  a list with duplication already in it, we will not clean it up for you.
 */

extern struct gl_list_t *EvaluateNamesNeededShallow(CONST struct Expr *expr,
                                                    CONST struct Expr *stop,
                                                    struct gl_list_t *list);
/**<
 *  <!--  list = EvaluateNamesNeededShallow(expr,stop,list);           -->
 *  <!--  const struct Expr *expr,*stop;                               -->
 *  <!--  (Analogous to EvaluateExpr, but the global EvaluatingSets is irrelevant). -->
 *
 *  Appends all the externally defined names found in expr to list.
 *  (Loop variables defined in the expression are ignored.)
 *  If e is not well formed wrt loops, you may get odd results back.
 *  If the list passed in is NULL, creates a list first.
 *  The list returned may be empty, but should not be NULL.
 *  When e_var are encountered in the expression, DOES NOT dissect the
 *  subscripts separately.<br><br>
 *
 *  Doesn't actually evaluate anything, so no EvaluateName function
 *  is not required. The list will be the set of names you must have
 *  values for in order for the expression to have a chance of
 *  evaluating to a result, including for loop variables, but not
 *  dummy indices defined in the expression.
 *  No duplicate names will be added to the list, but if you send in
 *  a list with duplication already in it, we will not clean it up for you.
 */

extern struct gl_list_t
*EvaluateSetNamesNeeded(CONST struct Set *sptr, struct gl_list_t *list);
/**<
 *  <!--  list = EvaluateSetNamesNeeded(sptr,list)                     -->
 *  <!--  const struct Set *sptr;                                      -->
 *  <!--  extern struct gl_list_t *list;                               -->
 *  <!--  (Analogous to EvaluateExpr, but the global EvaluatingSets is irrelevant). -->
 *
 *  Appends all the externally defined names found in sptr to list.
 *  (Loop variables defined in the expression are ignored.)
 *  If sptr  is not well formed wrt loops, you may get odd results back.
 *  If the list passed in is NULL, creates a list first.
 *  The list returned may be empty, but should not be NULL.
 *  DOES investigate subscripts deeply for needed names.<br><br>
 *
 *  Doesn't actually evaluate anything, so no EvaluateName function
 *  is not required. The list will be the set of names you must have
 *  values for in order for the set to have a chance of
 *  evaluating to a result, including for loop variables, but not
 *  dummy indices defined in the expression.
 *  No duplicate names will be added to the list, but if you send in
 *  a list with duplication already in it, we will not clean it up for you.
 */

extern struct gl_list_t
*EvaluateSetNamesNeededShallow(CONST struct Set *sptr, struct gl_list_t *list);
/**<
 *  <!--  list = EvaluateSetNamesNeededShallow(sptr,list)              -->
 *  <!--  const struct Set *sptr;                                      -->
 *  <!--  extern struct gl_list_t *list;                               -->
 *  <!--  (Analogous to EvaluateExpr, but the global EvaluatingSets is irrelevant). -->
 *
 *  Appends all the externally defined names found in sptr to list.
 *  (Loop variables defined in the expression are ignored.)
 *  If sptr  is not well formed wrt loops, you may get odd results back.
 *  If the list passed in is NULL, creates a list first.
 *  The list returned may be empty, but should not be NULL.
 *  DOES NOT investigate subscripts deeply for needed names.<br><br>
 *
 *  Doesn't actually evaluate anything, so no EvaluateName function
 *  is not required. The list will be the set of names you must have
 *  values for in order for the set to have a chance of
 *  evaluating to a result, including for loop variables, but not
 *  dummy indices defined in the expression.
 *  No duplicate names will be added to the list, but if you send in
 *  a list with duplication already in it, we will not clean it up for you.
 */

extern void ClearRecycleStack(void);
/**<
 *  Call this function after shutting down the compiler.
 *
 *  The stack seldom gets beyond 2 deep, but we recycle the memory for
 *  it anyway because malloc and free are expensive and we use the
 *  stack millions of times.
 *  The recycler is initialized to empty by the ANSI C assumption,
 *  so there is no init function. This function can be used as a ReInit
 *  if so desired, but there is no need for it.
 *  There is an option inside evaluate.c that causes this function to
 *  report how many recycled stack elements it deallocates.
 */

#endif /* __EVALUATE_H_SEEN__ */

