/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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

*//*
	by Harry, 4th June 2015
	
	This file contains an implmentation of the prepare_integrator function
	that sets up the integration engine to perform event guided integration.
	The various tasks it performs include: allocate memory (ida_mem), obtain
	integrator configuration, analyse equations and configure IDA to setup 
	sufficient memory for the derivative and non-derivative variables.

*/





#define _GNU_SOURCE								

#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"										/* The list of includes needs cleaning up! Once basic files are written,  
#include "idaprec.h"										all commonly called functions can be added in just one header file.
#include "idacalc.h"										Once that's done, these includes must be revisited*/
#include "idaio.h"
#include "idaboundary.h"

#include <signal.h>										/*Check if these includes are necessary*/
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


int ida_prepare_integrator(IntegratorSystem *integ){
	flag = IDAInit(ida_mem, &integrator_ida_fex, t0, y0 ,yp0);

	/*-------------------                 RECORDING INTEGRATOR PREFERENCES                  --------------------*
			     The following block sets several preferences based on user choices.
			     The code implemented in this block was initially obtained from 
			     ida.c in the Ksenija2 directory.
 	*-----------------------------------------------------------------------------------------------------------*/











	/*Takes in a host of parameters for specifying integrator preferences*/
	static int integrator_ida_params_default(IntegratorSystem *integ) {
		asc_assert(integ!=NULL);
		asc_assert(integ->engine==INTEG_IDA);
		slv_parameters_t *p;
		p = &(integ->params);
		slv_destroy_parms(p);
		if (p->parms == NULL) {
			//CONSOLE_DEBUG("params NULL");
			p->parms = ASC_NEW_ARRAY(struct slv_parameter, IDA_PARAMS_SIZE);
			return -1;
			p->dynamic_parms = 1;
		}else{
		//CONSOLE_DEBUG("params not NULL");
		}
	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;
	/*The following calls to slv_param_* specify several preferences. See integrator manual for choice of available preferences in IDA*/
	slv_param_bool(p, IDA_PARAM_AUTODIFF,
			(SlvParameterInitBool){{"autodiff"
							,"Use auto-diff?",1
							,"Use automatic differentiation of expressions (1) or use numerical derivatives (0)"
						}, TRUE});

	slv_param_char(p,IDA_PARAM_CALCIC
			,(SlvParameterInitChar){{"calcic"
							,"Initial conditions calcuation",1
							,"Use specified values of ydot to solve for inital y (Y),"
							" or use the the values of the differential variables (yd) to solve"
							" for the pure algebraic variables (ya) along with the derivatives"
							" of the differential variables (yddot) (YA_YDP), or else don't solve"
							" the intial conditions at all (NONE). See IDA manual p 41 (IDASetId)"
						}, "YA_YDP"}, (char *[]) {"Y", "YA_YDP", "NONE",NULL});


	slv_param_bool(p,IDA_PARAM_SAFEEVAL
			,(SlvParameterInitBool){{"safeeval"
							,"Use safe evaluation?",1
							,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
							"throw SIGFPE errors which will then halt integration."
							}, FALSE});

	slv_param_bool(p,IDA_PARAM_ATOLVECT
			,(SlvParameterInitBool){{"atolvect"
							,"Use 'ode_atol' values as specified?",1
							,"If TRUE, values of 'ode_atol' are taken from your model and used "
							" in the integration. If FALSE, a scalar absolute tolerance value"
							" is shared by all variables. See IDA manual, section 5.5.1"
							}, TRUE});

	slv_param_real(p,IDA_PARAM_ATOL
			,(SlvParameterInitReal){{"atol"
							,"Scalar absolute error tolerance",1
							,"Value of the scalar absolute error tolerance. See also 'atolvect'."
							" See IDA manual, sections 5.5.1 and 5.5.2 'Advice on choice and use of tolerances'"
							}, 1e-5, 0, 1e10});

	slv_param_real(p,IDA_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
							,"Scalar relative error tolerance",1
							,"Value of the scalar relative error tolerance. (Note that for IDA,"
							" it's not possible to set per-variable relative tolerances as it is"
							" with LSODE)."
							" See IDA manual, section 5.5.2 'Advice on choice and use of tolerances'"
							}, 1e-4, 0, 1});

	slv_param_char(p,IDA_PARAM_LINSOLVER
			,(SlvParameterInitChar){{"linsolver"
							,"Linear solver",1
							,"See IDA manual, section 5.5.3. Choose 'ASCEND' to use the linsolqr"
							" direct linear solver bundled with ASCEND, 'DENSE' to use the dense"
							" solver bundled with IDA, or one of the Krylov solvers SPGMR, SPBCG"
							" or SPTFQMR (which still need preconditioners to be implemented"
							" before they can be very useful."},"DENSE"},(char *[]){"ASCEND","DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL});

	slv_param_int(p,IDA_PARAM_MAXL
			,(SlvParameterInitInt){{"maxl"
							,"Maximum Krylov dimension",0
							,"The maximum dimension of Krylov space used by the linear solver"
							" (for SPGMR, SPBCG, SPTFQMR) with IDA. See IDA manual section 5.5."
							" The default of 0 results in IDA using its internal default, which"
							" is currently a value of 5."
							}, 0, 0, 20});

	slv_param_int(p,IDA_PARAM_MAXORD
			,(SlvParameterInitInt){{"maxord"
							,"Maximum order of linear multistep method",0
							,"The maximum order of the linear multistep method with IDA. See"
							" IDA manual p 38."
							}, 5, 1, 5});

	slv_param_bool(p,IDA_PARAM_GSMODIFIED
			,(SlvParameterInitBool){{"gsmodified"
							,"Gram-Schmidt Orthogonalisation Scheme", 2
							,"TRUE = GS_MODIFIED, FALSE = GS_CLASSICAL. See IDA manual section"
							" 5.5.6.6. Only applies when linsolve=SPGMR is selected."
							}, TRUE});

	slv_param_int(p,IDA_PARAM_MAXNCF
			,(SlvParameterInitInt){{"maxncf"
							,"Max nonlinear solver convergence failures per step", 2
							,"Maximum number of allowable nonlinear solver convergence failures"
							" on one step. See IDA manual section 5.5.6.1."
							}, 10,0,1000});

	slv_param_char(p,IDA_PARAM_PREC
			,(SlvParameterInitChar){{"prec"
							,"Preconditioner",1
							,"See IDA manual, section section 5.6.8."
							},"NONE"}, (char *[]) {"NONE","DIAG",NULL});

	/*Check to verify the number of parameters are correct*/
	asc_assert(p->num_parms == IDA_PARAMS_SIZE);
	//CONSOLE_DEBUG("Created %d params", p->num_parms);
	









	/*-------------------               PASSING ON USER PREFERENCES TO INTEGRATOR           --------------------*
	                     In the following block, the user preferences for several integrator 
			     settings are passed on to the integrator engine.		        

 	*-----------------------------------------------------------------------------------------------------------*/












	/*Passing initial calculation details*/
													/*Passing initial calculation details*/
	icopt = 0;
	if (strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC), "Y") == 0) {
		CONSOLE_DEBUG("Solving initial conditions using values of yddot");
		icopt = IDA_Y_INIT;
		asc_assert(icopt!=0);
	}else if (strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC), "YA_YDP") == 0){
		CONSOLE_DEBUG("Solving initial conditions using values of yd");
		icopt = IDA_YA_YDP_INIT;
		asc_assert(icopt!=0);
	flag = IDACalcIC(ida_mem, icopt, tout1);

	/*Passing safe-evaluation option*/								/*Passing safe-evaluation option*/

#ifdef ASC_SIGNAL_TRAPS                                                                                  /*Fix Dependencies here? Re-visit!*/
		if(enginedata->safeeval) {
		CONSOLE_DEBUG("SETTING TO IGNORE SIGFPE...");
		Asc_SignalHandlerPush(SIGFPE, SIG_DFL);
		}else{
#ifdef FEX_DEBUG
			CONSOLE_DEBUG("SETTING TO CATCH SIGFPE...");
#endif
	Asc_SignalHandlerPushDefault(SIGFPE);
		}

	/*Passing tolerance preferences*/								/*Passing tolerance preferences*/

	int ida_malloc(IntegratorSystem *integ, void *ida_mem, realtype t0,
		N_Vector y0, N_Vector yp0) {
		int flag;
		N_Vector abstolvect;
		realtype reltol, abstol;
		/* relative error tolerance */
		reltol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_RTOL);
		//CONSOLE_DEBUG("rtol = %8.2e",reltol);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		flag = IDAInit(ida_mem, &integrator_ida_fex, t0, y0 ,yp0);
#else
		if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)) {
			/* vector of absolute tolerances */
			CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
			abstolvect = N_VNew_Serial(integ->n_y);
			integrator_get_atol(integ, NV_DATA_S(abstolvect));
			flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, 
			reltol, abstolvect);
			N_VDestroy_Serial(abstolvect);
		}else{
		/* scalar absolute tolerance (one value for all) */
		abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
		CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS,
		reltol, &abstol);
		}
#endif

	/*Passing solver-type preferences*/								/*Passing solver-type preferences*/
linsolver = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_LINSOLVER);
	//CONSOLE_DEBUG("ASSIGNING LINEAR SOLVER '%s'",linsolver);
	if(strcmp(linsolver, "ASCEND") == 0) {
		CONSOLE_DEBUG("ASCEND DIRECT SOLVER, size = %d",integ->n_y);
		IDAASCEND(ida_mem, integ->n_y);
		IDAASCENDSetJacFn(ida_mem, &integrator_ida_sjex, (void *) integ);
		enginedata->flagfntype = "IDAASCEND";
		enginedata->flagfn = &IDAASCENDGetLastFlag;
		enginedata->flagnamefn = &IDAASCENDGetReturnFlagName;
	}else if (strcmp(linsolver, "DENSE") == 0){
		//CONSOLE_DEBUG("DENSE DIRECT SOLVER, size = %d",integ->n_y);
		flag = IDADense(ida_mem, integ->n_y);
		switch(flag){
		case IDADENSE_SUCCESS:
			break;
		case IDADENSE_MEM_NULL:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 5;
		case IDADENSE_ILL_INPUT:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDADENSE is not compatible with current nvector module");
			return 5;
		case IDADENSE_MEM_FAIL:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Memory allocation failed for IDADENSE");
			return 5;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"bad return");
			return 5;
		}

		if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)) {
			//CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			flag = IDADlsSetDenseJacFn(ida_mem, &integrator_ida_djex);
#else
			flag = IDADenseSetJacFn(ida_mem, &integrator_ida_djex,
					(void *) integ);
#endif
			switch (flag) {
			case IDADENSE_SUCCESS:
				break;
			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed IDADenseSetJacFn");
				return 6;
			}
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}

		enginedata->flagfntype = "IDADENSE";
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		enginedata->flagfn = &IDADlsGetLastFlag;
		enginedata->flagnamefn = &IDADlsGetReturnFlagName;
#else
		enginedata->flagfn = &IDADenseGetLastFlag;
		enginedata->flagnamefn = &IDADenseGetReturnFlagName;
#endif
		}else{
		/* remaining methods are all SPILS */
		CONSOLE_DEBUG("IDA SPILS");
		maxl = SLV_PARAM_INT(&(integ->params),IDA_PARAM_MAXL);
		CONSOLE_DEBUG("maxl = %d",maxl);
		
		/* what preconditioner for SPILS solver? */

		pname = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_PREC);
		if(strcmp(pname, "NONE") == 0){
			prec = NULL;
		}else if(strcmp(pname, "JACOBI") == 0){
			prec = &prec_jacobi;
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid preconditioner choice '%s'",pname);
			return 7;
		}

		/* which SPILS linear solver? */
		if(strcmp(linsolver, "SPGMR") == 0){
			CONSOLE_DEBUG("IDA SPGMR");
			flag = IDASpgmr(ida_mem, maxl); /* 0 means use the default max Krylov dimension of 5 */
		}else if(strcmp(linsolver, "SPBCG") == 0){
			CONSOLE_DEBUG("IDA SPBCG");
			flag = IDASpbcg(ida_mem, maxl);
		}else if(strcmp(linsolver, "SPTFQMR") == 0){
			CONSOLE_DEBUG("IDA SPTFQMR");
			flag = IDASptfqmr(ida_mem, maxl);
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown IDA linear solver choice '%s'",linsolver);
			return 8;
		}

		if(prec){
			/* assign the preconditioner to the linear solver */
			(prec->pcreate)(integ);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve);
#else
			IDASpilsSetPreconditioner(ida_mem, prec->psetup, prec->psolve,
					(void *) integ);
#endif
			CONSOLE_DEBUG("PRECONDITIONER = %s",pname);
		}else{
			CONSOLE_DEBUG("No preconditioner");
		}

		enginedata->flagfntype = "IDASPILS";
		enginedata->flagfn = &IDASpilsGetLastFlag;
		enginedata->flagnamefn = &IDASpilsGetReturnFlagName;
		if(flag == IDASPILS_MEM_NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 9;
		}else if (flag == IDASPILS_MEM_FAIL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDASpgmr)");
			return 9;
		}/* else success */
		/* assign the J*v function */
		if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)) {
			CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex);
#else
			flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex,
					(void *) integ);
#endif
			if(flag == IDASPILS_MEM_NULL) {
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
				return 10;
			}else if (flag == IDASPILS_LMEM_NULL) {
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDASPILS linear solver has not been initialized");
				return 10;
			}/* else success */
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}
		if(strcmp(linsolver, "SPGMR") == 0) {
			/* select Gram-Schmidt orthogonalisation */
			if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_GSMODIFIED)) {
				CONSOLE_DEBUG("USING MODIFIED GS");
				flag = IDASpilsSetGSType(ida_mem, MODIFIED_GS);
				if(flag != IDASPILS_SUCCESS) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			}else{
				CONSOLE_DEBUG("USING CLASSICAL GS");
				flag = IDASpilsSetGSType(ida_mem, CLASSICAL_GS);
				if(flag != IDASPILS_SUCCESS) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			}
		}
	}

	/*Passing Maxord and Maxncf values*/							/*Passing Maxord and Maxncf values*/

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	IDASetUserData(ida_mem, (void *)integ);
#else
	IDASetRdata(ida_mem, (void *) integ);
#endif
	IDASetMaxStep(ida_mem, integrator_get_maxstep(integ));
	IDASetInitStep(ida_mem, integrator_get_stepzero(integ));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(integ));
	if (integrator_get_minstep(integ) > 0) {
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"IDA does not support minstep (ignored)\n");
	}
	//CONSOLE_DEBUG("MAXNCF = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));
	IDASetMaxConvFails(ida_mem, SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));
	//CONSOLE_DEBUG("MAXORD = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));
	IDASetMaxOrd(ida_mem, SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));








	/*-------------------                            ANALYSE EQUATIONS                      --------------------*
			     The following block analyses the equations passed on from the 
			     GUI to aid integration. This block runs a series of checks, the 
			     process is stopped when flag takes a non-zero value.
 	*-----------------------------------------------------------------------------------------------------------*/

	flag =  integrator_ida_check_vars(integ) + integrator_ida_flag_rels(integ) 
	+ integrator_ida_sort_rels_and_vars(integ) + integrator_ida_create_lists(integ) + 
	integrator_ida_analyse(integ) + integrator_ida_check_partitioning(integ)+
	integrator_ida_check_diffindex(integ);

	if(flag!=0){
	return -2;						/*Failed analysis*/
	}







	/*-------------------                     ALLOCATE MEMORY FOR YDOT, Y ETC              --------------------*
			     The following block analyses the equations passed on from the 
			     GUI to aid integration. This block runs a series of checks, the 
			     process is stopped when flag takes a non-zero value.
 	*-----------------------------------------------------------------------------------------------------------*/



		/*This block has to be discussed with Ksenija before implamentation. Is this block necessary? 
		The IDA Manual says that most memory allocation procedures are taken care of. Also there's the 
		implementation of IDAMalloc, which needs to be cleared up. Also ida_malloc has already been called above
		this does the same job of allocating ydot and ypdot. Anything else needs to be done here? */






return 0;		/*prepare_integrator successful!*/					
}	
