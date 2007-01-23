/*
	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*/

#include "rel_blackbox.h"

#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

#include <general/pairlist.h>
#include <general/dstring.h>

#include "symtab.h"
#include "functype.h"
#include "safe.h"

#include "vlist.h"
#include "dimen_io.h"
#include "find.h"
#include "atomvalue.h"
#include "instance_name.h"
#include "relation.h"
#include "relation_util.h"
#include "instance_io.h"
#include "relation_io.h"
#include "instquery.h"
#include "visitinst.h"
#include "mathinst.h"
#include "extcall.h" /* for copy/destroy speciallist */
#include "packages.h" /* for init slv interp */
#include "name.h" /* for copy/destroy name */

#define BBDEBUG 0 /* set 0 if not wanting spew */

static int32 ArgsDifferent(double new, double old, double tol)
{
	if (fabs(new - old) > fabs(tol)) {
		return 1;
	} else {
		return 0;
	}
}

/*------------------------------------------------------------------------------
  RESIDUAL AND GRADIENT EVALUATION ROUTINES
*/

/*
	Note:
	Assume a function in c/fortran that computes yhat(x),
	aka func(inputs_x, outputs_yhat);.
	This is fit into the ascend world by associating
	inputs_x directly to ascend variables and defining
	the ascend variables y in the outer model.
	The residual of the blackbox equations in the ascend model
	is y-yhat. The gradient is I - dyhat/dx, where dyhat/dx
	is the reduced jacobian of the blackbox.
*/
int BlackBoxCalcResidGrad(struct Instance *i, double *res, double *gradient, struct relation *r)
{
	int residErr = 0;
	int gradErr = 0;

	residErr = BlackBoxCalcResidual(i, res, r);
	gradErr = BlackBoxCalcGradient(i, gradient, r);
	return (int)(fabs(residErr) + fabs(gradErr));
}

/*
	Note:
	Assume a function in c/fortran that computes yhat(x),
	aka func(inputs_x, outputs_yhat);.
	This is fit into the ascend world by associating
	inputs_x directly to ascend variables and defining
	the ascend variables y in the outer model.
	The residual of the blackbox equations in the ascend model
	is y-yhat. The gradient is I - dyhat/dx, where dyhat/dx
	is the reduced jacobian of the blackbox.
*/
int BlackBoxCalcResidual(struct Instance *i, double *res, struct relation *r)
{
/* decls */
	unsigned long *argToVar;
	unsigned long c;
	unsigned long argToVarLen;
	struct BlackBoxCache *common;
	struct ExternalFunc *efunc;
	ExtBBoxFunc *evalFunc;
	int lhsVar;
	int outputIndex;
	struct Instance *arg;
	double value;
	double inputTolerance;
	int updateNeeded;
	int nok = 0;
  	static int warnexpt;

	if(!warnexpt){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox evaluation is experimental (%s)",__FUNCTION__);
		warnexpt = 1;
	}

	(void)i;
	efunc = RelationBlackBoxExtFunc(r);
	common = RelationBlackBoxCache(r);
	assert(efunc && common);
	argToVarLen = common->inputsLen;
	argToVar = RBBOX(r).inputArgs;
	lhsVar = RBBOX(r).lhsvar;
	outputIndex = RBBOX(r).lhsindex;
	evalFunc = GetValueFunc(efunc);
	inputTolerance = GetValueFuncTolerance(efunc);
	updateNeeded = 0;

/* impl:
	check input values changed.
	if changed, recompute output values.
	compute y - yhat for lhsvar.
*/
	/* check: */
	if (common->residCount < 1) {
		updateNeeded = 1;
	} else {
		for (c=0; c < argToVarLen ; c++) {
			arg = RelationVariable(r,argToVar[c]);
			value = RealAtomValue(arg);
			if (ArgsDifferent(value, common->inputs[c], inputTolerance)) {
				updateNeeded = 1;
				break;
			}
		}
	}
	/* recompute */
	if (updateNeeded) {
		for (c=0; c < argToVarLen ;c++) {
			arg = RelationVariable(r,argToVar[c]);
			value = RealAtomValue(arg);
			common->inputs[c] = value;
		}
		common->interp.task = bb_func_eval;

		nok = (*evalFunc)(&(common->interp),
				common->inputsLen,
				common->outputsLen,
				common->inputs,
				common->outputs,
				common->jacobian);
		if(nok)CONSOLE_DEBUG("blackbox residual function returned error %d",nok);
		common->residCount++;
	}
	value = common->outputs[outputIndex];

	/* return lhsval - value */
	arg = RelationVariable(r,lhsVar);
	*res = RealAtomValue(arg) - value;
	return nok;

}

/**
	Calculate the gradient (slice of the overall jacobian) for the blackbox.

	The tricky bit in this is that if input or output args are merged,
	their partial derivatives get summed and the solver at least gets
	what it deserves, which may or may not be sane in a modeling sense.

	Implementation:
		# check input values changed.
		# if changed, recompute gradient in bbox.
		# compute gradient per varlist from bbox row.
*/

int blackbox_fdiff(ExtBBoxFunc *resfn, struct BBoxInterp *interp
	, int ninputs, int noutputs
	, double *inputs, double *outputs, double *jac
);

int BlackBoxCalcGradient(struct Instance *i, double *gradient
		, struct relation *r
){
/* decls */
	unsigned long *argToVar;
	unsigned long c, varlistLen;
	unsigned long argToVarLen;
	struct BlackBoxCache *common;
	struct ExternalFunc *efunc;
	ExtBBoxFunc *derivFunc;
	int lhsVar;
	int outputIndex;
	struct Instance *arg;
	double value;
	double inputTolerance;
	double *bboxRow;
	int updateNeeded, offset;
	unsigned int k;
	int nok = 0;
    static int warnexpt, warnfdiff;

	if(!warnexpt){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox gradient is experimental (%s)",__FUNCTION__);
		warnexpt = 1;
	}

 	/* prepare */
	(void)i;
	efunc = RelationBlackBoxExtFunc(r);
	common = RelationBlackBoxCache(r);
	argToVarLen = common->inputsLen;
	argToVar = RBBOX(r).inputArgs;
	lhsVar = RBBOX(r).lhsvar;
	outputIndex = RBBOX(r).lhsindex;
	derivFunc = GetDerivFunc(efunc);
	inputTolerance = GetValueFuncTolerance(efunc);
	updateNeeded = 0;
	varlistLen = NumberVariables(r);

	/* do we need to calculate? */
	if(common->gradCount < 1){
		updateNeeded = 1;
	}else{
		for(c=0; c<argToVarLen; c++){
			arg = RelationVariable(r,argToVar[c]);
			value = RealAtomValue(arg);
			if (ArgsDifferent(value, common->inputsJac[c], inputTolerance)) {
				updateNeeded = 1;
				break;
			}
		}
	}

	/* if inputs have changed more than allowed, or it's first time: evaluate */
	if (updateNeeded) {
		for (c=0; c < argToVarLen ;c++) {
			arg = RelationVariable(r,argToVar[c]);
			value = RealAtomValue(arg);
			common->inputsJac[c] = value;
		}
		common->interp.task = bb_deriv_eval;

		if(derivFunc){
			nok = (*derivFunc)(
				&(common->interp)
				, common->inputsLen, common->outputsLen
				, common->inputsJac, common->outputs, common->jacobian
			);
		}else{
			if(!warnfdiff){
				ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Using finite-difference "
					" to compute derivatives for one or more black boxes in"
					" this model."
				);
				warnfdiff = 1;
			}

			nok = blackbox_fdiff(GetValueFunc(efunc), &(common->interp)
				, common->inputsLen, common->outputsLen
				, common->inputsJac, common->outputs, common->jacobian
			);
		}
		common->gradCount++;
	}

	for (k = 0; k < varlistLen; k++) {
		gradient[k] = 0.0;
	}

	/* now compute d(y-yhat)/dx for this row as ( I - dyhat/dx ) */
	k = lhsVar-1;
	gradient[k] = 1.0; /* I */
	offset = common->inputsLen * outputIndex; /* offset to row needed. */
	bboxRow = common->jacobian + offset; /* pointer to row by ptr arithmetic */
	for (c=0; c < argToVarLen; c++) {
		k = ((int)(argToVar[c])) - 1; /* varnum is 1..n, not 0 .*/
		gradient[k] -= bboxRow[c];
	}

	return nok;
}

struct Instance *BlackBoxGetOutputVar(CONST struct relation *r)
{
	assert(r != NULL);
 	unsigned long lhsVarNumber;
	struct Instance *i;

	lhsVarNumber = RBBOX(r).lhsvar;
	i = RelationVariable(r, lhsVarNumber);
	return i;
}

static int g_cbbdcount=0;
struct BlackBoxData *CreateBlackBoxData(struct BlackBoxCache *common)
{
	struct BlackBoxData *b = (struct BlackBoxData *)malloc(sizeof(struct BlackBoxData));
	g_cbbdcount++;
	b->count = g_cbbdcount;
	assert(common!=NULL);
	b->common = common;
	AddRefBlackBoxCache(common);
#if BBDEBUG
	FPRINTF(ASCERR,"CreateBlackBoxData(%p) made BBD#%d (%p)\n",common,b->count, b);
#endif
	return b;
}

static
struct BlackBoxCache *CloneBlackBoxCache(struct BlackBoxCache *b)
{
	return CreateBlackBoxCache(b->inputsLen , b->outputsLen, b->argListNames, b->dataName, b->efunc);
}

void CopyBlackBoxDataByReference(struct relation *src, struct relation *dest, void * bboxtable_p)
{
	unsigned long entry;
	struct pairlist_t *bboxtable = (struct pairlist_t *)bboxtable_p;
	struct BlackBoxCache * newCache = NULL, *oldCache;
	struct BlackBoxData *b;
	struct BlackBoxData *a;
	assert(src);
	assert(dest);
	a = RelationBlackBoxData(src);
	oldCache = RelationBlackBoxCache(src);
	assert(a != NULL);

	/* at moment, key is src cache and val is first dest bbd */
	entry = pairlist_contains(bboxtable, oldCache);
	if (entry) {
		/* everyone gets a unique bbd and shared cache */
		b = (struct BlackBoxData *)pairlist_valueAt(bboxtable, entry);
#if BBDEBUG
		FPRINTF(ASCERR,"CopyBlackBoxDataByReference(%p, %p): found already cloned cache C#%d (%p)in BBD#%d (%p)\n",src,dest,b->common->count, b->common, b->count, b);
#endif
		newCache = b->common;
		b = CreateBlackBoxData(newCache);
	} else {
		/* dup common */
		/* This is common across a single box output array of rels,
		not across all box instances sharing relation data.
		All the array pointers, etc are different.
		*/
		newCache = CloneBlackBoxCache(oldCache); /* starts refc 1 for our scope. */
		b = CreateBlackBoxData(newCache);
		DeleteRefBlackBoxCache(dest, &newCache); /* left our scope. now null */
		pairlist_append(bboxtable, oldCache, b); /* next relation in same array will find its data in the table now, because it has the same a. */
#if BBDEBUG
		FPRINTF(ASCERR,"CopyBlackBoxDataByReference(%p, %p): cloned cache C#%d (%p)into BBD#%d (%p) with new cache C#%d (%p)\n",src,dest,oldCache->count, oldCache, b->count, b,b->common->count, b->common);
#endif
	}

	dest->externalData = b;
}

void DestroyBlackBoxData(struct relation *rel, struct BlackBoxData *b)
{
#if BBDEBUG
	FPRINTF(ASCERR,"DestroyBlackBoxData(%p): destroying bbd %p BBD#%d\n",rel,b,b->count);
#endif
	DeleteRefBlackBoxCache(rel, &(b->common));
	b->count *= -1;
	ascfree(b);
}

/*------------------------------------------------------------------------------
  BLACKBOX GRADIENT BY FINITE-DIFFERENCE
*/

/**
	This function works out what the peturbed value for a variable should be.

	@TODO add some awareness of boundaries in this, hopefully.
*/
static inline double blackbox_peturbation(double varvalue){
  return (1.0e-05);
}

/**
	Blackbox derivatives estimated by finite difference (by evaluation at
	peturbed value of each input in turn)

	Call signature as for ExtBBoxFunc (except for addition leading 'resfn' parameter)
*/
int blackbox_fdiff(ExtBBoxFunc *resfn, struct BBoxInterp *interp
	, int ninputs, int noutputs
	, double *inputs, double *outputs, double *jac
){
  long c,r;
  int nok = 0;
  double *tmp_outputs;
  double *ptr;
  double old_x,deltax,value;

  /* CONSOLE_DEBUG("NUMERICAL DERIVATIVE..."); */

  tmp_outputs = ASC_NEW_ARRAY_CLEAR(double,noutputs);

  for (c=0;c<ninputs;c++){
    /* perturb each input in turn */
    old_x = inputs[c];
	deltax = blackbox_peturbation(old_x);
    inputs[c] = old_x + deltax;
	/* CONSOLE_DEBUG("PETURBATED VALUE of input[%ld] = %f",c,inputs[c]); */

	/* call routine. note that the 'jac' parameter is just along for the ride */
    nok = (*resfn)(interp, ninputs, noutputs, inputs, tmp_outputs, jac);
    if(nok){
	    CONSOLE_DEBUG("External evaluation error (%d)",nok);
		break;
	}

	/* fill load jacobian */
    ptr = &jac[c];
    for(r=0;r<noutputs;r++){
      value = (tmp_outputs[r] - outputs[r]) / deltax;
	  /* CONSOLE_DEBUG("output[%ld]: value = %f, gradient = %f",r,tmp_outputs[r],value); */
      *ptr = value;
      ptr += ninputs;
    }

	/* now return this input to its old value */
    inputs[c] = old_x;
  }
  ASC_FREE(tmp_outputs);
  if(nok){
    CONSOLE_DEBUG("External evaluation error");
  }
  return nok;

}

/*------------------------------------------------------------------------------
  BLACK BOX CACHE
*/

static int g_cbbccount = 0;

#define JACMAGIC -3.141592071828
struct BlackBoxCache *CreateBlackBoxCache(
	int32 inputsLen,
	int32 outputsLen,
	struct gl_list_t *argListNames,
	struct Name *dataName,
	struct ExternalFunc *efunc
)
{
	struct BlackBoxCache *b = (struct BlackBoxCache *)malloc(sizeof(struct BlackBoxCache));
	g_cbbccount++;
	b->count = g_cbbccount;
 	b->interp.task = bb_none;
	b->interp.status = calc_all_ok;
	b->interp.user_data = NULL;
	b->argListNames = DeepCopySpecialList(argListNames,(CopyFunc)CopyName);
	b->dataName = CopyName(dataName);
	b->inputsLen = inputsLen;
	b->outputsLen = outputsLen;
	b->jacobianLen = inputsLen*outputsLen;
	b->hessianLen = 0; /* FIXME. */
	b->inputs = (double *)ascmalloc(inputsLen*sizeof(double));
	b->inputsJac = (double *)ascmalloc(inputsLen*sizeof(double));
	b->outputs = (double *)ascmalloc(outputsLen*sizeof(double));
	b->jacobian = (double*)ascmalloc(outputsLen*inputsLen*sizeof(double)+sizeof(double));
	b->jacobian[outputsLen*inputsLen] = JACMAGIC;
	b->hessian = NULL;
	b->residCount = 0;
	b->gradCount = 0;
	b->refCount = 1;
	b->efunc = efunc;
	return b;
}

void InitBBox(struct Instance *context, struct BlackBoxCache *b)
{
	ExtBBoxInitFunc * init;
	enum find_errors ferr = correct_instance;
	unsigned long nbr, br, errpos;
	struct gl_list_t *tmp;

	struct gl_list_t *arglist;
	struct Instance *data;

	/* fish up data from name. */
	if (b->dataName != NULL) {
		tmp = FindInstances(context,b->dataName,&ferr);
		assert(tmp != NULL);
		assert(gl_length(tmp) == 1);
		data = (struct Instance *)gl_fetch(tmp,1);
		gl_destroy(tmp);
		assert(data != NULL);
	} else {
		data = NULL;
	}

	/* convert arglistnames to arglist. */
	WriteNamesInList2D(ASCERR, b->argListNames, ", ", "\n");
	nbr = gl_length(b->argListNames);
	arglist = gl_create(nbr);
	for (br = 1; br <= nbr; br++) {
		tmp = (struct gl_list_t *)gl_fetch(b->argListNames,br);
		tmp = FindInstancesFromNames(context, tmp, &ferr, &errpos);
		assert(tmp != NULL);
		gl_append_ptr(arglist,tmp);
	}

	/* now do the init */
	init = GetInitFunc(b->efunc);
	b->interp.task = bb_first_call;
	(*init)( &(b->interp), data, arglist);
  	b->interp.task = bb_none;
}

int32 BlackBoxCacheInputsLen(struct BlackBoxCache *b)
{
	assert(b != NULL);
	return b->inputsLen;
}

void AddRefBlackBoxCache(struct BlackBoxCache *b)
{
	assert(b != NULL);
	(b->refCount)++;
}

static void DestroyBlackBoxCache(struct relation *rel, struct BlackBoxCache *b)
{
	struct ExternalFunc *efunc;
	ExtBBoxFinalFunc *final;
	assert(b != NULL);
	if (b->jacobian[b->outputsLen*b->inputsLen] != JACMAGIC)
	{
		ERROR_REPORTER_HERE(ASC_PROG_WARNING, "Jacobian overrun detected while destroying BlackBoxCache. Debug the user blackbox implementation.\n");
	}
#if BBDEBUG
	FPRINTF(ASCERR,"DestroyBlackBoxCache(%p,%p): destroying %d\n",rel,b,b->count);
#endif
	ascfree(b->inputs);
	ascfree(b->inputsJac);
	ascfree(b->outputs);
	ascfree(b->jacobian);
	DeepDestroySpecialList(b->argListNames,(DestroyFunc)DestroyName);
	b->argListNames = NULL;
	DestroyName(b->dataName);
	b->dataName = NULL;
	b->inputsLen = -(b->inputsLen);
	b->outputsLen = -(b->outputsLen);
	b->jacobianLen = -(b->jacobianLen);
	b->hessianLen = -(b->hessianLen);
	b->inputs = NULL;
	b->inputsJac = NULL;
	b->outputs = NULL;
	b->jacobian = NULL;
	b->hessian = NULL;
	b->residCount = -(b->residCount);
	b->gradCount = -(b->gradCount);
	b->refCount = -3;
	if (rel != NULL) {
		/* rel is null with refcount 0 only when there's a bbox array
		instantiation failure or we're in a weird copy process.
		Either way, we don't need final. */
		efunc = b->efunc;
		final = GetFinalFunc(efunc);
		b->interp.task = bb_last_call;
		(*final)(&(b->interp));
		b->efunc = NULL;
	}
	b->count *= -1;
	ascfree(b);
}

void DeleteRefBlackBoxCache(struct relation *rel, struct BlackBoxCache **b)
{
	struct BlackBoxCache * d = *b;
	assert(b != NULL && *b != NULL);
	if (d->refCount > 0) {
		(d->refCount)--;
#if BBDEBUG
	FPRINTF(ASCERR,"DeleteRefBlackBoxCache(%p,%p): C#%d refcount reduced to %d in bbox %p\n",rel,d,d->count,d->refCount,b);
#endif
		*b = NULL;
	}
	if (d->refCount == 0) {
#if BBDEBUG
		FPRINTF(ASCERR,"DeleteRefBlackBoxCache(%p,%p): bbc refcount reduced to %d in bbox %p\n",rel,d,d->count,b);
#endif
		DestroyBlackBoxCache(rel,d);
		*b = NULL;
		return;
	}
	if (d->refCount < 0) {
#if BBDEBUG
		FPRINTF(ASCERR, "Attempt to delete BlackBoxCache (%p) too often.\n", *b);
#endif
		*b = NULL;
	}
}
