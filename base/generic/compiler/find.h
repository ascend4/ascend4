/**< 
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

#ifndef __FIND_H_SEEN__
#define __FIND_H_SEEN__
/**< 
 *  When #including find.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 */


enum find_errors {
  unmade_instance,	/**< in searching found an unmade instance (NULL child)*/
  undefined_instance,	/**< instance in an expression is unknown child */
  impossible_instance,	/**< name cannot possibily exist(real error),often sets*/
  correct_instance	/**< return value when everything went okay */
};
/**< 
 * At present, there is a GREAT DEAL of insanity between find_errors and
 * evaluation_error (value_type.h). In particular there is a lot of idiocy
 * mapping undefined_instance <--> undefined_value which is just plain wrong
 * in most cases.
 */

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
 *  Access this varible only by get and set operators below.
 */
#define FINDEBUG 0
#if (FINDEBUG==0)
#define GetDeclarativeContext() g_DeclarativeContext
#define SetDeclarativeContext(n) g_DeclarativeContext = (n)
#else
#define GetDeclarativeContext() GetDeclarativeContextF()
#define SetDeclarativeContext(n) SetDeclarativeContextF((n),__FILE__,__LINE__)
#endif
/**< wrapper functions for debugging */
extern int GetDeclarativeContextF(void);
extern void SetDeclarativeContextF(int,char *,int);

/**< 
 * Global variables used throughout semantic analysis to
 * indicate context of evaluation. Do Not reference these
 * directly, but use the macros for them instead.
 */
extern CONST struct Instance *g_EvaluationContext;
extern struct for_table_t *g_EvaluationForTable;
#ifndef NDEBUG
#define EVALDEBUG 0 /**< 1 = contextnoisy, 2 = fornoisy, 3 = both */
#define GetEvaluationContext() GetEvaluationContextF()
#define GetEvaluationForTable() GetEvaluationForTableF()
#if EVALDEBUG
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
#define SetEvaluationContext(i) SetEvaluationContextF((i),__FILE__,__LINE__)
#else /**< evaldebug12*/
#define SetEvaluationContext(i) SetEvaluationContextF(i)
#endif /**< evaldebug  12*/
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
#define SetEvaluationForTable(ft) SetEvaluationForTableF((ft),__FILE__,__LINE__)
#else /**< evaldebug 23 */
#define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
#endif /**< evaldebug  23*/
#else /*evaldebug*/
#define SetEvaluationContext(i) SetEvaluationContextF(i)
#define SetEvaluationForTable(ft) SetEvaluationForTableF(ft)
#endif /*evaldebug*/
#else /**< ndebug */
#define GetEvaluationContext() g_EvaluationContext
#define SetEvaluationContext(i) g_EvaluationContext = (i)
#define GetEvaluationForTable() g_EvaluationForTable
#define SetEvaluationForTable(ft) g_EvaluationForTable = (ft)
#endif /*ndebug*/
/**< 
 * Wrappers mainly for break point purposes of g_EvaluationContext
 * and g_EvaluationForTable.
 */
extern struct Instance *GetEvaluationContextF(void);
extern struct for_table_t *GetEvaluationForTableF(void);
#if EVALDEBUG /**< version printing file/line when setting globals */
extern void SetEvaluationContextF(CONST struct Instance *
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
                                  ,char *,int
#endif /**< evaldebug 13*/
                                 );
extern void SetEvaluationForTableF(struct for_table_t *
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
                                  ,char *,int
#endif /**< evaldebug 23*/
                                  );
#else /**< evaldebug */
extern void SetEvaluationContextF(CONST struct Instance *);
extern void SetEvaluationForTableF(struct for_table_t *);
#endif /**< evaldebug*/

extern struct value_t InstanceEvaluateName(CONST struct Name *);
/**< 
 *  struct value_t InstanceEvaluateName(nptr)
 *  struct Name *nptr;
 *  This evaluates the name in the context given by EvaluationContext.
 *  This must be set before the InstanceEvaluateName call.  Note since
 *  this is a global variable you cannot evaluate names in more than
 *  one context simultaneously.
 *
 *  If EvaluationForTable is non-NULL, the for table will be checked before
 *  the instance tree.
 */

extern struct value_t InstanceEvaluateSatisfiedName(CONST struct Name *,
                                                    double tol);
/**< 
 *  struct value_t InstanceEvaluateName(nptr,tol)
 *  struct Name *nptr;
 *  double tol;
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

extern struct gl_list_t *FindInstances(CONST struct Instance *,
           CONST struct Name *,
           enum find_errors *);
/**< 
 *  struct gl_list_t *FindInstances(i,n,err)
 *  struct Instance *i;
 *  CONST struct Name *n;
 *  enum find_errors *err;
 *  Return the list of instances specified by n.  If this returns NULL,
 *  it indicates that it couldn't find the name.  Check err to discover why.
 */
#endif /**< __FIND_H_SEEN__ */
