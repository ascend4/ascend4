/* Optional IDA CSC Jacobian support. GPL-2.0-or-later, as for the IDA wrapper. */
#ifndef ASC_IDASPARSE_H
#define ASC_IDASPARSE_H
#include "idatypes.h"

/* Cache belongs to the engine; it never aliases SUNMatrix index storage. */
void ida_sparse_free(IntegratorIdaData *data);
/* AUTO policy; SIZE_MAX denotes a structural count that was not needed. */
int ida_auto_use_klu(int n, size_t nnz);
int ida_auto_select(IntegratorSystem *integ, int autodiff, const char **solver,
	const char **reason, size_t *nnz);
#ifdef ASC_IDA_KLU
/* Retry a failed fixed-pivot refactorization with fresh numerical pivots. */
int ida_klu_setup(SUNLinearSolver solver, SUNMatrix matrix);
int ida_sparse_count(IntegratorSystem *integ, size_t *nnz);
int ida_sparse_build(IntegratorSystem *integ);
int ida_sparse_jac(realtype t, realtype cj, N_Vector y, N_Vector yp,
	N_Vector r, SUNMatrix J, void *user, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);
#endif
#endif
