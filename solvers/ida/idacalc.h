/*
	for cases where we don't have SUNDIALS_VERSION_MINOR defined, guess version 2.2
*/
#ifndef SUNDIALS_VERSION_MINOR
# ifdef __GNUC__
#  warning "GUESSING SUNDIALS VERSION 2.2"
# endif
# define SUNDIALS_VERSION_MINOR 2
#endif
#ifndef SUNDIALS_VERSION_MAJOR
# define SUNDIALS_VERSION_MAJOR 2
#endif

/* SUNDIALS 2.4.0 introduces new DlsMat in place of DenseMat */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==4
# define IDA_MTX_T DlsMat
# define IDADENSE_SUCCESS IDADLS_SUCCESS
# define IDADENSE_MEM_NULL IDADLS_MEM_NULL
# define IDADENSE_ILL_INPUT IDADLS_ILL_INPUT
# define IDADENSE_MEM_FAIL IDADLS_MEM_FAIL
#else
# define IDA_MTX_T DenseMat
#endif


/* residual function forward declaration */
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data);

int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
);

/* dense jacobian evaluation for IDADense dense direct linear solver */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
int integrator_ida_djex(int Neq, realtype tt, realtype c_j
		, N_Vector yy, N_Vector yp, N_Vector rr
		, IDA_MTX_T Jac, void *jac_data
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);
#else
int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, IDA_MTX_T Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);
#endif

/* sparse jacobian evaluation for ASCEND's sparse direct solver */
IntegratorSparseJacFn integrator_ida_sjex;

/* boundary-detection function */
int integrator_ida_rootfn(realtype tt, N_Vector yy, N_Vector yp, realtype *gout, void *g_data);

