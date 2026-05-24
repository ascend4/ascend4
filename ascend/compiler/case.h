/*
 *  Case Processing functions
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: case.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:12 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Case Processing functions.
 *  <pre>
 *  When #including case.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "list.h"
 *         #include "expr_types.h"
 *  </pre>
 */

#ifndef ASC_CASE_H
#define ASC_CASE_H

/**	@addtogroup compiler_stmt Compiler Statements
	@{
*/

/** Case data structure. */
struct Case {
    struct Set *ValueList;  /**< List of Values for the Conditions
                                 NULL if OTHERWISE  */
    struct Expr *condition; /**< Optional CASE ... IF classifier guard */
    struct Expr *applies;   /**< Optional APPLIES IF region predicate */
    struct gl_list_t *ref;  /**< References to RelationInstance
                                 ModelInstance or WhenInstance */
    struct gl_list_t *reinit; /**< REINIT action statements attached to the case */
    struct module_t *mod;   /**< module where the source CASE was parsed */
    unsigned long linenum;  /**< line number where the source CASE was parsed */
    unsigned active;        /**<  1:active   0:inactive  */
};

extern struct Case *CreateCase(struct Set *v1, struct gl_list_t *refinst);
/**<
 *  Create a Case from information provided for a When data structure.
 */

#ifdef NDEBUG
#define GetCaseValues(c) ((c)->ValueList)
#else
#define GetCaseValues(c) GetCaseValuesF(c)
#endif
/**<  Return the List of Values of a Case. */
extern struct Set *GetCaseValuesF(struct Case *cs);
/**<
 *  Return the List of Values of a Case.  Implementation of GetCaseValues().
 */

#ifdef NDEBUG
#define GetCaseCondition(c) ((c)->condition)
#else
#define GetCaseCondition(c) GetCaseConditionF(c)
#endif
/**< Return the optional CASE ... IF classifier guard of a Case. */
extern struct Expr *GetCaseConditionF(struct Case *cs);
/**<
 *  Return the optional CASE ... IF classifier guard.
 *  Implementation of GetCaseCondition().
 */

#ifdef NDEBUG
#define GetCaseApplies(c) ((c)->applies)
#else
#define GetCaseApplies(c) GetCaseAppliesF(c)
#endif
/**< Return the optional APPLIES IF region predicate of a Case. */
extern struct Expr *GetCaseAppliesF(struct Case *cs);
/**<
 *  Return the optional APPLIES IF region predicate.
 *  Implementation of GetCaseApplies().
 */

#ifdef NDEBUG
#define GetCaseReferences(c) ((c)->ref)
#else
#define GetCaseReferences(c) GetCaseReferencesF(c)
#endif
/**<  Return the List of References of a Case. */
extern struct gl_list_t *GetCaseReferencesF(struct Case *cs);
/**<
 *  Return the List of References of a Case.  Implementation of GetCaseReferences().
 */

#ifdef NDEBUG
#define GetCaseReinitStatements(c) ((c)->reinit)
#else
#define GetCaseReinitStatements(c) GetCaseReinitStatementsF(c)
#endif
/**<  Return the list of REINIT statements attached to a Case. */
extern struct gl_list_t *GetCaseReinitStatementsF(struct Case *cs);
/**
 *  Return the list of REINIT statements attached to a Case.
 *  Implementation of GetCaseReinitStatements().
 */

#ifdef NDEBUG
#define GetCaseStatus(c) ((c)->active)
#else
#define GetCaseStatus(c) GetCaseStatusF(c)
#endif
/**<  Return the Status of a Case. */
extern int GetCaseStatusF(struct Case *cs);
/**<
 *  Return the Status of a Case.  Implementation of GetCaseStatus().
 */

extern struct Case *SetCaseValues(struct Case *cs, struct Set *set);
/**<
 *  Set the List of Values of a Case.
 */

extern struct Case *SetCaseCondition(struct Case *cs, struct Expr *condition);
/**<
 *  Set the optional CASE ... IF classifier guard of a Case.
 */

extern struct Case *SetCaseApplies(struct Case *cs, struct Expr *applies);
/**<
 *  Set the optional APPLIES IF region predicate of a Case.
 */

extern struct Case *SetCaseReferences(struct Case *cs, struct gl_list_t *refinst);
/**<
 *  Set the List of References of a Case.
 */

extern struct Case *SetCaseReinitStatements(struct Case *cs, struct gl_list_t *reinit);
/**<
 *  Set the list of REINIT statements of a Case.
 */

#ifdef NDEBUG
#define GetCaseModule(c) ((c)->mod)
#else
#define GetCaseModule(c) GetCaseModuleF(c)
#endif
/**< Return the module where the source CASE was parsed. */
extern struct module_t *GetCaseModuleF(struct Case *cs);

#ifdef NDEBUG
#define GetCaseLineNum(c) ((c)->linenum)
#else
#define GetCaseLineNum(c) GetCaseLineNumF(c)
#endif
/**< Return the line number where the source CASE was parsed. */
extern unsigned long GetCaseLineNumF(struct Case *cs);

extern struct Case *SetCaseSource(struct Case *cs,
                                  struct module_t *mod,
                                  unsigned long linenum);
/**<
 *  Set the source location of a Case.
 */

extern struct Case *SetCaseStatus(struct Case *cs, int status);
/**<
 *  Return the Status of a Case.
 */

extern unsigned long NumberCaseRefs(struct Case *cs);
/**<
 *  This will indicate the number of distinct instances to which the
 *  reflist of this case points.
 */

extern struct Instance *CaseRef(struct Case *cs, unsigned long casenum);
/**<
 *  This will return the casenum'th instance of the case reflist.
 */

extern void DestroyCase(struct Case *cs);
/**<
 *  Destroy a Case.
 */

extern struct Case *CopyCase(struct Case *cs);
/**<
 *  Copy a Case.
 */

/* @} */

#endif  /* ASC_CASE_H */
