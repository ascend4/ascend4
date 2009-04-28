/*
	ASCEND Language Interpreter
	Copyright (C) 2006 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
	This file is part of the SLV solver.
*//** @file

    @NOTE blackbox equations support the form
	
    	output[i] = f(input[j] for all j) foreach i

	which we calculate by calling yhat = f(x),
	and then residual is (y - yhat).
*/

#ifndef ASC_REL_BLACKBOX_H
#define ASC_REL_BLACKBOX_H

/**	@addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>
#include "instance_enum.h"
#include "relation_type.h"
#include <general/list.h>
#include "expr_types.h"
#include "extfunc.h"

/**
	Direct solve of blackbox. Only returns non-NULL if the requested
	variable is the output of the blackbox (not one of the inputs)

	@param ri relation instance (asserted to be e_blackbox type)
	@param v variable for which value is wanted
	@param able (returned) 1 if able to solve for v
	@param nsolns (returned) will always return 1 or 0.
	@return NULL if solution not possible, else an array of double of length 1
*/
real64 *blackbox_dsolve(struct Instance *ri, struct Instance *v
		, int *able
		, int *nsolns
);


extern int BlackBoxCalcResidual(struct Instance *i, double *res, struct relation *r);


/** Compute standard form residual and gradient. 
See relation_util.h gradient functions
for additional notes on the gradient output array and varlist.
@param i the relation instance in question.
@param residual the residual computed (output).
@param gradient the gradient computed (output), an array as long as r->varlist
and with elements corresponding to the elements of varlist.
@param r the relation data from i.
*/
extern int BlackBoxCalcResidGrad(struct Instance *i, double *residual, double *gradient, struct relation *r);

/** Compute standard form gradient. 
See relation_util.h gradient functions
for additional notes on the gradient output array and varlist.
@param i the relation instance in question.
@param gradient the gradient computed (output), an array as long as r->varlist
and with elements corresponding to the elements of varlist.
@param r the relation data from i.
*/
extern int BlackBoxCalcGradient(struct Instance *i, double *gradient, struct relation *r);

/**
Return the output variable instance from r, assuming r is from a blackbox.
*/
extern struct Instance *BlackBoxGetOutputVar(CONST struct relation *r);

/** All the relations evaluated in a single call to y=f(x) have this data in common.  */
struct BlackBoxCache { /* was extrelcache, sort of. */
	struct gl_list_t *argListNames; /**< list of list of names. */
	struct Name *dataName; /**< name of the DATA instance. */
	struct ExternalFunc *efunc; /**< external function table. */
	struct BBoxInterp interp; /**< userdata lives in here only */
	int32 inputsLen; /**< number of actual, not formal, inputs */
	int32 outputsLen; /**< number of actual, not formal, outputs. */
	int32 jacobianLen;
	int32 hessianLen;
	double *inputs; /**< aka x; previous input for func eval. */
	double *outputs; /**< aka yhat. previous output for func eval. */
	double *inputsJac; /**< aka x; previous input for gradient eval. */
	double *jacobian; /**< sensitivity dyhat/dx ; row major format; previous gradient output. */
	double *hessian; /**< undetermined format */
	int residCount; /**< number of calls made for y output. */
	int gradCount; /**< number of calls made for gradient. */
	int refCount; /**< when to destroy */
	int count; /**< serial number */
};

/** Fetch the input array len size. 
    @param bbc the source.
*/
extern int32 BlackBoxCacheInputsLen(struct BlackBoxCache *bbc);

/** All the elements of an array of blackbox relations resulting
from a single external statement have the BlackBoxCache in common, but
the varlist and lhs are unique to the specific relation.
If someone really needs a nodestamp, for some reason,
the 'common' pointer is unique to the set.
*/
struct BlackBoxData {
	struct BlackBoxCache *common;
	int count;
};

/** make a blackboxdata unique to a single relation.
@param common the data common to all the relations in the array of blackbox output relations.
@param lhsindex the index of this relation's residual in the blackbox output vector.
@param lhsVarNumber the index of the output variable (y) in the relation's varlist. 
*/
extern struct BlackBoxData *CreateBlackBoxData(struct BlackBoxCache *common);

/* do anoncopy of bbox data for relation sharing. */
extern void CopyBlackBoxDataByReference(struct relation *src, struct relation *dest, void *bboxtable_p);

/* called when destroying the relation containing b. */
extern void DestroyBlackBoxData(struct relation * rel, struct BlackBoxData *b);

/**
Returns an initialized common data for a blackbox relation array
with an initial refcount 1.
Call DeleteRefBlackBoxCache when done with the object, and
make a call to AddRef any time the pointer is stored in a 
persistent structure.
 @param inputsLen number of actual, not formal, inputs.
 @param outputsLen number of actual, not formal, outputs.
 @param argListNames list of lists of names of real atom instances in/output.
 @param dataName name of the data instance.
*/
extern struct BlackBoxCache *CreateBlackBoxCache( int32 inputsLen, int32 outputsLen, struct gl_list_t *argListNames, struct Name *dataName, struct ExternalFunc *efunc);

/** make a persistent reference to b. */
extern void AddRefBlackBoxCache(struct BlackBoxCache *b);

/** dispatch the init function on a bbox after updating the input instance list
  from the names list.
  @param context the parent model containing the relations the cache is
   referenced by.
  @param b a cache in need of the init call.
 */
extern void InitBBox(struct Instance *context, struct BlackBoxCache *b);

/** remove a persistent reference to *b.
 sets *b to null before returning.
 deallocates **b if refcount has dropped to 0.
 */
extern void DeleteRefBlackBoxCache(struct relation *rel, struct BlackBoxCache **b);

/* @} */

#endif /* ASC_REL_BLACKBOX_H */
