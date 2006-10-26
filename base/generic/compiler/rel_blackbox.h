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
extern struct Instance *BlackBoxGetOutputVar(struct relation *r);

/** All the relations evaluated in a single call to y=f(x) have this data in common.  */
struct BlackBoxCache { /* was extrelcache, sort of. */
	struct gl_list_t *formalArgList; /**< only passed on pre_slv */
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
	int refCount; /* when to destroy */
};

/** All the elements of an array of blackbox relations resulting
from a single external statement have the BlackBoxCache in common, but
the varlist and lhs are unique to the specific relation.
If someone really needs a nodestamp, for some reason,
the 'common' pointer is unique to the set.
*/
struct BlackBoxData {
	struct BlackBoxCache *common;
	unsigned long *inputArgs;  /**< an array of indexes into the varlist;
				see notes elsewhere about why varlist
				may be shorter than arglist due to alias/ats.
				size is in common. */
	unsigned long lhsindex; /**< location of value yhati in C output array. */
	unsigned long lhsvar; /**< location of lhs var(yi) in varlist. */
};

/* make a blackboxdata unique to a single relation. */
extern struct BlackBoxData *CreateBlackBoxData(struct BlackBoxCache *common,
					unsigned long lhsindex,
					unsigned long lhsVarNumber);

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
 @param formalArgs list of list of args, which will be copied.
*/
extern struct BlackBoxCache *CreateBlackBoxCache( int32 inputsLen, int32 outputsLen, struct gl_list_t *formalArgs);

/** make a persistent reference to b. */
extern void AddRefBlackBoxCache(struct BlackBoxCache *b);

/** remove a persistent reference to *b.
 sets *b to null before returning.
 deallocates **b if refcount has dropped to 0.
 */
extern void DeleteRefBlackBoxCache(struct relation *rel, struct BlackBoxCache **b);


#endif /* ASC_REL_BLACKBOX_H */
