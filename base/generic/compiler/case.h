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
#ifndef __CASE_H_SEEN__
#define __CASE_H_SEEN__
/* requires
# #include"compiler.h"
# #include"list.h"
# #include"types.h"
*/

/*
 *                         Case processing
 */

struct Case {
    struct Set *ValueList;   /* List of Values for the Conditions  */
                             /* NULL if OTHERWISE  */
    struct gl_list_t *ref;   /* References to RelationInstance
    ModelInstance or WhenInstance */
    unsigned active;         /*  1:active   0:inactive  */
};



extern struct Case *CreateCase(struct Set *, struct gl_list_t *);
/*
 *  struct Case *CreateCase(vl,refinst,cond);
 *  struct Set *vl;
 *  struct gl_list_t *refinst;
 *  Create a Case from information provided for a When data structure
 */

#ifdef NDEBUG
#define GetCaseValues(c) ((c)->ValueList)
#else
#define GetCaseValues(c) GetCaseValuesF(c)
#endif
extern struct Set *GetCaseValuesF(struct Case *);
/*
 *  struct Set *GetCaseValuesF(case);
 *  struct Case *case;
 *  Return the List of Values of a Case.
 */

#ifdef NDEBUG
#define GetCaseReferences(c) ((c)->ref)
#else
#define GetCaseReferences(c) GetCaseReferencesF(c)
#endif
extern struct gl_list_t *GetCaseReferencesF(struct Case *);
/*
 *  struct gl_list_t  *GetCaseReferences(case)
 *  struct Case *case;
 *  Return the List of References of a Case.
 */

#ifdef NDEBUG
#define GetCaseStatus(c) ((c)->active)
#else
#define GetCaseStatus(c) GetCaseStatusF(c)
#endif
extern int GetCaseStatusF(struct Case *);
/*
 *  int *GetCaseStatusF(case);
 *  struct Case *case;
 *  Return the Status of a Case.
 */

extern struct Case *SetCaseValues(struct Case *,struct Set *);
/*
 *  struct Case *SetCaseValues(set);
 *  struct Case *case;
 *  struct Set *set;
 *  Set the List of Values of a Case.
 */

extern struct Case *SetCaseReferences(struct Case *,struct gl_list_t *);
/*
 *  struct Case  *SetCaseReferences(list);
 *  struct Case *case;
 *  struct gl_list_t *refinst;
 *  Set the List of References of a Case.
 */


extern struct Case *SetCaseStatus(struct Case *,int);
/*
 *  extern Case *SetCaseStatus(int);
 *  struct Case *case;
 *  unsigned status;
 *  Return the Status of a Case.
 */

extern unsigned long NumberCaseRefs(struct Case *);
/*
 *  unsigned long NumberCaseRefs(c)
 *  struct Case *c;
 *  This will indicate the number of distinct instances to which the
 *  reflist of this case points.
 */

extern struct Instance *CaseRef(struct Case *,
    unsigned long);
/*
 *  struct Instance *CaseRef(c,casenum)
 *  struct Case *c;
 *  unsigned long casenum;
 *  This will return the casenum'th instance of the case reflist.
 */

extern void DestroyCase(struct Case *);
/*
 *  void DestroyCase(case)
 *  struct Case *case;
 *  Destroy a Case.
 */


extern struct Case *CopyCase(struct Case *);
/*
 *  struct Case *CopyCase(case)
 *  struct Case *case;
 *  Copy a Case.
 */


#endif /* __CASE_H_SEEN__ */


