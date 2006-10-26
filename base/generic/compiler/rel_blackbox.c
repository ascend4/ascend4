
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "functype.h"
#include "safe.h"
#include "fractions.h"
#include "dimen.h"
#include "expr_types.h"
#include "vlist.h"
#include "dimen_io.h"
#include "instance_enum.h"
#include "find.h"
#include "atomvalue.h"
#include "instance_name.h"
#include "extfunc.h"
#include "relation_type.h"
#include "rel_blackbox.h"
#include "relation.h"
#include "relation_util.h"
#include "instance_io.h"
#include "instquery.h"
#include "visitinst.h"
#include "mathinst.h"
#include "extcall.h" /* for copy/destroy speciallist */
#include "packages.h" /* for init slv interp */

static int32 ArgsDifferent(double new, double old, double tol)
{
	if (fabs(new - old) > fabs(tol)) {
		return 1;
	} else {
		return 0;
	}
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
	struct BlackBoxData *unique;
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
  
/* impl setup */
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox evaluation is experimental (%s)",__FUNCTION__);
	(void)i;
	efunc = RelationBlackBoxExtFunc(r);
	unique = RelationBlackBoxData(r);
	common = RelationBlackBoxCache(r);
	argToVar = unique->inputArgs;
	argToVarLen = common->inputsLen;
	lhsVar = unique->lhsvar;
	outputIndex = unique->lhsindex;
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
		common->residCount++;
	}
	value = common->outputs[outputIndex];

	/* return lhsval - value */
	arg = RelationVariable(r,lhsVar);
	*res = RealAtomValue(arg) - value;
	return nok;

}

/* The tricky bit in this is that if input or output args are merged,
their partial derivatives get summed and the solver at least gets
what it deserves, which may or may not be sane in a modeling sense.
*/
int BlackBoxCalcGradient(struct Instance *i, double *gradient, struct relation *r)
{
/* decls */
	unsigned long *argToVar;
	unsigned long c, varlistLen;
	unsigned long argToVarLen;
	struct BlackBoxData *unique;
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
  
/* impl setup */
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox gradient is experimental (%s)",__FUNCTION__);
	(void)i;
	efunc = RelationBlackBoxExtFunc(r);
	unique = RelationBlackBoxData(r);
	common = RelationBlackBoxCache(r);
	argToVar = unique->inputArgs;
	argToVarLen = common->inputsLen;
	lhsVar = unique->lhsvar;
	outputIndex = unique->lhsindex;
	derivFunc = GetDerivFunc(efunc);
	inputTolerance = GetValueFuncTolerance(efunc);
	updateNeeded = 0;
	varlistLen = NumberVariables(r);

/* impl:
	check input values changed.
	if changed, recompute gradient in bbox.
	compute gradient per varlist from bbox row.
*/
	/* check: */
	if (common->gradCount < 1) {
		updateNeeded = 1;
	} else {
		for (c=0; c < argToVarLen ; c++) {
			arg = RelationVariable(r,argToVar[c]); 
			value = RealAtomValue(arg);
			if (ArgsDifferent(value, common->inputsJac[c], inputTolerance)) {
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
			common->inputsJac[c] = value;
		}
		common->interp.task = bb_deriv_eval;

		nok = (*derivFunc)(&(common->interp),
				common->inputsLen,
				common->outputsLen,
				common->inputsJac,
				common->outputs,
				common->jacobian);
		common->gradCount++;
	}
	for (k = 0; k < varlistLen; k++) {
		gradient[k] = 0.0;
	}
	/* now compute d(y-yhat)/dx for this row as I - dyhat/dx*/
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

struct Instance *BlackBoxGetOutputVar(struct relation *r)
{
	assert(r != NULL);
 	unsigned long lhsVarNumber;
	/* FIXME BlackBoxGetOutputVar */	
	return NULL;
}

struct BlackBoxData *CreateBlackBoxData(struct BlackBoxCache *common,
					unsigned long lhsindex,
					unsigned long lhsVarNumber)
{
	struct BlackBoxData *b = (struct BlackBoxData *)malloc(sizeof(struct BlackBoxData));
	assert(common!=NULL);
	b->common = common;
	AddRefBlackBoxCache(common);
	b->inputArgs = (unsigned long *)ascmalloc(sizeof(long) * (common->inputsLen));
	b->lhsindex = lhsindex;
	b->lhsvar = lhsVarNumber;
	return b;
}

void DestroyBlackBoxData(struct relation *rel, struct BlackBoxData *b)
{
	b->lhsindex = -(b->lhsindex);
	b->lhsvar = 0;
	ascfree(b->inputArgs);
	b->inputArgs = NULL;
	DeleteRefBlackBoxCache(rel, &(b->common));
	ascfree(b);
}

#define JACMAGIC -3.141592071828
struct BlackBoxCache *CreateBlackBoxCache(
	int32 inputsLen,
	int32 outputsLen,
	struct gl_list_t *formalArgs
)
{
	struct BlackBoxCache *b = (struct BlackBoxCache *)malloc(sizeof(struct BlackBoxCache));
 	b->interp.task = bb_none;
	b->interp.status = calc_all_ok;
	b->interp.user_data = NULL;
	b->formalArgList = CopySpecialList(formalArgs);
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
	return b;
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
		FPRINTF(ASCERR, "Jacobian overrun detected while destroying BlackBoxCache. Debug the user blackbox implementation.\n");
	}
	ascfree(b->inputs);
	ascfree(b->inputsJac);
	ascfree(b->outputs);
	ascfree(b->jacobian);
	DestroySpecialList(b->formalArgList);
	b->formalArgList = NULL;
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
		instantiation failure. If failure, we don't need final. */ 
		efunc = RelationBlackBoxExtFunc(rel);
		final = GetFinalFunc(efunc);
		b->interp.task = bb_last_call;
		(*final)(&(b->interp));
	}
	ascfree(b);
}

void DeleteRefBlackBoxCache(struct relation *rel, struct BlackBoxCache **b)
{
	struct BlackBoxCache * d = *b;
	assert(b != NULL && *b != NULL);
	*b = NULL;
	if (d->refCount > 0) {
		(d->refCount)--;
	}
	if (d->refCount == 0) {
		DestroyBlackBoxCache(rel,d);
		return;
	}
	if (d->refCount < 0) {
		FPRINTF(ASCERR, "Attempt to delete BlackBoxCache (%p) too often.\n", *b);
	}
}
