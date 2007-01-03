/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//** @file
	This file presents a linear solver interface that can be utilised by the
	IDA integrator from the SUNDIALS suite by LLNL. The linear solver uses the
	ASCEND linsolqr routines internally, and takes advantage of the block
	decomposition functionality available in ASCEND.

	EXPERIMENTAL -- INCOMPLETE -- UNDER DEVELOPMENT

	This file and idalinear.c are modelled fairly closely on ida_dense.c from 
	the SUNDIALS distribution, which maps out the expected use of ida_lmem
	data structure, etc.

	@see http://www.llnl.gov/casc/sundials/
*/
#ifndef ASC_IDALINEAR_H
#define ASC_IDALINEAR_H

#include <ida/ida.h>
#include <nvector/nvector_serial.h>
#include <solver/mtx.h>

/**
	Function prototype for sparse jacobian evaluation as required by this linear solver
*/
typedef int IntegratorSparseJacFn(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, mtx_matrix_t Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);

#define IDAASCEND_SUCCESS 0

#define IDAASCEND_JACFN_RECVR 1

#define IDAASCEND_MEM_NULL -1
#define IDAASCEND_LMEM_NULL -2
#define IDAASCEND_MEM_FAIL -4
#define IDAASCEND_JACFN_UNDEF -5
#define IDAASCEND_JACFN_UNRECVR -6

/*------------------------------------
  User functions (called from ida.c in this directory)
*/

/**
	Configure IDA to use the ASCEND linear solver

	@param size IDA problem size (number of equations)
*/
int IDAASCEND(void *ida_mem, long size);

/**	
	Register a (sparse mtx) Jacobian evaluation function with the linear solver (required)
*/
int IDAASCENDSetJacFn(void *ida_mem, IntegratorSparseJacFn *jacfn, void *jac_data);

/**
	@param flag variable into which the last flag is returned
	@return non-zero if unable to retrieve the last flag successfully (eg if ida_mem is NULL)
*/
int IDAASCENDGetLastFlag(void *ida_mem, int *flag);

/**
	You need to free the returned string here.
*/
char *IDAASCENDGetReturnFlagName(int flag);

#endif
