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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
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

#ifndef __CASE_H_SEEN__
#define __CASE_H_SEEN__

/** Case data structure. */
struct Case {
    struct Set *ValueList;  /**< List of Values for the Conditions
                                 NULL if OTHERWISE  */
    struct gl_list_t *ref;  /**< References to RelationInstance
                                 ModelInstance or WhenInstance */
    unsigned active;        /**<  1:active   0:inactive  */
};

extern struct Case *CreateCase(struct Set *v1, struct gl_list_t *refinst);
/**<
 *  <!--  struct Case *CreateCase(vl,refinst);                         -->
 *  <!--  struct Set *vl;                                              -->
 *  <!--  struct gl_list_t *refinst;                                   -->
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
 *  <!--  struct Set *GetCaseValuesF(cs);                              -->
 *  <!--  struct Case *cs;                                             -->
 *  Return the List of Values of a Case.  Implementation of GetCaseValues().
 */

#ifdef NDEBUG
#define GetCaseReferences(c) ((c)->ref)
#else
#define GetCaseReferences(c) GetCaseReferencesF(c)
#endif
/**<  Return the List of References of a Case. */
extern struct gl_list_t *GetCaseReferencesF(struct Case *cs);
/**<
 *  <!--  struct gl_list_t  *GetCaseReferences(cs)                     -->
 *  <!--  struct Case *cs;                                             -->
 *  Return the List of References of a Case.  Implementation of GetCaseReferences().
 */

#ifdef NDEBUG
#define GetCaseStatus(c) ((c)->active)
#else
#define GetCaseStatus(c) GetCaseStatusF(c)
#endif
/**<  Return the Status of a Case. */
extern int GetCaseStatusF(struct Case *cs);
/**<
 *  <!--  int *GetCaseStatusF(cs);                                     -->
 *  <!--  struct Case *cs;                                             -->
 *  Return the Status of a Case.  Implementation of GetCaseStatus().
 */

extern struct Case *SetCaseValues(struct Case *cs, struct Set *set);
/**<
 *  <!--  struct Case *SetCaseValues(cs, set);                         -->
 *  <!--  struct Case *cs;                                             -->
 *  <!--  struct Set *set;                                             -->
 *  Set the List of Values of a Case.
 */

extern struct Case *SetCaseReferences(struct Case *cs, struct gl_list_t *refinst);
/**<
 *  <!--  struct Case  *SetCaseReferences(cs,refinst);                 -->
 *  <!--  struct Case *cs;                                             -->
 *  <!--  struct gl_list_t *refinst;                                   -->
 *  Set the List of References of a Case.
 */

extern struct Case *SetCaseStatus(struct Case *cs, int status);
/**<
 *  <!--  struct Case *SetCaseStatus(cs,status);                       -->
 *  <!--  struct Case *cs;                                             -->
 *  <!--  unsigned status;                                             -->
 *  Return the Status of a Case.
 */

extern unsigned long NumberCaseRefs(struct Case *cs);
/**<
 *  <!--  unsigned long NumberCaseRefs(cs)                             -->
 *  <!--  struct Case *cs;                                             -->
 *  This will indicate the number of distinct instances to which the
 *  reflist of this case points.
 */

extern struct Instance *CaseRef(struct Case *cs, unsigned long casenum);
/**<
 *  <!--  struct Instance *CaseRef(cs,casenum)                         -->
 *  <!--  struct Case *cs;                                             -->
 *  <!--  unsigned long casenum;                                       -->
 *  This will return the casenum'th instance of the case reflist.
 */

extern void DestroyCase(struct Case *cs);
/**<
 *  <!--  void DestroyCase(cs)                                         -->
 *  <!--  struct Case *cs;                                             -->
 *  Destroy a Case.
 */

extern struct Case *CopyCase(struct Case *cs);
/**<
 *  <!--  struct Case *CopyCase(cs)                                    -->
 *  <!--  struct Case *cs;                                             -->
 *  Copy a Case.
 */

#endif  /* __CASE_H_SEEN__ */

