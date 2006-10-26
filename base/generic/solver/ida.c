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
	Be careful with the following. You need to have installed SUNDIALS on a
	directory which your build tool has on its include path. Note that
	the local "ida.h" would have to be referred to as <solver/ida.h> if you 
	wanted to use angle-brackets. You're not supposed to add the *sub*-
	directories of base/generic to your include path.
*/

/* standard includes */
#include <signal.h>

/* ASCEND includes */
#include "ida.h"
#include <utilities/error.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
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
# include <ida/ida.h>
# include <nvector/nvector_serial.h>
# include <ida/ida_spgmr.h>
# ifndef IDA_SUCCESS
#  error "Failed to include SUNDIALS IDA header file"
# endif
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
	struct var_variable **varlist;   /**< NULL terminated list of rels */
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
	realtype t0, reltol, t, tret, tout1;
	N_Vector y0, yp0, abstol, ypret, yret;
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
	t0 = samplelist_get(blsys->samples,start_index);

	y0 = N_VNew_Serial(size);
	integrator_get_y(blsys,NV_DATA_S(y0));

	yp0 = N_VNew_Serial(size);
	integrator_get_ydot(blsys,NV_DATA_S(yp0));

	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("yp0 is at %p",&yp0);

	/* create IDA object */
	ida_mem = IDACreate();

	/* retrieve the absolute tolerance values for each variable */
	abstol = N_VNew_Serial(size);
	N_VConst(0.1,abstol); /** @TODO fill in the abstol values from the model */
	reltol = 0.001;

	/* allocate internal memory */
	flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstol);
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
    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)blsys);
	if(flag==IDASPILS_MEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 0;
	}else if(flag==IDASPILS_LMEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDASPILS linear solver has not been initialized");
		return 0;
	}/* else success */

	/* set linear solver optional inputs...

		...nothing here at the moment... 

	*/

	/* correct initial values, given derivatives */
	blsys->currentstep=0;
 	t_index=start_index+1;
	tout1 = samplelist_get(blsys->samples, t_index);
	flag = IDACalcIC(ida_mem, t0, y0, yp0, IDA_Y_INIT, tout1);
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

		CONSOLE_DEBUG("SOLVING UP TO t = %f", t);
	
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
	Function to evaluate the product J*v, in the form required for IDA.

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

	int *variables;
	double *derivatives;
	var_filter_t filter;
	int count;

	CONSOLE_DEBUG("EVALUTING JACOBIAN...");

	blsys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(blsys);

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));
	/* no real use for residuals (rr) here, I don't think? */

	/* allocate space for returns from relman_diff2 */
	variables = ASC_NEW_ARRAY(int, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/* evaluate the derivatives... */
	/* J = dG_dy = dF_dy + alpha * dF_dyp */

	filter.matchbits = VAR_SVAR;
	filter.matchvalue = VAR_SVAR;

	Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	if (setjmp(g_fpe_env)==0) {
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* get derivatives for this particular relation */
			status = relman_diff2(*relptr, &filter, derivatives, variables, &count, enginedata->safeeval);

			CONSOLE_DEBUG("Got derivatives against %d matching variables", count);

			relname = rel_make_name(blsys->system, *relptr);
			if(!status){
				fprintf(stderr,"\n\n");
				CONSOLE_DEBUG("Derivatives for relation %d '%s' OK",i,relname);
			}else{
				CONSOLE_DEBUG("ERROR calculating derivatives for relation %d '%s'",i,relname);
				break;
			}
			ASC_FREE(relname);

			Jv_i = 0;
			for(j=0; j < count; ++j){
				/* CONSOLE_DEBUG("j = %d, variables[j] = %d, n_y = %ld", j, variables[j], blsys->n_y); */
				varname = var_make_name(blsys->system, enginedata->varlist[variables[j]]);
				if(varname){
					CONSOLE_DEBUG("Variable %d '%s' derivative = %f", variables[j],varname,derivatives[j]);
					ASC_FREE(varname);
				}else{
					CONSOLE_DEBUG("Variable %d: derivative = %f",variables[j],derivatives[j]);
				}

				Jv_i += derivatives[j] * NV_Ith_S(v,variables[j]);
			}

			NV_Ith_S(Jv,i) = Jv_i;
			CONSOLE_DEBUG("(J*v)[%d] = %f", i, Jv_i);

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
