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
	Calculation of residuals, jacobian, etc, for the ASCEND wrapping of IDA.

	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

#define _GNU_SOURCE

#include "idacalc.h"
#include "idatypes.h"
#include "idalinear.h"
#include "idaanalyse.h"

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


/* #define FEX_DEBUG  */
#define JEX_DEBUG
/* #define DJEX_DEBUG */
/* #define ROOT_DEBUG */

/*--------------------------------------------------
  RESIDUALS AND JACOBIAN AND IDAROOTFN
*/

#if 0
typedef void (SignalHandlerFn)(int);
SignalHandlerFn integrator_ida_sig;
SignalHandlerFn *integrator_ida_sig_old;
jmp_buf integrator_ida_jmp_buf;
fenv_t integrator_ida_fenv_old;


void integrator_ida_write_feinfo(){
	int f;
	f = fegetexcept();
	CONSOLE_DEBUG("Locating nature of exception...");
	if(f & FE_DIVBYZERO)ERROR_REPORTER_HERE(ASC_PROG_ERR,"DIV BY ZERO");
	if(f & FE_INEXACT)ERROR_REPORTER_HERE(ASC_PROG_ERR,"INEXACT");
	if(f & FE_INVALID)ERROR_REPORTER_HERE(ASC_PROG_ERR,"INVALID");
	if(f & FE_OVERFLOW)ERROR_REPORTER_HERE(ASC_PROG_ERR,"OVERFLOW");
	if(f & FE_UNDERFLOW)ERROR_REPORTER_HERE(ASC_PROG_ERR,"UNDERFLOW");
	if(f==0)ERROR_REPORTER_HERE(ASC_PROG_ERR,"FLAGS ARE CLEAR?!?");
}

void integrator_ida_sig(int sig){
	/* the wrong signal: rethrow to the default handler */
	if(sig!=SIGFPE){
		signal(SIGFPE,SIG_DFL);
		raise(sig);
	}
	integrator_ida_write_feinfo();
	CONSOLE_DEBUG("Caught SIGFPE=%d (in signal handler). Jumping to...",sig);
	longjmp(integrator_ida_jmp_buf,sig);
}
#endif

/**
	Function to evaluate system residuals, in the form required for IDA.

	Given tt, yy and yp, we need to evaluate and return rr.

	@param tt current value of indep variable (time)
	@param yy current values of dependent variable vector
	@param yp current values of derivatives of dependent variables
	@param rr the output residual vector (is we're returning data to)
	@param res_data pointer to our stuff (integ in this case).

	@return 0 on success, positive on recoverable error, and
		negative on unrecoverable error.
*/
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data){
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	int i, calc_ok, is_error;
	struct rel_relation** relptr;
	double resid;
	char *relname;
#ifdef FEX_DEBUG
	char *varname;
	char diffname[30];
#endif

	integ = (IntegratorSystem *)res_data;
	enginedata = integrator_ida_enginedata(integ);

#ifdef FEX_DEBUG
	/* fprintf(stderr,"\n\n"); */
	CONSOLE_DEBUG("EVALUTE RESIDUALS...");
#endif

	if(NV_LENGTH_S(rr)!=enginedata->nrels){
		CONSOLE_DEBUG("y");
		N_VPrint_Serial(yy);
		CONSOLE_DEBUG("yp");
		N_VPrint_Serial(yp);
		CONSOLE_DEBUG("r");
		N_VPrint_Serial(rr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid residuals nrels!=length(rr)");
		return -1; /* unrecoverable */
	}

	/* pass the values of everything back to the compiler */
	integrator_set_t(integ, (double)tt);
	integrator_set_y(integ, NV_DATA_S(yy));
	integrator_set_ydot(integ, NV_DATA_S(yp));

	/* perform bounds checking on all variables */
	if(slv_check_bounds(integ->system, 0, -1, NULL)){
		/* ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Variable(s) out of bounds"); */
		return 1;
	}

	/* evaluate each residual in the rellist */
	is_error = 0;
	relptr = enginedata->rellist;


#ifdef ASC_SIGNAL_TRAPS
	if(enginedata->safeeval){
		Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	}else{
# ifdef FEX_DEBUG
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SETTING TO CATCH SIGFPE...");
# endif
		Asc_SignalHandlerPushDefault(SIGFPE);
	}

	if (SETJMP(g_fpe_env)==0) {
#endif


	for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
	){
		resid = relman_eval(*relptr, &calc_ok, enginedata->safeeval);

		NV_Ith_S(rr,i) = resid;
		if(!calc_ok){
			relname = rel_make_name(integ->system, *relptr);
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
			ASC_FREE(relname);
			/* presumable some output already made? */
			is_error = 1;
		}/*else{
			CONSOLE_DEBUG("Calc OK");
		}*/
	}

	if(!is_error){
		for(i=0;i< enginedata->nrels; ++i){
			if(isnan(NV_Ith_S(rr,i))){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"NAN detected in residual %d",i);
				is_error=1;
			}
		}
#ifdef FEX_DEBUG
		if(!is_error){
			CONSOLE_DEBUG("No NAN detected");
		}
#endif
	}

#ifdef ASC_SIGNAL_TRAPS
	}else{
		relname = rel_make_name(integ->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}

	if(enginedata->safeeval){
		Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
	}else{
		Asc_SignalHandlerPopDefault(SIGFPE);
	}
#endif


#ifdef FEX_DEBUG
	/* output residuals to console */
	CONSOLE_DEBUG("RESIDUAL OUTPUT");
	fprintf(stderr,"index\t%25s\t%25s\t%s\n","y","ydot","resid");
	for(i=0; i<integ->n_y; ++i){
		varname = var_make_name(integ->system,integ->y[i]);
		fprintf(stderr,"%d\t%15s=%10f\t",i,varname,NV_Ith_S(yy,i));
		if(integ->ydot[i]){
			varname = var_make_name(integ->system,integ->ydot[i]);
			fprintf(stderr,"%15s=%10f\t",varname,NV_Ith_S(yp,i));
		}else{
			snprintf(diffname,99,"diff(%s)",varname);
			fprintf(stderr,"%15s=%10f\t",diffname,NV_Ith_S(yp,i));
		}
		ASC_FREE(varname);
		relname = rel_make_name(integ->system,enginedata->rellist[i]);
		fprintf(stderr,"'%s'=%f (%p)\n",relname,NV_Ith_S(rr,i),enginedata->rellist[i]);
	}
#endif

	if(is_error){
		return 1;
	}

#ifdef FEX_DEBUG
	CONSOLE_DEBUG("RESIDUAL OK");
#endif
	return 0;
}

/**
	Dense Jacobian evaluation. Only suitable for small problems!
	Has been seen working for problems up to around 2000 vars, FWIW.
*/
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
int integrator_ida_djex(int Neq, realtype tt, realtype c_j
		, N_Vector yy, N_Vector yp, N_Vector rr
		, IDA_MTX_T Jac, void *jac_data
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
#else
int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, IDA_MTX_T Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
#endif
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	char *relname;
#ifdef DJEX_DEBUG
	struct var_variable **varlist;
	char *varname;
#endif
	struct rel_relation **relptr;
	int i;
	double *derivatives;
	struct var_variable **variables;
	int count, j;
	int status, is_error = 0;

	integ = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(integ);

	/* allocate space for returns from relman_diff3 */
	/** @TODO instead, we should use 'tmp1' and 'tmp2' here... */
	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/* pass the values of everything back to the compiler */
	integrator_set_t(integ, (double)tt);
	integrator_set_y(integ, NV_DATA_S(yy));
	integrator_set_ydot(integ, NV_DATA_S(yp));

	/* perform bounds checking on all variables */
	if(slv_check_bounds(integ->system, 0, -1, NULL)){
		/* ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Variable(s) out of bounds"); */
		return 1;
	}

#ifdef DJEX_DEBUG
	varlist = slv_get_solvers_var_list(integ->system);

	/* print vars */
	for(i=0; i < integ->n_y; ++i){
		varname = var_make_name(integ->system, integ->y[i]);
		CONSOLE_DEBUG("%s = %f",varname,NV_Ith_S(yy,i));
		asc_assert(NV_Ith_S(yy,i) == var_value(integ->y[i]));
		ASC_FREE(varname);
	}

	/* print derivatives */
	for(i=0; i < integ->n_y; ++i){
		if(integ->ydot[i]){
			varname = var_make_name(integ->system, integ->ydot[i]);
			CONSOLE_DEBUG("%s = %f =%g",varname,NV_Ith_S(yp,i),var_value(integ->ydot[i]));
			ASC_FREE(varname);
		}else{
			varname = var_make_name(integ->system, integ->y[i]);
			CONSOLE_DEBUG("diff(%s) = %g",varname,NV_Ith_S(yp,i));
			ASC_FREE(varname);
		}
	}

	/* print step size */
	CONSOLE_DEBUG("<c_j> = %g",c_j);
#endif

	/* build up the dense jacobian matrix... */
	status = 0;
	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);

		if(status){
			relname = rel_make_name(integ->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			is_error = 1;
			break;
		}

		/* output what's going on here ... */
#ifdef DJEX_DEBUG
		relname = rel_make_name(integ->system, *relptr);
		fprintf(stderr,"%d: '%s': ",i,relname);
		for(j=0;j<count;++j){
			varname = var_make_name(integ->system, variables[j]);
			if(var_deriv(variables[j])){
				fprintf(stderr,"  '%s'=",varname);
				fprintf(stderr,"ydot[%d]",integrator_ida_diffindex(integ,variables[j]));
			}else{
				fprintf(stderr,"  '%s'=y[%d]",varname,var_sindex(variables[j]));
			}
			ASC_FREE(varname);
		}
		/* relname is freed further down */
		fprintf(stderr,"\n");
#endif

		/* insert values into the Jacobian row in appropriate spots (can assume Jac starts with zeros -- IDA manual) */
		for(j=0; j < count; ++j){
#ifdef DJEX_DEBUG
			varname = var_make_name(integ->system,variables[j]);
			fprintf(stderr,"d(%s)/d(%s) = %g",relname,varname,derivatives[j]);
			ASC_FREE(varname);
#endif
			if(!var_deriv(variables[j])){
#ifdef DJEX_DEBUG
				fprintf(stderr," --> J[%d,%d] += %g\n", i,j,derivatives[j]);
				asc_assert(var_sindex(variables[j]) >= 0);
				ASC_ASSERT_LT(var_sindex(variables[j]) , Neq);
#endif
				DENSE_ELEM(Jac,i,var_sindex(variables[j])) += derivatives[j];
			}else{
				DENSE_ELEM(Jac,i,integrator_ida_diffindex(integ,variables[j])) += derivatives[j] * c_j;
#ifdef DJEX_DEBUG
				fprintf(stderr," --> * c_j --> J[%d,%d] += %g\n", i,j,derivatives[j] * c_j);
#endif
			}
		}
	}

#ifdef DJEX_DEBUG
	ASC_FREE(relname);
	CONSOLE_DEBUG("PRINTING JAC");
	fprintf(stderr,"\t");
	for(j=0; j < integ->n_y; ++j){
		if(j)fprintf(stderr,"\t");
		varname = var_make_name(integ->system,integ->y[j]);
		fprintf(stderr,"%11s",varname);
		ASC_FREE(varname);
	}
	fprintf(stderr,"\n");
	for(i=0; i < enginedata->nrels; ++i){
		relname = rel_make_name(integ->system, enginedata->rellist[i]);
		fprintf(stderr,"%s\t",relname);
		ASC_FREE(relname);

		for(j=0; j < integ->n_y; ++j){
			if(j)fprintf(stderr,"\t");
			fprintf(stderr,"%11.2e",DENSE_ELEM(Jac,i,j));
		}
		fprintf(stderr,"\n");
	}
#endif

	/* test for NANs */
	if(!is_error){
		for(i=0;i< enginedata->nrels; ++i){
			for(j=0;j<integ->n_y;++j){
				if(isnan(DENSE_ELEM(Jac,i,j))){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"NAN detected in jacobian J[%d,%d]",i,j);
					is_error=1;
				}
			}
		}
#ifdef DJEX_DEBUG
		if(!is_error){
			CONSOLE_DEBUG("No NAN detected");
		}
#endif
	}

/*	if(integrator_ida_check_diffindex(integ)){
		is_error = 1;
	}*/

	ASC_FREE(variables);
	ASC_FREE(derivatives);

	if(is_error){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"There were derivative evaluation errors in the dense jacobian");
		return 1;
	}

#ifdef DJEX_DEBUG
	CONSOLE_DEBUG("DJEX RETURNING 0");
	/* ASC_PANIC("Quitting"); */
#endif
	return 0;
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
	@param jac_data pointer to our stuff (integ in this case, passed into IDA via IDASp*SetJacTimesVecFn.)
	@param tmp1 @see tmp2
	@param tmp2 (as well as tmp1) pointers to memory allocated for variables of type N_Vector for use here as temporary storage or work space.
	@return 0 on success
*/
int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
){
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	int i, j, is_error=0;
	struct rel_relation** relptr = 0;
	char *relname;
	int status;
	double Jv_i;

	struct var_variable **variables;
	double *derivatives;
	int count;
	struct var_variable **varlist;
#ifdef JEX_DEBUG

	CONSOLE_DEBUG("EVALUATING JACOBIAN...");
#endif

	integ = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(integ);
	varlist = slv_get_solvers_var_list(integ->system);

	/* pass the values of everything back to the compiler */
	integrator_set_t(integ, (double)tt);
	integrator_set_y(integ, NV_DATA_S(yy));
	integrator_set_ydot(integ, NV_DATA_S(yp));
	/* no real use for residuals (rr) here, I don't think? */

	/* allocate space for returns from relman_diff2: we *should* be able to use 'tmp1' and 'tmp2' here... */

	i = NV_LENGTH_S(yy) * 2;
#ifdef JEX_DEBUG
	CONSOLE_DEBUG("Allocating 'variables' with length %d",i);
#endif
	variables = ASC_NEW_ARRAY(struct var_variable*, i);
	derivatives = ASC_NEW_ARRAY(double, i);

	/* evaluate the derivatives... */
	/* J = dG_dy = dF_dy + alpha * dF_dyp */

#ifdef ASC_SIGNAL_TRAPS
	Asc_SignalHandlerPushDefault(SIGFPE);
	if (SETJMP(g_fpe_env)==0) {
#endif
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* get derivatives for this particular relation */
			status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("Got derivatives against %d matching variables, status = %d", count,status);
#endif

			if(status){
				relname = rel_make_name(integ->system, *relptr);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
				ASC_FREE(relname);
				is_error = 1;
				break;
			}

			/*
				Now we have the derivatives wrt each alg/diff variable in the
				present equation. variables[] points into the varlist. need
				a mapping from the varlist to the y and ydot lists.
			*/

			Jv_i = 0;
			for(j=0; j < count; ++j){
				/* CONSOLE_DEBUG("j = %d, variables[j] = %d, n_y = %ld", j, variables[j], integ->n_y);
				varname = var_make_name(integ->system, enginedata->varlist[variables[j]]);
				if(varname){
					CONSOLE_DEBUG("Variable %d '%s' derivative = %f", variables[j],varname,derivatives[j]);
					ASC_FREE(varname);
				}else{
					CONSOLE_DEBUG("Variable %d (UNKNOWN!): derivative = %f",variables[j],derivatives[j]);
				}
				*/

				/* we don't calculate derivatives wrt indep var */
				asc_assert(variables[j]>=0);
				if(variables[j] == integ->x) continue;
#ifdef JEX_DEBUG
				CONSOLE_DEBUG("j = %d: variables[j] = %d",j,var_sindex(variables[j]));
#endif
				if(var_deriv(variables[j])){
#define DIFFINDEX integrator_ida_diffindex(integ,variables[j])
#ifdef JEX_DEBUG
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dydot[%d] = %f, v[%d] = %f)\n", i
						, derivatives[j] * NV_Ith_S(v,DIFFINDEX)
						, i, DIFFINDEX, derivatives[j]
						, DIFFINDEX, NV_Ith_S(v,DIFFINDEX)
					);
#endif
					asc_assert(integ->ydot[DIFFINDEX]==variables[j]);
					Jv_i += derivatives[j] * NV_Ith_S(v,DIFFINDEX) * c_j;
#undef DIFFINDEX
				}else{
#define VARINDEX var_sindex(variables[j])
#ifdef JEX_DEBUG
					asc_assert(integ->y[VARINDEX]==variables[j]);
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dy[%d] = %f, v[%d] = %f)\n"
						, i
						, derivatives[j] * NV_Ith_S(v,VARINDEX)
						, i, VARINDEX, derivatives[j]
						, VARINDEX, NV_Ith_S(v,VARINDEX)
					);
#endif
					Jv_i += derivatives[j] * NV_Ith_S(v,VARINDEX);
#undef VARINDEX
				}
			}

			NV_Ith_S(Jv,i) = Jv_i;
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("rel = %p",*relptr);
			relname = rel_make_name(integ->system, *relptr);
			CONSOLE_DEBUG("'%s': Jv[%d] = %f", relname, i, NV_Ith_S(Jv,i));
			ASC_FREE(relname);
			return 1;
#endif
		}
#ifdef ASC_SIGNAL_TRAPS
	}else{
		relname = rel_make_name(integ->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}
	Asc_SignalHandlerPopDefault(SIGFPE);
#endif

	if(is_error){
		CONSOLE_DEBUG("SOME ERRORS FOUND IN EVALUATION");
		return 1;
	}
	return 0;
}

/* sparse jacobian evaluation for IDAASCEND sparse direct linear solver */
int integrator_ida_sjex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, mtx_matrix_t Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return -1;
}

/* root finding function */

int integrator_ida_rootfn(realtype tt, N_Vector yy, N_Vector yp, realtype *gout, void *g_data){
	IntegratorSystem *integ;
	IntegratorIdaData *enginedata;
	int i;
#ifdef ROOT_DEBUG
	char *relname;
#endif

	asc_assert(g_data!=NULL);
	integ = (IntegratorSystem *)g_data;
	enginedata = integrator_ida_enginedata(integ);

	/* pass the values of everything back to the compiler */
	integrator_set_t(integ, (double)tt);
	integrator_set_y(integ, NV_DATA_S(yy));
	integrator_set_ydot(integ, NV_DATA_S(yp));

	asc_assert(gout!=NULL);

#ifdef ROOT_DEBUG
	CONSOLE_DEBUG("t = %f",tt);
#endif

	/* evaluate the residuals for each of the boundaries */
	for(i=0; i < enginedata->nbnds; ++i){
		switch(bnd_kind(enginedata->bndlist[i])){
			case e_bnd_rel: /* real-valued boundary relation */
				gout[i] = bndman_real_eval(enginedata->bndlist[i]);
#ifdef ROOT_DEBUG
				relname = bnd_make_name(integ->system,enginedata->bndlist[i]);
				CONSOLE_DEBUG("gout[%d] = %f (boundary '%s')", i, gout[i], relname);
				ASC_FREE(relname);
#endif
				break;
			case e_bnd_logrel:
				if(bndman_log_eval(enginedata->bndlist[i])){
					CONSOLE_DEBUG("bnd[%d] = TRUE",i);
#ifdef ROOT_DEBUG
					relname = bnd_make_name(integ->system,enginedata->bndlist[i]);
					CONSOLE_DEBUG("gout[%d] = %f (boundary '%s')", i, gout[i], relname);
					ASC_FREE(relname);
#endif
					gout[i] = +1.0;
				}else{
					CONSOLE_DEBUG("bnd[%d] = FALSE",i);
					gout[i] = -1.0;
				}
				break;
			case e_bnd_undefined:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid boundary type e_bnd_undefined");
				return 1;
		}
	}

	return 0; /* no way to detect errors in bndman_*_eval at this stage */
}

