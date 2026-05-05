#include "eqm_linalg.h"

#include <math.h>

int eqm_linalg_dense_solve(double *A, double *b, int n){
	const double piv_tol = 1e-14;
	int k;
	if(!A || !b || n <= 0){
		return 0;
	}
	for(k = 0; k < n; ++k){
		int i;
		int piv = k;
		double maxabs = fabs(A[k * n + k]);
		for(i = k + 1; i < n; ++i){
			double v = fabs(A[i * n + k]);
			if(v > maxabs){
				maxabs = v;
				piv = i;
			}
		}
		if(!(maxabs > piv_tol)){
			return 0;
		}
		if(piv != k){
			int j;
			for(j = k; j < n; ++j){
				double tmp = A[k * n + j];
				A[k * n + j] = A[piv * n + j];
				A[piv * n + j] = tmp;
			}
			{
				double tmp = b[k];
				b[k] = b[piv];
				b[piv] = tmp;
			}
		}
		{
			int i;
			double diag = A[k * n + k];
			for(i = k + 1; i < n; ++i){
				int j;
				double f = A[i * n + k] / diag;
				A[i * n + k] = 0.0;
				for(j = k + 1; j < n; ++j){
					A[i * n + j] -= f * A[k * n + j];
				}
				b[i] -= f * b[k];
			}
		}
	}
	for(k = n - 1; k >= 0; --k){
		int j;
		double s = b[k];
		double diag = A[k * n + k];
		if(!(fabs(diag) > piv_tol)){
			return 0;
		}
		for(j = k + 1; j < n; ++j){
			s -= A[k * n + j] * b[j];
		}
		b[k] = s / diag;
	}
	return 1;
}
