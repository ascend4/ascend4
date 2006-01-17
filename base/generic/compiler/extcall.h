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

/** @file
	External Call Module.

	@todo Complete documentation of compiler/extcall.h.
*/

/** @file
	Requires:
       #include <stdio.h>
       #include <stdlib.h>
       #include "utilities/ascConfig.h"
       #include "compiler.h"
       #include "ascmalloc.h"
       #include "list.h"
       #include "extfunc.h"
       #include "instance_enum.h"
 */

#ifndef ASC_EXTCALL_H
#define ASC_EXTCALL_H

/** External call data structure. */
struct ExtCallNode{
  struct ExternalFunc *efunc; /**< Pointer to external function. */
  struct gl_list_t *arglist;  /**< List of Instance pointers. */
  struct Instance **data;     /**< A handle to additional user data. */
  unsigned long subject;      /**< Index into the arglist. */
  int nodestamp;              /**< A unique id for each call node. */
};

extern struct ExtCallNode *CreateExtCall(struct ExternalFunc *efunc,
                                         struct gl_list_t *args,
                                         struct Instance *subject,
                                         struct Instance *data);
/**<
	Create ExtCall
*/

extern void DestroyExtCall(struct ExtCallNode *ext,
                           struct Instance *relinst);
/**<
	Destroy ExtCall
*/

extern struct Instance *GetSubjectInstance(struct gl_list_t *arglist,
                                           unsigned long varndx);

extern unsigned long GetSubjectIndex(struct gl_list_t *arglist,
                                     struct Instance *subject);

extern unsigned long CountNumberOfArgs(struct gl_list_t *arglist,
                                       unsigned long start,
                                       unsigned long end);

extern struct gl_list_t *LinearizeArgList(struct gl_list_t *arglist,
                                          unsigned long start,
                                          unsigned long end);
/**<
	Given a list of gl_list_t's this function will create a new list which
	is a linearized representation,i.e, each of the lists is spliced into
	the original list, to create one long list. The user now owns the
	new list structure, although the data in the original list is shared
	with the new list.
*/

extern struct gl_list_t *CopySpecialList(struct gl_list_t *list);

extern void DestroySpecialList(struct gl_list_t *list);
/**<
	Given a list of gl_list_t's, this function will destroy the lists
	structures associated with this complex list. It *will* not destroy
	the *leaf* data, but it will destroy all the list structures.
*/

#ifdef NDEBUG
#define ExternalCallExtFunc(ext) ((ext)->efunc)
#else
#define ExternalCallExtFunc(ext) ExternalCallExtFuncF(ext)
#endif
/**< 
	Return the external function pointer of an external call.
	@param ext <code>struct ExtCallNode*</code>, node to query.
	@return Returns the external function as a <code>struct ExternalFunc*</code>.
	@see ExternalCallExtFuncF()
*/
extern struct ExternalFunc *ExternalCallExtFuncF(struct ExtCallNode *ext);
/**<
	Return the external function pointer of an external call;
	Implementation function for ExternalCallExtFunc().  Do not call this
	function directly - call ExternalCallExtFunc() instead.
*/

#ifdef NDEBUG
#define ExternalCallArgList(ext) ((ext)->arglist)
#else
#define ExternalCallArgList(ext) ExternalCallArgListF(ext)
#endif
/**<
	Return the argument list.  This is a List of Lists of struct
	Instances, for an external call.
	@param ext <code>struct ExtCallNode*</code>, node to query.
	@return Returns the arguments as a <code>struct gl_list_t*</code>.
	@see ExternalCallArgListF()
*/
extern struct gl_list_t *ExternalCallArgListF(struct ExtCallNode *ext);
/**<
	Return the arguement list, which is a List of Lists of struct
	Instances, for an external call;

	Implementation function for ExternalCallArgList().  Do not call this
	function directly - call ExternalCallArgList() instead.
*/

extern struct Instance *ExternalCallDataInstance(struct ExtCallNode *ext);
/**< 
	Return the 'data' instance for an external call.  This 'data'
	instance can has to be a MODEL_INST. It is used to convey additional
	information to a client who may need it. A NULL result means that no
	additional information was requested and is a valid result.
*/

#ifdef NDEBUG
#define ExternalCallVarIndex(ext) ((ext)->subject)
#else
#define ExternalCallVarIndex(ext) ExternalCallVarIndexF(ext)
#endif
/**< 
	Return the index in the argument list of the subject variable.
	@param ext <code>struct ExtCallNode*</code>, node to query.
	@return Returns the index as an <code>unsigned long</code>.
	@see ExternalCallVarIndexF()
*/
extern unsigned long ExternalCallVarIndexF(struct ExtCallNode *ext);
/**<
	@return the index in the argument list of the subject variable.
	This function uses the primitives GetWhichVar etc.
	Implementation function for ExternalCallVarIndex().  Do not call this
	function directly - call ExternalCallVarIndex() instead.
*/

extern struct Instance *ExternalCallVarInstance(struct ExtCallNode *ext);
/**< 
	Return the "subject" variable instance of the external call. This is
	the variable that relation was constructed wrt. If NULL, then a user
	should consider this as an error.
*/

#ifdef NDEBUG
#define ExternalCallNodeStamp(ext) ((ext)->nodestamp)
#else
#define ExternalCallNodeStamp(ext) ExternalCallNodeStampF(ext)
#endif
/**< 
	Return the nodestamp for the given external node.
	Valid results are >= 0.
	
	@param ext <code>struct ExtCallNode*</code>, node to query.
	@return Returns the node stamp as an <code>int</code>.
	@see ExternalCallNodeStampF()
*/

extern int ExternalCallNodeStampF(struct ExtCallNode *ext);
/**<
	Return the nodestamp for the given external node. Valid results
	are >= 0.

	Implementation function for ExternalCallNodeStamp().  Do not call this
	function directly - call ExternalCallNodeStamp() instead.
*/

extern void SetExternalCallNodeStamp(struct ExtCallNode *ext,
                                     int nodestamp);
/**< 
	Set the nodestamp for the given external node.
*/

#endif /* ASC_EXTCALL_H */

