/* Optional IDA CSC Jacobian support. GPL-2.0-or-later, as for the IDA wrapper. */
#include "idasparse.h"
#include "idaanalyse.h"
#include <ascend/system/relman.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/utilities/error.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef ASC_IDA_KLU
struct IdaSparsePattern {
	sunindextype n, nnz;
	sunindextype *colptr, *rowind;
	size_t *rowstart;
	sunindextype *scatter;
	unsigned char *isderiv;
	struct var_variable **variables; /* gradient workspace */
	double *values;
};
struct IdaEntry { sunindextype row, col; size_t source; };

static int entry_compare(const void *a, const void *b){
	const struct IdaEntry *x = a, *y = b;
	if(x->col != y->col) return x->col < y->col ? -1 : 1;
	return (x->row > y->row) - (x->row < y->row);
}
#endif

void ida_sparse_free(IntegratorIdaData *data){
#ifdef ASC_IDA_KLU
	struct IdaSparsePattern *p = data->sparse;
	if(!p) return;
	ASC_FREE(p->colptr); ASC_FREE(p->rowind); ASC_FREE(p->rowstart);
	ASC_FREE(p->scatter); ASC_FREE(p->isderiv);
	ASC_FREE(p->variables); ASC_FREE(p->values); ASC_FREE(p);
#endif
	data->sparse = NULL;
}

#ifdef ASC_IDA_KLU
int ida_klu_setup(SUNLinearSolver solver, SUNMatrix matrix){
	int flag = SUNLinSolSetup_KLU(solver, matrix);
	sun_klu_common *common = SUNLinSol_KLUGetCommon(solver);
	if(flag != SUNLS_PACKAGE_FAIL_REC || common->status != KLU_SINGULAR) return flag;

	/* KLU refactor reuses numerical pivots. A zero pivot can therefore occur
	 * in a nonsingular matrix. SUNDIALS returns before its conditioning-based
	 * fresh-factorization path in this case (including in 6.4.1 and 7.9.0).
	 * Keep symbolic analysis and the old numeric object until recovery works:
	 * a genuinely singular trial must remain recoverable by IDA step retries.
	 * This uses the KLU content declared in sunlinsol_klu.h. */
	SUNLinearSolverContent_KLU content = (SUNLinearSolverContent_KLU)solver->content;
#if defined(SUNDIALS_INT64_T)
	typedef SuiteSparse_long IdaKluIndex;
#else
	typedef int IdaKluIndex;
#endif
	sun_klu_numeric *fresh = sun_klu_factor(
		(IdaKluIndex *)SM_INDEXPTRS_S(matrix), (IdaKluIndex *)SM_INDEXVALS_S(matrix),
		SM_DATA_S(matrix), SUNLinSol_KLUGetSymbolic(solver), common);
	if(!fresh){
		/* Preserve the original recoverable result for a singular trial.
		 * Allocation/invalid-input failures are not convergence retries. */
		if(common->status != KLU_SINGULAR){
#if SUNDIALS_VERSION_MAJOR >= 7
			flag = SUN_ERR_EXT_FAIL;
#else
			flag = SUNLS_PACKAGE_FAIL_UNREC;
#endif
		}
		content->last_flag = flag;
		return flag;
	}
	sun_klu_free_numeric(&content->numeric, common);
	content->numeric = fresh;
	content->last_flag = SUNLS_SUCCESS;
	return SUNLS_SUCCESS;
}

/* Share exactly the same derivative-to-state mapping in counting and assembly. */
static int ida_sparse_column(IntegratorSystem *integ, const struct var_variable *v){
	int col = var_sindex(v);
	if(var_deriv(v)){
		if(col < integ->n_y || col - integ->n_y >= integ->n_ydot) return -1;
		col = integrator_ida_diffindex(integ, v);
	}
	return col >= 0 && col < integ->n_y ? col : -1;
}

int ida_sparse_count(IntegratorSystem *integ, size_t *nnz){
	IntegratorIdaData *d = integ->enginedata;
	int *seen;
	*nnz = 0;
	if(integ->n_y <= 0 || d->nrels != integ->n_y
		|| (size_t)integ->n_y > SIZE_MAX / sizeof(*seen)) return 1;
	seen = ASC_NEW_ARRAY_CLEAR(int, integ->n_y);
	if(!seen) return 1;
	for(int i = 0; i < d->nrels; ++i){
		int len = rel_n_incidences(d->rellist[i]);
		const struct var_variable **vars = rel_incidence_list(d->rellist[i]);
		if(len < 0) goto fail;
		for(int j = 0; j < len; ++j){
			if(!var_apply_filter(vars[j], &d->vfilter)) continue;
			int col = ida_sparse_column(integ, vars[j]);
			if(col < 0) goto fail;
			if(seen[col] != i + 1){
				if(*nnz == SIZE_MAX) goto fail;
				++*nnz;
				seen[col] = i + 1;
			}
		}
	}
	ASC_FREE(seen);
	return 0;
fail:
	ASC_FREE(seen);
	return 1;
}
#endif

int ida_auto_use_klu(int n, size_t nnz){
	/* n is an int; its square fits uint64_t, including on 32-bit hosts. */
	return n >= 64 && (uint64_t)nnz <= (uint64_t)n * (uint64_t)n / 10;
}

int ida_auto_select(IntegratorSystem *integ, int autodiff, const char **solver,
	const char **reason, size_t *nnz){
	*solver = "DENSE";
	*nnz = SIZE_MAX;
#ifndef ASC_IDA_KLU
	(void)integ;
	(void)autodiff;
	*reason = "no-klu";
#else
	if(!autodiff){
		*reason = "finite-difference";
	}else if(integ->n_y < 64){
		*reason = "small-system";
	}else{
		if(ida_sparse_count(integ, nnz)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR, "Unable to count IDA Jacobian structure for AUTO");
			return 1;
		}
		if(ida_auto_use_klu(integ->n_y, *nnz)){
			*solver = "KLU";
			*reason = "sparse-pattern";
		}else{
			*reason = "dense-pattern";
		}
	}
#endif
	return 0;
}

#ifdef ASC_IDA_KLU
int ida_sparse_build(IntegratorSystem *integ){
	IntegratorIdaData *d = integ->enginedata;
	struct IdaSparsePattern *p;
	struct IdaEntry *entries = NULL;
	size_t count = 0, maxrow = 1, k = 0;
	int i, j;
	ida_sparse_free(d);
	if(integ->n_y <= 0 || d->nrels != integ->n_y) return 1;
	p = ASC_NEW_CLEAR(struct IdaSparsePattern);
	if(!p) return 1;
	d->sparse = p;
	p->n = integ->n_y;
	p->rowstart = ASC_NEW_ARRAY(size_t, (size_t)d->nrels + 1);
	if(!p->rowstart) goto fail;
	for(i = 0; i < d->nrels; ++i){
		int len = rel_n_incidences(d->rellist[i]);
		const struct var_variable **vars = rel_incidence_list(d->rellist[i]);
		p->rowstart[i] = count;
		if(len < 0) goto fail;
		if((size_t)len > maxrow) maxrow = len;
		for(j = 0; j < len; ++j){
			if(var_apply_filter(vars[j], &d->vfilter)){
				if(count == SIZE_MAX) goto fail;
				++count;
			}
		}
	}
	p->rowstart[d->nrels] = count;
	/* All allocations and the SUNDIALS capacity must represent this size. */
	if(count == 0 || count > SIZE_MAX / sizeof(struct IdaEntry)
		|| count > SIZE_MAX / sizeof(sunindextype)
		|| count > (sizeof(sunindextype) == 4 ? INT32_MAX : INT64_MAX)) goto fail;
	entries = ASC_NEW_ARRAY(struct IdaEntry, count);
	p->colptr = ASC_NEW_ARRAY_CLEAR(sunindextype, (size_t)p->n + 1);
	p->rowind = ASC_NEW_ARRAY(sunindextype, count);
	p->scatter = ASC_NEW_ARRAY(sunindextype, count);
	p->isderiv = ASC_NEW_ARRAY(unsigned char, count);
	p->variables = ASC_NEW_ARRAY(struct var_variable *, maxrow);
	p->values = ASC_NEW_ARRAY(double, maxrow);
	if(!entries || !p->colptr || !p->rowind || !p->scatter || !p->isderiv
		|| !p->variables || !p->values) goto fail;
	for(i = 0; i < d->nrels; ++i){
		int len = rel_n_incidences(d->rellist[i]);
		const struct var_variable **vars = rel_incidence_list(d->rellist[i]);
		for(j = 0; j < len; ++j){
			const struct var_variable *v = vars[j];
			int col;
			if(!var_apply_filter(v, &d->vfilter)) continue;
			p->isderiv[k] = !!var_deriv(v);
			col = ida_sparse_column(integ, v);
			if(col < 0) goto fail;
			entries[k].row = i; entries[k].col = col; entries[k].source = k;
			++k;
		}
	}
	qsort(entries, count, sizeof(*entries), entry_compare);
	for(k = 0; k < count; ++k){
		if(k == 0 || entries[k].row != entries[k-1].row || entries[k].col != entries[k-1].col){
			p->rowind[p->nnz++] = entries[k].row;
			++p->colptr[entries[k].col + 1];
		}
		p->scatter[entries[k].source] = p->nnz - 1;
	}
	for(i = 0; i < p->n; ++i) p->colptr[i+1] += p->colptr[i];
	ASC_FREE(entries);
	entries = NULL;
#if SUNDIALS_VERSION_MAJOR >= 6
	d->matrix = SUNSparseMatrix(p->n, p->n, p->nnz, CSC_MAT, d->sunctx);
#else
	d->matrix = SUNSparseMatrix(p->n, p->n, p->nnz, CSC_MAT);
#endif
	if(!d->matrix) goto fail;
	return 0;
fail:
	if(entries) ASC_FREE(entries);
	ida_sparse_free(d);
	ERROR_REPORTER_HERE(ASC_PROG_ERR, "Unable to construct IDA sparse Jacobian pattern");
	return 1;
}

int ida_sparse_jac(realtype t, realtype cj, N_Vector y, N_Vector yp,
	N_Vector r, SUNMatrix J, void *user, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3){
	IntegratorSystem *integ = user;
	IntegratorIdaData *d = integ->enginedata;
	struct IdaSparsePattern *p = d->sparse;
	int i, j, count;
	(void)r; (void)tmp1; (void)tmp2; (void)tmp3;
	if(!p || p->n != integ->n_y || d->nrels != integ->n_y) return -1;
	integrator_set_t(integ, t);
	integrator_set_y(integ, NV_DATA_S(y));
	integrator_set_ydot(integ, NV_DATA_S(yp));
	if(slv_check_bounds(integ->system, 0, -1, NULL)) return 1;
	/* IDA zeros the matrix, including its index arrays, before calling us. */
	memcpy(SM_INDEXPTRS_S(J), p->colptr, ((size_t)p->n + 1)*sizeof(sunindextype));
	memcpy(SM_INDEXVALS_S(J), p->rowind, (size_t)p->nnz*sizeof(sunindextype));
	memset(SM_DATA_S(J), 0, (size_t)p->nnz*sizeof(realtype));
	for(i = 0; i < d->nrels; ++i){
		if(relman_diff3(d->rellist[i], &d->vfilter, p->values, p->variables, &count, d->safeeval)) return 1;
		if((size_t)count != p->rowstart[i+1] - p->rowstart[i]) return -1;
		for(j = 0; j < count; ++j){
			size_t source = p->rowstart[i] + j;
			SM_DATA_S(J)[p->scatter[source]] += p->values[j] * (p->isderiv[source] ? cj : 1);
		}
	}
	for(sunindextype k = 0; k < p->nnz; ++k){
		if(!isfinite(SM_DATA_S(J)[k])){
			ERROR_REPORTER_HERE(ASC_PROG_ERR, "Non-finite IDA sparse Jacobian entry");
			return 1;
		}
	}
	return 0;
}
#endif
