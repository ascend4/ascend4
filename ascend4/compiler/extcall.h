/*
 *  External Call Module
 *  by Kirk Andre Abbott
 *  Created: Jun 1, 1995.
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: extcall.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:27 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
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

/*
 *  When #including extcall.h, make sure these files are #included first:
 *         #include "compiler.h"
 */



#ifndef __EXTCALL_H_SEEN__
#define __EXTCALL_H_SEEN__
/* requires
# #include <stdio.h>
# #include <stdlib.h>
# #include "compiler.h"
# #include "ascmalloc.h"
# #include "list.h"
# #include "extfunc.h"
# #include "instance_enum.h"
*/

struct ExtCallNode{
  struct ExternalFunc *efunc;
  struct gl_list_t *arglist;    /* list of Instance pointers */
  struct Instance **data;      	/* a hanlde to additional user data*/
  unsigned long subject;	/* index into the arglist */
  int nodestamp;               	/* a unique id for each call node */
};

extern struct ExtCallNode *CreateExtCall(struct ExternalFunc *efunc,
      struct gl_list_t *args,
      struct Instance *subject,
      struct Instance *data);
/*
 *
 */

extern void DestroyExtCall(struct ExtCallNode *ext,
      struct Instance *relinst);
/*
 *
 */

extern struct Instance *GetSubjectInstance(struct gl_list_t *,unsigned long);
extern unsigned long GetSubjectIndex(struct gl_list_t *,struct Instance *);
extern unsigned long CountNumberOfArgs(struct gl_list_t *,
           unsigned long, unsigned long);

extern struct gl_list_t *LinearizeArgList(struct gl_list_t *,
       unsigned long, unsigned long);
/*
 *  struct gl_list_t *LinearizeArgList(list,start,end)
 *  struct gl_list_t *list;
 *  unsigned long start;
 *  unsigned long end;
 *  Given a list of gl_list_t's this function will create a new list which
 *  is a linearized representation,i.e, each of the lists is spliced into
 *  the original list, to create one long list. The user now owns the
 *  new list structure, although the data in the original list is shared
 *  with the new list.
 */

extern struct gl_list_t *CopySpecialList(struct gl_list_t *);

extern void DestroySpecialList(struct gl_list_t *);
/*
 *  void DestroySpecialList(struct gl_list_t *)
 *  struct gl_list_t *list;
 *  Given a list of gl_list_t's, this function will destroy the lists
 *  structures associated with this complex list. It *will* not destroy
 *  the *leaf* data, but it will destroy all the list structures.
 */


#ifdef NDEBUG
#define ExternalCallExtFunc(ext) ((ext)->efunc)
#else
#define ExternalCallExtFunc(ext) ExternalCallExtFuncF(ext)
#endif
extern struct ExternalFunc *ExternalCallExtFuncF(struct ExtCallNode *);
/*
 *  struct ExternalFunc *ExternalCallExtFunc(ext);
 *  struct ExtCallNode *ext;
 *  Return the external function pointer of an external call;
 */

#ifdef NDEBUG
#define ExternalCallArgList(ext) ((ext)->arglist)
#else
#define ExternalCallArgList(ext) ExternalCallArgListF(ext)
#endif
extern struct gl_list_t *ExternalCallArgListF(struct ExtCallNode *ext);
/*
 *  struct gl_list_t *ExternalCallArgList(ext);
 *  struct relation_ExternalCall;
 *  Return the arguement list, which is a List of Lists of struct
 *  Instances, for an external call;
 */

extern struct Instance *ExternalCallDataInstance(struct ExtCallNode *);
/*
 *  struct Instance *ExternalCallDataInstance(ext);
 *  struct ExtCallNode *ext;
 *  Return the 'data' instance for an external call. This 'data'
 *  instance can has to be a MODEL_INST. It is used to convey additional
 *  information to a client who may need it. A NULL result means that no
 *  additional information was requested and is a valid result.
 */

#ifdef NDEBUG
#define ExternalCallVarIndex(ext) ((ext)->subject)
#else
#define ExternalCallVarIndex(ext) ExternalCallVarIndexF(ext)
#endif
extern unsigned long ExternalCallVarIndexF(struct ExtCallNode *ext);
/*
 *  unsigned long ExternalCallSubjectVarNdx(ext);
 *  struct ExtCallNode *ext;
 *  Return the index in the arguement list of the subject variable.
 *  This function uses the primitives GetWhichVar etc.
 */

extern struct Instance *ExternalCallVarInstance(struct ExtCallNode *ext);
/*
 *  struct Instance *ExternalCallSubjectVar(ext);
 *  struct ExtCallNode *ext;
 *  Return the "subject" variable instance of the external call. This is
 *  the variable that relation was constructed wrt. If NULL, then a user
 *  should consider this as an error.
 */

#ifdef NDEBUG
#define ExternalCallNodeStamp(ext) ((ext)->nodestamp)
#else
#define ExternalCallNodeStamp(ext) ExternalCallNodeStampF(ext)
#endif
extern int ExternalCallNodeStampF(struct ExtCallNode *ext);
/*
 *  int ExternalCallNodeStamp(ext);
 *  struct ExtCallNode *ext;
 *  Return the nodestamp for the given external node. Valid results
 *  are >= 0.
 */

extern void SetExternalCallNodeStamp(struct ExtCallNode *ext,
         int nodestamp);

#endif /* __EXTCALL_H_SEEN__ */










