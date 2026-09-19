/* Optional IDA CSC Jacobian support. GPL-2.0-or-later, as for the IDA wrapper. */
#ifndef ASC_IDASPARSE_H
#define ASC_IDASPARSE_H
#include "idatypes.h"

/* Cache belongs to the engine; it never aliases SUNMatrix index storage. */
void ida_sparse_free(IntegratorIdaData *data);
#ifdef ASC_IDA_KLU
int ida_sparse_build(IntegratorSystem *integ);
int ida_sparse_jac(realtype t, realtype cj, N_Vector y, N_Vector yp,
	N_Vector r, SUNMatrix J, void *user, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);
#endif
#endif
