/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

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
	Ascend Instance Tree Search Routines.
	
	Requires:
	#include "utilities/ascConfig.h"
	#include "fractions.h"
	#include "compiler.h"
	#include "instance_enum.h"
	#include "dimen.h"
	#include "expr_types.h"
*//*
	by Tom Epperly
	Created: 1/24/90
	Last in CVS: $Revision: 1.12 $ $Date: 1998/03/26 20:39:45 $ $Author: ballan $
*/

#ifndef ASC_FIND_H
#define ASC_FIND_H

/**	addtogroup compiler Compiler
	@{
*/

/**
 * Search error codes.
 * At present, there is a GREAT DEAL of insanity between find_errors and
 * evaluation_error (value_type.h). In particular there is a lot of idiocy
 * mapping undefined_instance <--> undefined_value which is just plain wrong
 * in most cases.
 */
enum find_errors {
  unmade_instance,      /**< Found an unmade instance (NULL child). */
  undefined_instance,   /**< Instance in an expression is unknown child. */
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
# define GetDeclarativeContext() g_DeclarativeContext
  /**< retrieve the declarative context */
# define SetDeclarativeContext(n) g_DeclarativeContext = (n)
  /**< set the declarative context */
#else
# define GetDeclarativeContext() GetDeclarativeContextF()
  /** retrieve the declarative context */
extern int GetDeclarativeContextF(void);
  /**< retrieve the declarative context (for debugging) */
# define SetDeclarativeContext(n) SetDeclarativeContextF((n),__FILE__,__LINE__)
  /**< set the declarative context */
extern void SetDeclarativeContextF(int value, char *file, int line);
  /**< set the declarative context (for debugging) */
#endif

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
# define EVALDEBUG 0 /**< 1 = contextnoisy, 2 = fornoisy, 3 = both */
# define GetEvaluationContext() GetEvaluationContextF()
# define GetEvaluationForTable() GetEvaluationForTableF()
# if EVALDEBUG
#  if (EVALDEBUG == 1 || EVALDEBUG == 3)
#   define SetEvaluationContext(i) SetEvaluationContextF((i),__FILE__,__LINE__)
#  else
#   define SetEvaluationContext(i) SetEvaluationContextF(i)
#  endif
#  if (EVALDEBUG == 2 || EVALDEBUG == 3)
#   define SetEvaluationForTable(ft) SetEvaluationForTableF((ft),__FILE__,__LINE__)
#  else
#   define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
#  endif
# else
#  define SetEvaluationContext(i) SetEvaluationContextF(i)
#  define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
# endif
#else
# define GetEvaluationContext() g_EvaluationContext
# define SetEvaluationContext(i) g_EvaluationContext = (i)
# define GetEvaluationForTable() g_EvaluationForTable
# define SetEvaluationForTable(ft) g_EvaluationForTable = (ft)
#endif

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
 *  This evaluates the name in the context given by EvaluationContext.
 *  This must be set before the InstanceEvaluateName call.  Note since
 *  this is a global variable you cannot evaluate names in more than
 *  one context simultaneously.
 *
 *  If EvaluationForTable is non-NULL, the for table will be checked before
 *  the instance tree.
 */

extern struct value_t InstanceEvaluateSatisfiedName(CONST struct Name *nptr,
                                                    double tol);
/**<
 *  This function is specially to evaluate name of relations or logical
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
 *  Return the list of instances specified by n.  If this returns NULL,
 *  it indicates that it couldn't find the name.  Check err to discover why.
 */


extern struct gl_list_t *FindInstancesFromNames(CONST struct Instance *i,
                                                CONST struct gl_list_t *names,
                                                enum find_errors *err,
                                                unsigned long *errpos);
/**<
 *  <!--  struct gl_list_t *FindInstancesFromNames(i,names,err,errpos) -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  CONST struct gl_list_t *names;                               -->
 *  <!--  enum find_errors *err;                                       -->
 *  <!--  unsigned long *errpos;                                       -->
 *  Return the list of instances specified by names.  If this returns NULL,
 *  it indicates that it couldn't find a name.  Check err to discover why
 *  and errpos to discover which list element had the problem.
 *  Each listed name on input must resolve to a single instance, or
 *  the err will be impossible_instance.
 */
/* @} */

#endif /* ASC_FIND_H */

