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
*//**
	@file
	Access to the IDA integrator for ASCEND. IDA is a DAE solver that comes
	as part of the GPL-licensed SUNDIALS solver package from LLNL.
	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

/* 
	Be careful with the following. This file requires both the 'ida.h' from
	SUNDIALS as well as the 'ida.h' from ASCEND. Make sure that we're getting
	both of these; if you get problems check your build tool for the paths being
	passed to the C preprocessor.
*/

/* standard includes */
#include <signal.h>

/* ASCEND includes */
#include "ida.h"
#include <utilities/error.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascPanic.h>
#include <compiler/instance_enum.h>
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_common.h"
#include "slv_client.h"
#include "relman.h"

/* SUNDIALS includes */
#ifdef ASC_WITH_IDA
# include <sundials/sundials_config.h>
# include <ida/ida.h>
# include <nvector/nvector_serial.h>
# include <ida/ida_spgmr.h>
# ifndef IDA_SUCCESS
#  error "Failed to include SUNDIALS IDA header file"
# endif
#endif

/*
	for the benefit of build tools that didn't sniff the SUNDIALS version, we
	assume version 2.2.x (and thence possible errors).
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

/* check that we've got what we expect now */
#ifndef ASC_IDA_H
# error "Failed to include ASCEND IDA header file"
#endif

/**
	Struct containing any stuff that IDA needs that doesn't fit into the 
	common IntegratorSystem struct.
*/
typedef struct{
	struct rel_relation **rellist;   /**< NULL terminated list of rels */
	struct var_variable **varlist;   /**< NULL terminated list of vars. ONLY USED FOR DEBUGGING -- get rid of it! */
	int nrels;
	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
} IntegratorIdaData;

/*-------------------------------------------------------------
  FORWARD DECLS
*/
/* residual function forward declaration */
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data);

int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
);

/* error handler forward declaration */
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
);

/*-------------------------------------------------------------
  SETUP/TEARDOWN ROUTINES
*/
void integrator_ida_create(IntegratorSystem *blsys){
	CONSOLE_DEBUG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	enginedata->rellist = NULL;
	enginedata->varlist = NULL;
	enginedata->safeeval = 1;
	blsys->enginedata = (void *)enginedata;
	integrator_ida_params_default(blsys);
}

void integrator_ida_free(void *enginedata){
	CONSOLE_DEBUG("DELETING IDA ENGINE DATA");
	IntegratorIdaData *d = (IntegratorIdaData *)enginedata;
	/* note, we don't own the rellist, so don't need to free it */
	ASC_FREE(d);
}

IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *blsys){
	IntegratorIdaData *d;
	assert(blsys!=NULL);
	assert(blsys->enginedata!=NULL);
	assert(blsys->engine==INTEG_IDA);
	d = ((IntegratorIdaData *)(blsys->enginedata));
	assert(d->safeeval = 1);
	return d;
}

/*-------------------------------------------------------------
  PARAMETERS FOR IDA
*/

enum ida_parameters{
	IDA_PARAM_AUTODIFF
	,IDA_PARAM_RTOL
	,IDA_PARAM_ATOL
	,IDA_PARAM_ATOLVECT
	,IDA_PARAMS_SIZE
};

/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the blsys->params structure.

	@return 0 on success
*/
int integrator_ida_params_default(IntegratorSystem *blsys){
	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL){
		CONSOLE_DEBUG("params NULL");
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, IDA_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	}else{
		CONSOLE_DEBUG("params not NULL");
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(p,IDA_PARAM_AUTODIFF
			,(SlvParameterInitBool){{"autodiff"
			,"Use auto-diff?",1
			,"Use automatic differentiation of expressions (1) or use numerical derivatives (0)"
		}, TRUE}
	);

	slv_param_bool(p,IDA_PARAM_ATOLVECT
			,(SlvParameterInitBool){{"atolvect"
			,"Use 'ode_atol' values as specified?",1
			,"If TRUE, values of 'ode_atol' are taken from your model and used "
			" in the integration. If FALSE, a scalar absolute tolerance value"
			" is shared by all variables. See IDA manual, section 5.5.1"
		}, TRUE }
	);

	slv_param_real(p,IDA_PARAM_ATOL
			,(SlvParameterInitReal){{"atol"
			,"Scalar absolute error tolerance",1
			,"Value of the scalar absolute error tolerance. See also 'atolvect'."
			" See IDA manual, section 5.5.1"
		}, 1e-5, DBL_MIN, DBL_MAX }
	);

	slv_param_real(p,IDA_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Value of the scalar relative error tolerance."
			" See IDA manual, section 5.5.1"
		}, 1e-5, DBL_MIN, DBL_MAX }
	);

	asc_assert(p->num_parms == IDA_PARAMS_SIZE);

	CONSOLE_DEBUG("Created %d params", p->num_parms);

	return 0;
}

/*-------------------------------------------------------------
  MAIN IDA SOLVER ROUTINE, see IDA manual, sec 5.4, p. 27 ff.
*/

/* return 1 on success */
int integrator_ida_solve(
		IntegratorSystem *blsys
		, unsigned long start_index
		, unsigned long finish_index
){
	void *ida_mem;
	int size, flag, t_index;
	realtype t0, reltol, abstol, t, tret, tout1;
	N_Vector y0, yp0, abstolvect, ypret, yret;
	IntegratorIdaData *enginedata;

	CONSOLE_DEBUG("STARTING IDA...");

	enginedata = integrator_ida_enginedata(blsys);
	CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);

	/* store reference to list of relations (in enginedata) */
	enginedata->nrels = slv_get_num_solvers_rels(blsys->system);
	enginedata->rellist = slv_get_solvers_rel_list(blsys->system);
	enginedata->varlist = slv_get_solvers_var_list(blsys->system);
	CONSOLE_DEBUG("Number of relations: %d",enginedata->nrels);
	CONSOLE_DEBUG("Number of dependent vars: %ld",blsys->n_y);
	size = blsys->n_y;

	if(enginedata->nrels!=size){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration problem is not square (%d rels, %d vars)", enginedata->nrels, size);
		return 0; /* failure */
	}

	/* retrieve initial values from the system */
	
	/** @TODO fix this, the starting time != first sample */
	t0 = integrator_get_t(blsys);
	CONSOLE_DEBUG("RETRIEVED t0 = %f",t0);

	CONSOLE_DEBUG("RETRIEVING y0");

	y0 = N_VNew_Serial(size);
	integrator_get_y(blsys,NV_DATA_S(y0));

	CONSOLE_DEBUG("RETRIEVING yp0");

	yp0 = N_VNew_Serial(size);
	integrator_get_ydot(blsys,NV_DATA_S(yp0));

	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("yp0 is at %p",&yp0);

	/* create IDA object */
	ida_mem = IDACreate();

	/* relative error tolerance */	
	reltol = SLV_PARAM_REAL(&(blsys->params),IDA_PARAM_RTOL);

	/* allocate internal memory */
	if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_ATOLVECT)){
		/* vector of absolute tolerances */
		CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
		abstolvect = N_VNew_Serial(size);
		integrator_get_atol(blsys,NV_DATA_S(abstolvect));

		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect);
	}else{
		/* scalar absolute tolerance (one value for all) */
		CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
		abstol = SLV_PARAM_REAL(&(blsys->params),IDA_PARAM_ATOL);
		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS, reltol, &abstol);

	}

	if(flag==IDA_MEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 0;
	}else if(flag==IDA_MEM_FAIL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDAMalloc)");
		return 0;
	}else if(flag==IDA_ILL_INPUT){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid input to IDAMalloc");
		return 0;
	}/* else success */

	/* set optional inputs... */
	IDASetErrHandlerFn(ida_mem, &integrator_ida_error, (void *)blsys);
	IDASetRdata(ida_mem, (void *)blsys);
	IDASetMaxStep(ida_mem, integrator_get_maxstep(blsys));
	IDASetInitStep(ida_mem, integrator_get_stepzero(blsys));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(blsys));
	/* there's no capability for setting *minimum* step size in IDA */

	CONSOLE_DEBUG("ASSIGNING LINEAR SOLVER");

	/* attach linear solver module, using the default value of maxl */
	flag = IDASpgmr(ida_mem, 0);
	if(flag==IDASPILS_MEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 0;
	}else if(flag==IDASPILS_MEM_FAIL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDASpgmr)");
		return 0;
	}/* else success */

	/* assign the J*v function */
	if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_AUTODIFF)){
		CONSOLE_DEBUG("USING AUTODIFF");
	    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)blsys);
		if(flag==IDASPILS_MEM_NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 0;
		}else if(flag==IDASPILS_LMEM_NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDASPILS linear solver has not been initialized");
			return 0;
		}/* else success */
	}else{
		CONSOLE_DEBUG("USING NUMERICAL DIFF");
	}		

	/* set linear solver optional inputs...

		...nothing here at the moment... 

	*/

	/* correct initial values, given derivatives */
	blsys->currentstep=0;
 	t_index=start_index;
	tout1 = samplelist_get(blsys->samples, t_index);

	/* CONSOLE_DEBUG("Giving t value %f to IDACalcIC", tout1);*/

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==3
	/* note the new API from version 2.3 and onwards */
	flag = IDACalcIC(ida_mem, IDA_Y_INIT, tout1);
#else
	flag = IDACalcIC(ida_mem, t0, y0, yp0, IDA_Y_INIT, tout1);
#endif

	if(flag!=IDA_SUCCESS){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to solve initial values (IDACalcIC)");
		return 0;
	}/* else success */

	CONSOLE_DEBUG("INITIAL CONDITIONS SOLVED :-)");

	/* optionally, specify ROO-FINDING PROBLEM */

	/* -- set up the IntegratorReporter */
	integrator_output_init(blsys);

	/* -- store the initial values of all the stuff */
	integrator_output_write(blsys);
	integrator_output_write_obs(blsys);

	/* specify where the returned values should be stored */
	yret = y0;
	ypret = yp0;

	/* advance solution in time, return values as yret and derivatives as ypret */
	blsys->currentstep=1;
	for(t_index=start_index+1;t_index <= finish_index;++t_index, ++blsys->currentstep){
		t = samplelist_get(blsys->samples, t_index);

		/* CONSOLE_DEBUG("SOLVING UP TO t = %f", t); */
	
		flag = IDASolve(ida_mem, t, &tret, yret, ypret, IDA_NORMAL);

		/* pass the values of everything back to the compiler */
		integrator_set_t(blsys, (double)tret);
		integrator_set_y(blsys, NV_DATA_S(yret));
		integrator_set_ydot(blsys, NV_DATA_S(ypret));

		if(flag<0){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve t = %f (IDASolve), error %d", t, flag);
			break;
		}

		/* -- do something so that blsys knows the values of tret, yret and ypret */

		/* -- store the current values of all the stuff */
		integrator_output_write(blsys);
		integrator_output_write_obs(blsys);
	}

	/* -- close the IntegratorReporter */
	integrator_output_close(blsys);

	if(flag < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Solving aborted while attempting t = %f", t);
		return 0;
	}

	/* get optional outputs */
	
	/* free solution memory */
	N_VDestroy_Serial(yret);
	N_VDestroy_Serial(ypret);
	
	/* free solver memory */
	IDAFree(ida_mem);

	/* all done */
	return 1;
}

/*--------------------------------------------------
  RESIDUALS AND JACOBIAN
*/
/**
	Function to evaluate system residuals, in the form required for IDA.

	Given tt, yy and yp, we need to evaluate and return rr.

	@param tt current value of indep variable (time)
	@param yy current values of dependent variable vector
	@param yp current values of derivatives of dependent variables
	@param rr the output residual vector (is we're returning data to)
	@param res_data pointer to our stuff (blsys in this case).

	@return 0 on success, positive on recoverable error, and
		negative on unrecoverable error.
*/
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data){
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	int i, calc_ok, is_error;
	struct rel_relation** relptr;
	double resid;
	char *relname;

	blsys = (IntegratorSystem *)res_data;
	enginedata = integrator_ida_enginedata(blsys);

	/* fprintf(stderr,"\n\n"); */
	/* CONSOLE_DEBUG("ABOUT TO EVALUTE RESIDUALS..."); */

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));

	/* revaluate the system residuals using the new data */
	is_error = 0; 
	relptr = enginedata->rellist;

	/* CONSOLE_DEBUG("IDA requests residuals of length %lu",NV_LENGTH_S(rr)); */
	if(NV_LENGTH_S(rr)!=enginedata->nrels){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid residuals nrels!=length(rr)");
		return -1; /* unrecoverable */
	}

	Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	if (setjmp(g_fpe_env)==0) {
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			resid = relman_eval(*relptr, &calc_ok, enginedata->safeeval);
			
			relname = rel_make_name(blsys->system, *relptr);
			/* if(calc_ok){
				CONSOLE_DEBUG("residual[%d:\"%s\"] = %f",i,relname,resid);
			}else{
				CONSOLE_DEBUG("residual[%d:\"%s\"] = %f (ERROR)",i,relname,resid);
			}*/
			ASC_FREE(relname);

			NV_Ith_S(rr,i) = resid;
			if(!calc_ok){
				/* presumable some output already made? */
				is_error = 1;
			}
		}
	}else{
		CONSOLE_DEBUG("FLOATING POINT ERROR WITH i=%d",i);
	}
	Asc_SignalHandlerPop(SIGFPE,SIG_IGN);

	if(is_error)CONSOLE_DEBUG("SOME ERRORS FOUND IN EVALUATION");
	return is_error;
}

/**
	Function to evaluate the product J*v, in the form required for IDA (see IDASpilsSetJacTimesVecFn)

	Given tt, yy, yp, rr and v, we need to evaluate and return Jv.

	@param tt current value of the independent variable (time, t)
	@param yy current value of the dependent variable vector, y(t).
	@param yp current value of y'(t).
	@param rr current value of the residual vector F(t, y, y').
	@param v  the vector by which the Jacobian must be multiplied to the right.
	@param Jv the output vector computed
	@param c_j the scalar in the system Jacobian, proportional to the inverse of the step size ($ \alpha$ in Eq. (3.5) ).
	@param jac_data pointer to our stuff (blsys in this case, passed into IDA via IDASp*SetJacTimesVecFn.)
	@param tmp1 @see tmp2
	@param tmp2 (as well as tmp1) pointers to memory allocated for variables of type N_Vector for use here as temporary storage or work space.
	@return 0 on success
*/
int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
){
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	int i, j, is_error=0;
	struct rel_relation** relptr;
	char *relname, *varname;
	int status;
	double Jv_i;
	int var_yindex;

	int *variables;
	double *derivatives;
	var_filter_t filter;
	int count;

	/* fprintf(stderr,"\n--------------\n"); */
	/* CONSOLE_DEBUG("EVALUTING JACOBIAN..."); */

	blsys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(blsys);

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));
	/* no real use for residuals (rr) here, I don't think? */

	/* allocate space for returns from relman_diff2: we *should* be able to use 'tmp1' and 'tmp2' here... */
	variables = ASC_NEW_ARRAY(int, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/* evaluate the derivatives... */
	/* J = dG_dy = dF_dy + alpha * dF_dyp */

	filter.matchbits = VAR_SVAR;
	filter.matchvalue = VAR_SVAR;

	/* CONSOLE_DEBUG("PRINTING VALUES OF 'v' VECTOR (length %ld)",NV_LENGTH_S(v)); */
	/* for(i=0; i<NV_LENGTH_S(v); ++i){
		CONSOLE_DEBUG("v[%d] = %f",i,NV_Ith_S(v,i));
	}*/

	Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	if (setjmp(g_fpe_env)==0) {
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* fprintf(stderr,"\n"); */
			relname = rel_make_name(blsys->system, *relptr);
			/* CONSOLE_DEBUG("RELATION %d '%s'",i,relname); */
			ASC_FREE(relname);

			/* get derivatives for this particular relation */
			status = relman_diff2(*relptr, &filter, derivatives, variables, &count, enginedata->safeeval);
			/* CONSOLE_DEBUG("Got derivatives against %d matching variables", count); */

			for(j=0;j<count;++j){
				varname = var_make_name(blsys->system, enginedata->varlist[variables[j]]);
				/* CONSOLE_DEBUG("derivatives[%d] = %f (variable %d, '%s')",j,derivatives[j],variables[j],varname); */
				ASC_FREE(varname);
			}

			if(!status){
				/* CONSOLE_DEBUG("Derivatives for relation %d OK",i); */
			}else{
				CONSOLE_DEBUG("ERROR calculating derivatives for relation %d",i);
				break;
			}

			/*
				Now we have the derivatives wrt each alg/diff variable in the
				present equation. variables[] points into the varlist. need
				a mapping from the varlist to the y and ydot lists.
			*/

			Jv_i = 0;
			for(j=0; j < count; ++j){
				/* CONSOLE_DEBUG("j = %d, variables[j] = %d, n_y = %ld", j, variables[j], blsys->n_y); */
				varname = var_make_name(blsys->system, enginedata->varlist[variables[j]]);
				if(varname){
					/* CONSOLE_DEBUG("Variable %d '%s' derivative = %f", variables[j],varname,derivatives[j]); */
					ASC_FREE(varname);
				}else{
					CONSOLE_DEBUG("Variable %d (UNKNOWN!): derivative = %f",variables[j],derivatives[j]);
				}
				
				var_yindex = blsys->y_id[variables[j]];
				/* CONSOLE_DEBUG("j = %d: variables[j] = %d, y_id = %d",j,variables[j],var_yindex); */

				if(var_yindex >= 0){
					/* CONSOLE_DEBUG("j = %d: algebraic, deriv[j] = %f, v[%d] = %f",j,derivatives[j], var_yindex, NV_Ith_S(v,var_yindex)); */
					Jv_i += derivatives[j] * NV_Ith_S(v,var_yindex);
				}else{
					var_yindex = -var_yindex-1;
					/* CONSOLE_DEBUG("j = %d: differential, deriv[j] = %f, v[%d] = %f",j,derivatives[j], var_yindex, NV_Ith_S(v,var_yindex)); */
					Jv_i += derivatives[j] * NV_Ith_S(v,var_yindex) / c_j; 
				}
			}

			NV_Ith_S(Jv,i) = Jv_i;
			/* CONSOLE_DEBUG("(J*v)[%d] = %f", i, Jv_i); */

			if(status){
				/* presumably some error_reporter will already have been made*/
				is_error = 1;
			}
		}
	}else{
		CONSOLE_DEBUG("FLOATING POINT ERROR WITH i=%d",i);
	}
	Asc_SignalHandlerPop(SIGFPE,SIG_IGN);

	if(is_error)CONSOLE_DEBUG("SOME ERRORS FOUND IN EVALUATION");

	
	
	return is_error;
}

/*----------------------------------------------
  ERROR REPORTING
*/
/**
	Error message reporter function to be passed to IDA. All error messages
	will trigger a call to this function, so we should find everything
	appearing on the console (in the case of Tcl/Tk) or in the errors/warnings
	panel (in the case of PyGTK).
*/
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
){
	IntegratorSystem *blsys;
	error_severity_t sev;

	/* cast back the IntegratorSystem, just in case we need it */
	blsys = (IntegratorSystem *)eh_data;

	/* severity depends on the sign of the error_code value */
	if(error_code <= 0){
		sev = ASC_PROG_ERR;
	}else{
		sev = ASC_PROG_WARNING;
	}

	/* use our all-purpose error reporting to get stuff back to the GUI */
	error_reporter(sev,module,0,function,"%s (error %d)",msg,error_code);
}
