/*
 *  Ascend Instance Tree Search Routines
 *  by Tom Epperly
 *  Created: 1/24/90
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: find.h,v $
 *  Date last modified: $Date: 1998/03/26 20:39:45 $
 *  Last modified by: $Author: ballan $
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
 *  Ascend Instance Tree Search Routines.
 *  <pre>
 *  When #including find.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "instance_enum.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *  </pre>
 */

#ifndef ASC_FIND_H
#define ASC_FIND_H

#include <compiler/value_type.h>

/**
 * Search error codes.
 * At present, there is a GREAT DEAL of insanity between find_errors and
 * evaluation_error (value_type.h). In particular there is a lot of idiocy
 * mapping undefined_instance <--> undefined_value which is just plain wrong
 * in most cases.
 */
enum find_errors {
  unmade_instance,      /**< Found an unmade instance (NULL child). */
  undefined_instance,   /**< Unstance in an expression is unknown child. */
  impossible_instance,  /**< Name cannot possibily exist(real error),often sets. */
  correct_instance      /**< Return value when everything went okay. */
};

extern int ListMode;
/**<
 *  Tells whether to evaluate a set strictly as a set i.e no order,
 *  or as a list, i.e., with order important.
 */

extern int EvaluatingSets;
/**<
 *  Tells whether the evaluation of a set is in place. Used for marking
 *  atoms as mutable or not.
 */

extern int g_DeclarativeContext;
/**<
 *  Tells whether declarative processing, the default = 0 , is in effect,
 *  or procedural processing as when doing initializations.
 *  Access this varible only by GetDeclarativeContext() and
 *  SetDeclarativeContext() below.
 */

#define FINDEBUG 0
#if (FINDEBUG==0)
/** retrieve the declarative context */
#define GetDeclarativeContext() g_DeclarativeContext
#define SetDeclarativeContext(n) g_DeclarativeContext = (n)
/**< set the declarative context */
#else
/** retrieve the declarative context */
#define GetDeclarativeContext() GetDeclarativeContextF()
#define SetDeclarativeContext(n) SetDeclarativeContextF((n),__FILE__,__LINE__)
/**< set the declarative context */
#endif
extern int GetDeclarativeContextF(void);
/**< retrieve the declarative context (for debugging) */
extern void SetDeclarativeContextF(int value, char *file, int line);
/**< set the declarative context (for debugging) */

extern CONST struct Instance *g_EvaluationContext;
/**<
 * Global variable used throughout semantic analysis to
 * indicate context of evaluation. Do Not reference this
 * directly, but use GetEvaluationContext() and
 * SetEvaluationContext() instead.
 */
extern struct for_table_t *g_EvaluationForTable;
/**<
 * Global variable used throughout semantic analysis to
 * indicate context of evaluation. Do Not reference this
 * directly, but use GetEvaluationForTable() and
 * SetEvaluationForTable() instead.
 */

#ifndef NDEBUG
#define EVALDEBUG 0 /**< 1 = contextnoisy, 2 = fornoisy, 3 = both */
#define GetEvaluationContext() GetEvaluationContextF()
#define GetEvaluationForTable() GetEvaluationForTableF()
#if EVALDEBUG
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
#define SetEvaluationContext(i) SetEvaluationContextF((i),__FILE__,__LINE__)
#else /* evaldebug 12 */
#define SetEvaluationContext(i) SetEvaluationContextF(i)
#endif /* evaldebug  12 */
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
#define SetEvaluationForTable(ft) SetEvaluationForTableF((ft),__FILE__,__LINE__)
#else /* evaldebug 23 */
#define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
#endif /* evaldebug  23 */
#else /* evaldebug */
#define SetEvaluationContext(i) SetEvaluationContextF(i)
#define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
#endif /* evaldebug */
#else /* ndebug */
#define GetEvaluationContext() g_EvaluationContext
#define SetEvaluationContext(i) g_EvaluationContext = (i)
#define GetEvaluationForTable() g_EvaluationForTable
#define SetEvaluationForTable(ft) g_EvaluationForTable = (ft)
#endif /* ndebug */

/*
 * Wrappers mainly for break point purposes of g_EvaluationContext
 * and g_EvaluationForTable.
 */
extern struct Instance *GetEvaluationContextF(void);
/**< retrieve the evaluation context (mainly for debugging) */

extern struct for_table_t *GetEvaluationForTableF(void);
/**< retrieve the evaluation for table (mainly for debugging) */

#if EVALDEBUG /* version printing file/line when setting globals */
extern void SetEvaluationContextF(CONST struct Instance *i
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
                                  ,char *file, int line
#endif /* evaldebug 13 */
                                 );
/**< set the evaluation context (mainly for debugging) */

extern void SetEvaluationForTableF(struct for_table_t *ft
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
                                  ,char *file, int line
#endif /* evaldebug 23 */
                                  );
/**< set the evaluation for table (mainly for debugging) */

#else /* evaldebug */
extern void SetEvaluationContextF(CONST struct Instance *i);
/**< set the evaluation context (mainly for debugging) */
extern void SetEvaluationForTableF(struct for_table_t *ft);
/**< set the evaluation for table (mainly for debugging) */
#endif /* evaldebug*/

extern struct value_t InstanceEvaluateName(CONST struct Name *nptr);
/**<
 *  <!--  struct value_t InstanceEvaluateName(nptr)                    -->
 *  <!--  struct Name *nptr;                                           -->
 *  This evaluates the name in the context given by EvaluationContext.
 *  This must be set before the InstanceEvaluateName call.  Note since
 *  this is a global variable you cannot evaluate names in more than
 *  one context simultaneously.<br><br>
 *
 *  If EvaluationForTable is non-NULL, the for table will be checked before
 *  the instance tree.
 */

extern struct value_t InstanceEvaluateSatisfiedName(CONST struct Name *nptr,
                                                    double tol);
/**<
 *  <!--  struct value_t InstanceEvaluateName(nptr,tol)                -->
 *  <!--  struct Name *nptr;                                           -->
 *  <!--  double tol;                                                  -->
 *
 *  This functionis specially to evaluate name of relations or logical
 *  relations included in SATISFIED expressions.
 *  This evaluates the name in the context given by EvaluationContext.
 *  This must be set before the InstanceEvaluateName call.  Note since
 *  this is a global variable you cannot evaluate names in more than
 *  one context simultaneously.
 *  If EvaluationForTable is non-NULL, the for table will be checked before
 *  the instance tree.
 */

extern struct gl_list_t *FindInstances(CONST struct Instance *i,
                                       CONST struct Name *n,
                                       enum find_errors *err);
/**<
 *  <!--  struct gl_list_t *FindInstances(i,n,err)                     -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  CONST struct Name *n;                                        -->
 *  <!--  enum find_errors *err;                                       -->
 *  Return the list of instances specified by n.  If this returns NULL,
 *  it indicates that it couldn't find the name.  Check err to discover why.
 */

#endif /* ASC_FIND_H */

