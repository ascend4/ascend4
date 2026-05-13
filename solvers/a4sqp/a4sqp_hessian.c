/*
 * Shared Hessian utilities for A4SQP frontends.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_hessian.h"

#include <math.h>

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
