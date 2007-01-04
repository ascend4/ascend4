/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott

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
*//**
	@file
	Hold-all structure for external relations and method calls as declared in an
	ASCEND model file. -- JP

	The ExtCallNode	structure points to an ExternalFunc structure and holds a
	list of instances which are tied to the arguments in the calling ASCEND
	statement. -- JP

	It seems that we shouldn't be accessing this structure at solve-time: the
	ExtRelCache object contains a lot of pre-processed stuff that makes
	evaluating black box functions easier (probably glass box too?) -- JP

	@TODO Complete documentation of compiler/extcall.h.

	In the context of an ExtCallNode (at least in the case of black box
	relations) the 'subject' is the output variable for the particular
	relation within the black box. There is 'relation' created for each
	output variable, as required for the [eye(NxN), ones(NxM)] submatrix
	in the jacobian. -- JP
*//*
	by Kirk Andre Abbott
	Created: Jun 1, 1995.
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:29:27 $ $Author: mthomas $
*/

#ifndef ASC_EXTCALL_H
#define ASC_EXTCALL_H

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <compiler/extfunc.h>
#include "compiler.h"
#include "instance_enum.h"

/**
	External call data structure

	@TODO Check the structure of the arglist. Maybe it's a list of lists? -- JP
*/
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
	Create ExtCall structure and return.
*/

extern void DestroyExtCall(struct ExtCallNode *ext,
		struct Instance *relinst);
/**<
	Destroy ExtCall structure.
*/

extern struct Instance *GetSubjectInstance(struct gl_list_t *arglist,
		unsigned long varndx);

extern unsigned long GetSubjectIndex(struct gl_list_t *arglist,
		struct Instance *subject);
/**<
	@TODO what is this?
*/

ASC_DLLSPEC(unsigned long) CountNumberOfArgs(struct gl_list_t *arglist,
        unsigned long start, unsigned long end);
/**< Count the expanded length of a list of lists, using
     those elements of arglist in [start .. end].
     This will match the size of the list returned by
     LinearizeArgList.
	This function gets used in the ExtRelCache instantiation stuff.

	@TODO what is the purpose of the 'start' and 'end' parameters?
*/

extern struct gl_list_t *LinearizeArgList(struct gl_list_t *arglist,
	    unsigned long start, unsigned long end);
/**<
	Taking a list of lists (list of struct gl_list_t*), this list will create
	a new list that is effectively all of the lists lined up end-to-end, then
	then sliced according to the start and end parameters.

	The user now owns the new list structure, although the data in the
	original list is shared with the new list.
*/

extern struct gl_list_t *CopySpecialList(struct gl_list_t *list);
/**< Shallow duplicate a 2d list, that is the resulting lists are new,
   but the (instance) data in them is not.
 */

extern struct gl_list_t *DeepCopySpecialList(struct gl_list_t *list, CopyFunc copy);
/**< Deep duplicate a 2d list of Names. That is the resulting lists are new,
   and the data enclosed is duplicated using copy.
 */

extern void DestroySpecialList(struct gl_list_t *list);
/**<
	Given a list of lists (list of struct gl_list_t*), this function will
	destroy the lists structures associated with this complex list.
	It *will* not destroy the *leaf* data, but it will destroy all the list
	structures.
*/

extern void DeepDestroySpecialList(struct gl_list_t *list, DestroyFunc dtor);
/**<
	Given a list of gl_list_t's, this function will destroy the lists
	structures associated with this complex list and destroy
	the *leaf* data using dtor.
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
	Implementation function for ExternalCallArgList().  Do not call this
	function directly - call ExternalCallArgList() instead.
*/



extern struct Instance *ExternalCallDataInstance(struct ExtCallNode *ext);
/**<
	Return the 'data' instance for an external call.  This 'data'
	instance "can has to be" (???) a MODEL_INST. It is used to convey additional
	information to a client who may need it. A NULL result means that no
	additional information was requested and is a valid result.

	@TODO what is the normal usage of this in the context of ASCEND compiler
	with an ASCEND solver???
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
	Implementation function for ExternalCallVarIndex().  Do not call this
	function directly - call ExternalCallVarIndex() instead.

	This function uses the primitives GetWhichVar etc.
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
	Return the (integer) nodestamp for the given external node.
	Valid results are >= 0. The nodestamp is incremented in the function
	AddExtArrayChildren such that all BlackBoxRelation objects corresponding to
	a single black box have the same nodestamp.
	This nodestamp is referenced by ExtRelCache objects.

	@TODO why is a nodestamp required when we could be using instance pointers
	or something more 'real'?

	@param ext <code>struct ExtCallNode*</code>, node to query.
	@return Returns the node stamp as an <code>int</code>.
	@see ExternalCallNodeStampF()
*/

extern int ExternalCallNodeStampF(struct ExtCallNode *ext);
/**<
	Implementation function for ExternalCallNodeStamp().  Do not call this
	function directly - call ExternalCallNodeStamp() instead.
*/



extern void SetExternalCallNodeStamp(struct ExtCallNode *ext, int nodestamp);
/**<
	Set the nodestamp for the given external node.
*/

#endif /* ASC_EXTCALL_H */
