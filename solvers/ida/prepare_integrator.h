/* SUNDIALS includes */
#ifndef ASC_WITH_IDA
# error "If you're building this file, you should have ASC_WITH_IDA"
#endif

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
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
# define IDA_MTX_T DlsMat
# define IDADENSE_SUCCESS IDADLS_SUCCESS
# define IDADENSE_MEM_NULL IDADLS_MEM_NULL
# define IDADENSE_ILL_INPUT IDADLS_ILL_INPUT
# define IDADENSE_MEM_FAIL IDADLS_MEM_FAIL
#else
# define IDA_MTX_T DenseMat
#endif

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==2
# include <sundials/sundials_config.h>
# include <sundials/sundials_nvector.h>
# include <ida/ida_spgmr.h>
# include <ida.h>
# include <nvector_serial.h>
#else
# include <sundials/sundials_config.h>
# include <nvector/nvector_serial.h>
# include <ida/ida.h>
#endif

#include <sundials/sundials_dense.h>
#include <ida/ida_spgmr.h>
#include <ida/ida_spbcgs.h>
#include <ida/ida_sptfqmr.h>
#include <ida/ida_dense.h>
#include <ida/ida_impl.h>

#ifndef IDA_SUCCESS
# error "Failed to include SUNDIALS IDA header file"
#endif

extern int prepare_integrator(IntegratorSystem *integ);
