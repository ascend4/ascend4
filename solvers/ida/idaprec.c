/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

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
*//**
	@file
	Access to the IDA integrator for ASCEND. IDA is a DAE solver that comes
	as part of the GPL-licensed SUNDIALS solver package from LLNL.

	IDA provides the non-linear parts, as well as a number of pluggable linear
	solvers: dense, banded and krylov types.

	We also implement here an EXPERIMENTAL direct sparse linear solver for IDA
	using the ASCEND linsolqr routines.

	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

#define _GNU_SOURCE

#include <signal.h>
#include <setjmp.h>
#include <fenv.h>
#include <math.h>

/* SUNDIALS includes */
#ifdef ASC_WITH_IDA

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

# include <sundials/sundials_dense.h>
# include <ida/ida_spgmr.h>
# include <ida/ida_spbcgs.h>
# include <ida/ida_sptfqmr.h>
# include <ida/ida_dense.h>

# ifndef IDA_SUCCESS
#  error "Failed to include SUNDIALS IDA header file"
# endif
#else
# error "If you're building this file, you should have ASC_WITH_IDA"
#endif

#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/instance_enum.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/relman.h>
#include <ascend/system/block.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/bndman.h>

#include <ascend/utilities/config.h>
#include <ascend/integrator/integrator.h>

#include "idalinear.h"
#include "idaanalyse.h"
#include "ida_impl.h"
#include "idaprec.h"
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

/* #define FEX_DEBUG */
#define JEX_DEBUG
/* #define DJEX_DEBUG */
#define SOLVE_DEBUG
#define STATS_DEBUG
#define PREC_DEBUG
/* #define ROOT_DEBUG */

/* #define DIFFINDEX_DEBUG */
/* #define ANALYSE_DEBUG */
/* #define DESTROY_DEBUG */
/* #define MATRIX_DEBUG */


/*------
  Full jacobian preconditioner -- experimental
*/

static int integrator_ida_psetup_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

static int integrator_ida_psolve_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

static void integrator_ida_pcreate_jacobian(IntegratorSystem *integ);

const IntegratorIdaPrec prec_jacobian = {
	integrator_ida_pcreate_jacobian
	, integrator_ida_psetup_jacobian
	, integrator_ida_psolve_jacobian
};

/*------
  Jacobi preconditioner -- experimental
*/

static int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

static int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

static void integrator_ida_pcreate_jacobi(IntegratorSystem *integ);

const IntegratorIdaPrec prec_jacobi = {
	integrator_ida_pcreate_jacobi
	, integrator_ida_psetup_jacobi
	, integrator_ida_psolve_jacobi
};

/*----------------------------------------------
  FULL JACOBIAN PRECONDITIONER -- EXPERIMENTAL.
*/

static void integrator_ida_pcreate_jacobian(IntegratorSystem *integ){
	IntegratorIdaData *enginedata =integ->enginedata;
	IntegratorIdaPrecDataJacobian *precdata;
	precdata = ASC_NEW(IntegratorIdaPrecDataJacobian);
	mtx_matrix_t P;
	asc_assert(integ->n_y);
	precdata->L = linsolqr_create_default();

	/* allocate matrix to be used by linsolqr */
	P = mtx_create();
	mtx_set_order(P, integ->n_y);
	linsolqr_set_matrix(precdata->L, P);

	enginedata->pfree = &integrator_ida_pfree_jacobian;
	enginedata->precdata = precdata;
	CONSOLE_DEBUG("Allocated memory for Full Jacobian preconditioner");
}

void integrator_ida_pfree_jacobian(IntegratorIdaData *enginedata){
	mtx_matrix_t P;
	IntegratorIdaPrecDataJacobian *precdata;

	if(enginedata->precdata){
		precdata = (IntegratorIdaPrecDataJacobian *)enginedata->precdata;
		P = linsolqr_get_matrix(precdata->L);
		mtx_destroy(P);
		linsolqr_destroy(precdata->L);
		ASC_FREE(precdata);
		enginedata->precdata = NULL;

		CONSOLE_DEBUG("Freed memory for Full Jacobian preconditioner");
	}
	enginedata->pfree = NULL;
}

/**
	EXPERIMENTAL. Full Jacobian preconditioner for use with IDA Krylov solvers

	'setup' function.
*/
static int integrator_ida_psetup_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i, j, res;
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	IntegratorIdaPrecDataJacobian *precdata;
	linsolqr_system_t L;
	mtx_matrix_t P;
	struct rel_relation **relptr;

	integ = (IntegratorSystem *)p_data;
	enginedata = integ->enginedata;
	precdata = (IntegratorIdaPrecDataJacobian *)(enginedata->precdata);
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;
	mtx_coord_t C;

	L = precdata->L;
	P = linsolqr_get_matrix(L);
	mtx_clear(P);

	CONSOLE_DEBUG("Setting up Jacobian preconditioner");

	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/**
		@TODO FIXME here we are using the very inefficient and contorted approach
		of calculating the whole jacobian, then extracting just the diagonal elements.
	*/

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			relname = rel_make_name(integ->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating preconditioner derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}
		/* CONSOLE_DEBUG("Got %d derivatives from relation %d",count,i); */
		/* find the diagonal elements */
		for(j=0; j<count; ++j){
			if(var_deriv(variables[j])){
				mtx_fill_value(P, mtx_coord(&C, i, var_sindex(variables[j])), c_j * derivatives[j]);
			}else{
				mtx_fill_value(P, mtx_coord(&C, i, var_sindex(variables[j])), derivatives[j]);
			}
		}
	}

	mtx_assemble(P);

	if(status){
		CONSOLE_DEBUG("Error found when evaluating derivatives");
		res = 1; goto finish; /* recoverable */
	}

	integrator_ida_write_incidence(integ);

	res = 0;
finish:
	ASC_FREE(variables);
	ASC_FREE(derivatives);
	return res;
};

/**
	EXPERIMENTAL. Full Jacobian preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
static int integrator_ida_psolve_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *integ;
	IntegratorIdaData *data;
	IntegratorIdaPrecDataJacobian *precdata;
	integ = (IntegratorSystem *)p_data;
	data = integ->enginedata;
	precdata = (IntegratorIdaPrecDataJacobian *)(data->precdata);
	linsolqr_system_t L = precdata->L;

	linsolqr_add_rhs(L,NV_DATA_S(rvec),FALSE);

	mtx_region_t R;
	R.row.low = R.col.low = 0;
	R.row.high = R.col.high = mtx_order(linsolqr_get_matrix(L)) - 1;
    linsolqr_set_region(L,R);

    linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
    linsolqr_reorder(L, &R, linsolqr_rmethod(L));

	/// @TODO more here

	linsolqr_remove_rhs(L,NV_DATA_S(rvec));

	CONSOLE_DEBUG("Solving Jacobian preconditioner (c_j = %f)",c_j);
	return 0;
};


/*----------------------------------------------
  JACOBI PRECONDITIONER -- EXPERIMENTAL.
*/

static void integrator_ida_pcreate_jacobi(IntegratorSystem *integ){
	IntegratorIdaData *enginedata =integ->enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	precdata = ASC_NEW(IntegratorIdaPrecDataJacobi);

	asc_assert(integ->n_y);
	precdata->PIii = N_VNew_Serial(integ->n_y);

	enginedata->pfree = &integrator_ida_pfree_jacobi;
	enginedata->precdata = precdata;
	CONSOLE_DEBUG("Allocated memory for Jacobi preconditioner");
}

void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata){
	if(enginedata->precdata){
		IntegratorIdaPrecDataJacobi *precdata = (IntegratorIdaPrecDataJacobi *)enginedata->precdata;
		N_VDestroy_Serial(precdata->PIii);

		ASC_FREE(precdata);
		enginedata->precdata = NULL;
		CONSOLE_DEBUG("Freed memory for Jacobi preconditioner");
	}
	enginedata->pfree = NULL;
}

/**
	EXPERIMENTAL. Jacobi preconditioner for use with IDA Krylov solvers

	'setup' function.
*/
static int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i, j, res;
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	struct rel_relation **relptr;

	integ = (IntegratorSystem *)p_data;
	enginedata = integ->enginedata;
	precdata = (IntegratorIdaPrecDataJacobi *)(enginedata->precdata);
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;

	CONSOLE_DEBUG("Setting up Jacobi preconditioner");

	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/**
		@TODO FIXME here we are using the very inefficient and contorted approach
		of calculating the whole jacobian, then extracting just the diagonal elements.
	*/

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){

		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			relname = rel_make_name(integ->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating preconditioner derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}
		/* CONSOLE_DEBUG("Got %d derivatives from relation %d",count,i); */
		/* find the diagonal elements */
		for(j=0; j<count; ++j){
			if(var_sindex(variables[j])==i){
				if(var_deriv(variables[j])){
					NV_Ith_S(precdata->PIii, i) = 1./(c_j * derivatives[j]);
				}else{
					NV_Ith_S(precdata->PIii, i) = 1./derivatives[j];
				}

			}
		}
#ifdef PREC_DEBUG
		CONSOLE_DEBUG("PI[%d] = %f",i,NV_Ith_S(precdata->PIii,i));
#endif
	}

	if(status){
		CONSOLE_DEBUG("Error found when evaluating derivatives");
		res = 1; goto finish; /* recoverable */
	}

	integrator_ida_write_incidence(integ);

	res = 0;
finish:
	ASC_FREE(variables);
	ASC_FREE(derivatives);
	return res;
};

/**
	EXPERIMENTAL. Jacobi preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
static int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *integ;
	IntegratorIdaData *data;
	IntegratorIdaPrecDataJacobi *precdata;
	integ = (IntegratorSystem *)p_data;
	data = integ->enginedata;
	precdata = (IntegratorIdaPrecDataJacobi *)(data->precdata);

	CONSOLE_DEBUG("Solving Jacobi preconditioner (c_j = %f)",c_j);
	N_VProd(precdata->PIii, rvec, zvec);
	return 0;
};


