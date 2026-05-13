/*
 * Shared Hessian utilities for A4SQP frontends.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_hessian.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static int a4sqp_hessian_dense_try_cholesky(
	const real64 *hess,
	int32 n,
	real64 shift,
	real64 pivot_floor
){
	real64 *l = NULL;
	int32 i;
	int32 j;
	int32 k;
	int ok = 0;
	if(hess == NULL || n <= 0){
		return 0;
	}
	if(!isfinite(shift) || shift < 0.0){
		shift = 0.0;
	}
	if(!isfinite(pivot_floor) || pivot_floor <= 0.0){
		pivot_floor = 1e-12;
	}
	l = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)n * (size_t)n);
	if(l == NULL){
		return 0;
	}
	for(i = 0; i < n; ++i){
		for(j = 0; j <= i; ++j){
			real64 sum = hess[i * n + j];
			if(i == j){
				sum += shift;
			}
			for(k = 0; k < j; ++k){
				sum -= l[i * n + k] * l[j * n + k];
			}
			if(i == j){
				if(!isfinite(sum) || sum < pivot_floor){
					goto cleanup;
				}
				l[i * n + i] = sqrt(sum);
			}else{
				real64 ljj = l[j * n + j];
				if(!isfinite(ljj) || ljj <= 0.0){
					goto cleanup;
				}
				l[i * n + j] = sum / ljj;
			}
		}
	}
	ok = 1;

cleanup:
	A4SQP_FREE(l);
	return ok;
}

real64 a4sqp_hessian_dense_regularize_psd(real64 *hess, int32 n, real64 min_diag){
	int32 i;
	int32 j;
	real64 shift = 0.0;
	real64 scale = 1.0;
	int tries;
	if(hess == NULL || n <= 0){
		return 0.0;
	}
	if(!isfinite(min_diag) || min_diag < 0.0){
		min_diag = 1e-8;
	}
	for(i = 0; i < n; ++i){
		for(j = i + 1; j < n; ++j){
			real64 a = hess[i * n + j];
			real64 b = hess[j * n + i];
			real64 sym;
			if(!isfinite(a)){
				a = 0.0;
			}
			if(!isfinite(b)){
				b = 0.0;
			}
			sym = 0.5 * (a + b);
			hess[i * n + j] = sym;
			hess[j * n + i] = sym;
		}
		if(!isfinite(hess[i * n + i])){
			hess[i * n + i] = 0.0;
		}
		if(fabs(hess[i * n + i]) > scale){
			scale = fabs(hess[i * n + i]);
		}
	}
	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			if(i == j){
				continue;
			}
			if(!isfinite(hess[i * n + j])){
				hess[i * n + j] = 0.0;
				hess[j * n + i] = 0.0;
			}
			if(fabs(hess[i * n + j]) > scale){
				scale = fabs(hess[i * n + j]);
			}
		}
	}
	if(scale < 1.0){
		scale = 1.0;
	}
	for(tries = 0; tries < 12; ++tries){
		if(a4sqp_hessian_dense_try_cholesky(hess,n,shift,min_diag)){
			break;
		}
		if(shift <= 0.0){
			shift = fmax(min_diag,1e-8 * scale);
		}else{
			shift *= 10.0;
		}
	}
	if(shift > 0.0){
		for(i = 0; i < n; ++i){
			hess[i * n + i] += shift;
		}
	}
	for(i = 0; i < n; ++i){
		if(hess[i * n + i] < min_diag){
			hess[i * n + i] = min_diag;
		}
	}
	return shift;
}

void a4sqp_hessian_dense_mul(const real64 *hess, int32 n, const real64 *x, real64 *y){
	int32 i;
	int32 j;
	if(hess == NULL || x == NULL || y == NULL || n <= 0){
		return;
	}
	for(i = 0; i < n; ++i){
		real64 sum = 0.0;
		for(j = 0; j < n; ++j){
			sum += hess[i * n + j] * x[j];
		}
		y[i] = sum;
	}
}

void a4sqp_dense_hessian_init(struct A4SqpDenseHessian *model){
	if(model == NULL){
		return;
	}
	model->n = 0;
	model->updates = 0;
	model->last_reg = 0.0;
	model->dense = NULL;
}

void a4sqp_dense_hessian_destroy(struct A4SqpDenseHessian *model){
	if(model == NULL){
		return;
	}
	A4SQP_FREE(model->dense);
	model->dense = NULL;
	model->n = 0;
	model->updates = 0;
	model->last_reg = 0.0;
}

int a4sqp_dense_hessian_reset_identity(
	struct A4SqpDenseHessian *model,
	int32 n,
	real64 diag
){
	int32 i;
	if(model == NULL || n < 0){
		return 1;
	}
	if(model->n != n || (n > 0 && model->dense == NULL)){
		a4sqp_dense_hessian_destroy(model);
		if(n > 0){
			model->dense = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)n * (size_t)n);
			if(model->dense == NULL){
				return 1;
			}
		}
		model->n = n;
	}
	if(!isfinite(diag) || diag <= 0.0){
		diag = 1.0;
	}
	if(n > 0){
		memset(model->dense,0,sizeof(real64) * (size_t)n * (size_t)n);
	}
	for(i = 0; i < n; ++i){
		model->dense[i * n + i] = diag;
	}
	model->updates = 0;
	model->last_reg = 0.0;
	return 0;
}

int a4sqp_dense_hessian_sync_identity(struct A4SqpDenseHessian *model, int32 n){
	if(model == NULL || n < 0){
		return 1;
	}
	if(model->n == n && (n == 0 || model->dense != NULL)){
		return 0;
	}
	return a4sqp_dense_hessian_reset_identity(model,n,1.0);
}

real64 a4sqp_dense_hessian_regularize_psd(
	struct A4SqpDenseHessian *model,
	real64 min_diag
){
	if(model == NULL || model->dense == NULL || model->n <= 0){
		return 0.0;
	}
	model->last_reg = a4sqp_hessian_dense_regularize_psd(model->dense,model->n,min_diag);
	return model->last_reg;
}

void a4sqp_dense_hessian_mul(
	const struct A4SqpDenseHessian *model,
	const real64 *x,
	real64 *y
){
	if(model == NULL){
		return;
	}
	a4sqp_hessian_dense_mul(model->dense,model->n,x,y);
}

int a4sqp_dense_hessian_bfgs_update(
	struct A4SqpDenseHessian *model,
	const real64 *new_scaled_x,
	const real64 *new_scaled_grad,
	const real64 *old_scaled_x,
	const real64 *old_scaled_grad,
	real64 min_diag
){
	const real64 eps = 1e-12;
	int32 i;
	int32 j;
	int32 n;
	real64 *s = NULL;
	real64 *y = NULL;
	real64 *bs = NULL;
	real64 sy = 0.0;
	real64 yy = 0.0;
	real64 sbs = 0.0;
	real64 theta = 1.0;
	if(
		model == NULL
		|| model->n <= 0
		|| model->dense == NULL
		|| new_scaled_x == NULL
		|| new_scaled_grad == NULL
		|| old_scaled_x == NULL
		|| old_scaled_grad == NULL
	){
		return 0;
	}
	n = model->n;
	s = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	y = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	bs = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	if(s == NULL || y == NULL || bs == NULL){
		A4SQP_FREE(s);
		A4SQP_FREE(y);
		A4SQP_FREE(bs);
		return 1;
	}
	for(i = 0; i < n; ++i){
		s[i] = new_scaled_x[i] - old_scaled_x[i];
		y[i] = new_scaled_grad[i] - old_scaled_grad[i];
		sy += s[i] * y[i];
		yy += y[i] * y[i];
	}
	if(model->updates == 0 && sy > eps && yy > eps){
		real64 gamma = yy / sy;
		if(!isfinite(gamma) || gamma < 1e-8){
			gamma = 1e-8;
		}else if(gamma > 1e8){
			gamma = 1e8;
		}
		if(a4sqp_dense_hessian_reset_identity(model,n,gamma)){
			A4SQP_FREE(s);
			A4SQP_FREE(y);
			A4SQP_FREE(bs);
			return 1;
		}
	}
	a4sqp_dense_hessian_mul(model,s,bs);
	for(i = 0; i < n; ++i){
		sbs += s[i] * bs[i];
	}
	if(sy < 0.2 * sbs && sbs > eps){
		theta = 0.8 * sbs / (sbs - sy);
	}
	if(theta < 1.0){
		sy = 0.0;
		for(i = 0; i < n; ++i){
			y[i] = theta * y[i] + (1.0 - theta) * bs[i];
			sy += s[i] * y[i];
		}
	}
	if(sy <= eps || sbs <= eps){
		A4SQP_FREE(s);
		A4SQP_FREE(y);
		A4SQP_FREE(bs);
		return 0;
	}
	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			model->dense[i * n + j] += y[i] * y[j] / sy - bs[i] * bs[j] / sbs;
		}
	}
	(void)a4sqp_dense_hessian_regularize_psd(model,min_diag);
	++model->updates;
	A4SQP_FREE(s);
	A4SQP_FREE(y);
	A4SQP_FREE(bs);
	return 0;
}
