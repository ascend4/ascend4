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
*//** @file
	Evaluation functions of the ASCEND wrapping of IDA. These functions provide
	residual and jacobian evaluation functions in various forms back to IDA
	so that the integrator can do its work.
*/

#include "ida.h"
#include "idalinear.h"

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

