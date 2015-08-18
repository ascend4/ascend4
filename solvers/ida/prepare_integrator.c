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
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaprec.h"
#include "idacalc.h"
#include "idaio.h"
#include "idaboundary.h"
#include "prepare_integrator.h"
#include <signal.h>
#include <setjmp.h>
#include <fenv.h>
#include <math.h>
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
#define IDA_BND_DEBUG




static IntegratorCreateFn integrator_ida_create;
static IntegratorParamsDefaultFn integrator_ida_params_default;
static IntegratorSolveFn integrator_ida_solve;
static IntegratorFreeFn integrator_ida_free;


//extern ASC_EXPORT int ida_register(void) {
	//CONSOLE_DEBUG("Registering IDA...");
	//return integrator_register(&integrator_ida_internals);
//}

static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);
static void integrator_ida_create(IntegratorSystem *integ) {
	//CONSOLE_DEBUG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	//CONSOLE_DEBUG("enginedata = %p",enginedata);
	enginedata->rellist = NULL;
	enginedata->safeeval = 0;
	enginedata->vfilter.matchbits = VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE
			| VAR_FIXED;
	enginedata->vfilter.matchvalue = VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0;
	enginedata->pfree = NULL;

	enginedata->rfilter.matchbits = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;
	enginedata->rfilter.matchvalue = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;

	enginedata->flagfn = NULL;
	enginedata->flagfntype = NULL;
	enginedata->flagnamefn = NULL;

	integ->enginedata = (void *) enginedata;

	integrator_ida_params_default(integ);
}

static void integrator_ida_free(void *enginedata) {
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("DESTROYING IDA engine data at %p",enginedata);
#endif
	IntegratorIdaData *d = (IntegratorIdaData *) enginedata;
	asc_assert(d);
	if (d->pfree) {
		CONSOLE_DEBUG("DESTROYING preconditioner data using fn at %p",d->pfree);
		/* free the preconditioner data, whatever it happens to be */
		(d->pfree)(enginedata);
	}

	ASC_FREE(d->rellist);

#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Now destroying the enginedata");
#endif
	ASC_FREE(d);
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("enginedata freed");
#endif
}



IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ) {
	IntegratorIdaData *d;
	assert(integ!=NULL);
	assert(integ->enginedata!=NULL);
	assert(integ->engine==INTEG_IDA);
	d = ((IntegratorIdaData *) (integ->enginedata));
	return d;
}




static int integrator_ida_params_default(IntegratorSystem *integ) {
	asc_assert(integ!=NULL);
	asc_assert(integ->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(integ->params);

	slv_destroy_parms(p);

	if (p->parms == NULL) {
		//CONSOLE_DEBUG("params NULL");
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, IDA_PARAMS_SIZE);
		if (p->parms == NULL)
			return -1;
		p->dynamic_parms = 1;
	} else {
		//CONSOLE_DEBUG("params not NULL");
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(
			p,
			IDA_PARAM_AUTODIFF,
			(SlvParameterInitBool) { {"autodiff"
							,"Use auto-diff?",1
							,"Use automatic differentiation of expressions (1) or use numerical derivatives (0)"
						}, TRUE}
					);

					slv_param_char(p,IDA_PARAM_CALCIC
							,(SlvParameterInitChar) { {"calcic"
									,"Initial conditions calcuation",1
									,"Use specified values of ydot to solve for inital y (Y),"
									" or use the the values of the differential variables (yd) to solve"
									" for the pure algebraic variables (ya) along with the derivatives"
									" of the differential variables (yddot) (YA_YDP), or else don't solve"
									" the intial conditions at all (NONE). See IDA manual p 41 (IDASetId)"
								}, "YA_YDP"}, (char *[]) {"Y", "YA_YDP", "NONE",NULL}
					);

					slv_param_bool(p,IDA_PARAM_SAFEEVAL
							,(SlvParameterInitBool) { {"safeeval"
									,"Use safe evaluation?",1
									,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
									"throw SIGFPE errors which will then halt integration."
								}, FALSE}
					);

					slv_param_bool(p,IDA_PARAM_ATOLVECT
							,(SlvParameterInitBool) { {"atolvect"
									,"Use 'ode_atol' values as specified?",1
									,"If TRUE, values of 'ode_atol' are taken from your model and used "
									" in the integration. If FALSE, a scalar absolute tolerance value"
									" is shared by all variables. See IDA manual, section 5.5.1"
								}, TRUE}
					);

					slv_param_real(p,IDA_PARAM_ATOL
							,(SlvParameterInitReal) { {"atol"
									,"Scalar absolute error tolerance",1
									,"Value of the scalar absolute error tolerance. See also 'atolvect'."
									" See IDA manual, sections 5.5.1 and 5.5.2 'Advice on choice and use of tolerances'"
								}, 1e-5, 0, 1e10}
					);

					slv_param_real(p,IDA_PARAM_RTOL
							,(SlvParameterInitReal) { {"rtol"
									,"Scalar relative error tolerance",1
									,"Value of the scalar relative error tolerance. (Note that for IDA,"
									" it's not possible to set per-variable relative tolerances as it is"
									" with LSODE)."
									" See IDA manual, section 5.5.2 'Advice on choice and use of tolerances'"
								}, 1e-4, 0, 1}
					);

					slv_param_char(p,IDA_PARAM_LINSOLVER
							,(SlvParameterInitChar) { {"linsolver"
									,"Linear solver",1
									,"See IDA manual, section 5.5.3. Choose 'ASCEND' to use the linsolqr"
									" direct linear solver bundled with ASCEND, 'DENSE' to use the dense"
									" solver bundled with IDA, or one of the Krylov solvers SPGMR, SPBCG"
									" or SPTFQMR (which still need preconditioners to be implemented"
									" before they can be very useful."
								}, "DENSE"}, (char *[]) {"ASCEND","DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL}
					);

					slv_param_int(p,IDA_PARAM_MAXL
							,(SlvParameterInitInt) { {"maxl"
									,"Maximum Krylov dimension",0
									,"The maximum dimension of Krylov space used by the linear solver"
									" (for SPGMR, SPBCG, SPTFQMR) with IDA. See IDA manual section 5.5."
									" The default of 0 results in IDA using its internal default, which"
									" is currently a value of 5."
								}, 0, 0, 20}
					);

					slv_param_int(p,IDA_PARAM_MAXORD
							,(SlvParameterInitInt) { {"maxord"
									,"Maximum order of linear multistep method",0
									,"The maximum order of the linear multistep method with IDA. See"
									" IDA manual p 38."
								}, 5, 1, 5}
					);

					slv_param_bool(p,IDA_PARAM_GSMODIFIED
							,(SlvParameterInitBool) { {"gsmodified"
									,"Gram-Schmidt Orthogonalisation Scheme", 2
									,"TRUE = GS_MODIFIED, FALSE = GS_CLASSICAL. See IDA manual section"
									" 5.5.6.6. Only applies when linsolve=SPGMR is selected."
								}, TRUE}
					);

					slv_param_int(p,IDA_PARAM_MAXNCF
							,(SlvParameterInitInt) { {"maxncf"
									,"Max nonlinear solver convergence failures per step", 2
									,"Maximum number of allowable nonlinear solver convergence failures"
									" on one step. See IDA manual section 5.5.6.1."
								}, 10,0,1000}
					);

					slv_param_char(p,IDA_PARAM_PREC
							,(SlvParameterInitChar) { {"prec"
									,"Preconditioner",1
									,"See IDA manual, section section 5.6.8."
								},"NONE"}, (char *[]) {"NONE","DIAG",NULL}
					);

					asc_assert(p->num_parms == IDA_PARAMS_SIZE);

					//CONSOLE_DEBUG("Created %d params", p->num_parms);

					return 0;
				}

const IntegratorInternals integrator_ida_internals = {
		integrator_ida_create, integrator_ida_params_default,
		integrator_ida_analyse, integrator_ida_solve,
		integrator_ida_write_matrix, integrator_ida_debug, integrator_ida_free,
		INTEG_IDA, "IDA" };


 int integrator_ida_params_default(IntegratorSystem *integ);
 void integrator_ida_free(void *enginedata);
 int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);
 void integrator_ida_create(IntegratorSystem *integ);
extern ASC_EXPORT int ida_register(void);





typedef void ( IntegratorVarVisitorFn)(IntegratorSystem *integ,
		struct var_variable *var, const int *varindx);
