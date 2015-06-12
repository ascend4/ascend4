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


int ida_prepare_integrator(IntegratorSystem *integ){
	/*Allocating memory for the ida_mem object*/
	ida_mem = IDACreate();






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
			,(SlvParameterInitInt) { {"maxncf"
							,"Max nonlinear solver convergence failures per step", 2
							,"Maximum number of allowable nonlinear solver convergence failures"
							" on one step. See IDA manual section 5.5.6.1."
							}, 10,0,1000});

	slv_param_char(p,IDA_PARAM_PREC
			,(SlvParameterInitChar) { {"prec"
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

	/*Passing safe-evaluation option*/

#ifdef ASC_SIGNAL_TRAPS
		if(enginedata->safeeval) {
		CONSOLE_DEBUG("SETTING TO IGNORE SIGFPE...");
		Asc_SignalHandlerPush(SIGFPE, SIG_DFL);
		}else{
# ifdef FEX_DEBUG
			CONSOLE_DEBUG("SETTING TO CATCH SIGFPE...");
# endif
	Asc_SignalHandlerPushDefault(SIGFPE);
		}

	/*Passing safe-evaluation option*/






//////////////////////////////////////////////////////////////*TO BE CONTINUED*//////////////////////////////////////////////////


					
}	