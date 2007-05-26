/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University
 
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
	DOPRI5 Runge-Kutta integrator
 
	Based on the implementation of LSODE integrator, but adapted to 
	an explicit one-step method.
*//*
	by John Pye, May 2007.
*/

#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <general/ospath.h>
#include <integrator/integrator.h>

#define INTEG_DOPRI5 5

IntegratorCreateFn integrator_dopri5_create;
IntegratorParamsDefaultFn integrator_dopri5_params_default;
IntegratorSolveFn integrator_dopri5_solve;
IntegratorFreeFn integrator_dopri5_free;
IntegratorWriteMatrixFn integrator_dopri5_write_matrix;

const IntegratorInternals integrator_lsode_internals;

const IntegratorInternals integrator_dopri5_internals =
	{
		integrator_dopri5_create
		,integrator_dopri5_params_default
		,integrator_analyse_ode /* note, this routine is back in integrator.c */
		,integrator_dopri5_solve
		,integrator_dopri5_write_matrix
		,NULL /* debugfn */
		,integrator_dopri5_free
		,INTEG_DOPRI5
		,"DOPRI5"
	};

extern ASC_EXPORT int dopri5_register(void)
{
	CONSOLE_DEBUG("DOPRI5");
	return 0;
}

typedef struct IntegratorDopri5DataStruct
{
	long n_eqns;                     /**< dimension of state vector */
	int *input_indices;              /**< vector of state vars indexes */
	int *output_indices;             /**< vector of derivative var indexes */
	struct var_variable **y_vars;    /**< NULL-terminated list of states vars */
	struct var_variable **ydot_vars; /**< NULL-terminated list of derivative vars*/
	struct rel_relation **rlist;     /**< NULL-terminated list of relevant rels
								                                      to be differentiated */

	char stop;                             /* stop requested? */
	int partitioned;                       /* partioned func evals or not */

	clock_t lastwrite;                     /* time of last call to the reporter 'write' function */
}
IntegratorDopri5Data;

/*------------------------------------------------------------------------------
	PARAMETERS
*/

enum ida_parameters{
	DOPRI5_PARAMS_
	,DOPRI5_PARAMS_RTOL
	,DOPRI5_PARAMS_ATOL
	,DOPRI5_PARAMS_TOLVECT

#if 0
	// more parameters for adding later...
	SolTrait *solout, /* function providing the numerical solution during integration */
	int iout,        /* switch for calling solout */

	double uround,   /* rounding unit */
	double safe,     /* safety factor */
	double fac1,     /* parameters for step size selection */
	double fac2,
	double beta,     /* for stabilized step size control */

	double hmax,     /* maximal step size */
	double h,        /* initial step size */
	long nmax,       /* maximal number of allowed steps */

	int meth,        /* switch for the choice of the coefficients */
	long nstiff,     /* test for stiffness */
#endif

	,DOPRI5_PARAMS_SIZE
};

/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the blsys->params structure.
 
	@return 0 on success
*/
int integrator_dopri5_params_default(IntegratorSystem *blsys){

	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_DOPRI5);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL) {
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, LSODE_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	} else {
		asc_assert(p->num_parms == LSODE_PARAMS_SIZE);
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(p,DOPRI5_PARAM_TOLVECT
				   ,(SlvParameterInitBool) {
					   {"tolvect"
						   ,"Use per-variable tolerances?",1
						   ,"If TRUE, values of 'ode_rtol' and 'ode_atol' are taken from your"
						   " model and used in the integration. If FALSE, scalar values are"
						   " used (see 'rtol' and 'atol') and shared by all variables."
					   }
					   , TRUE
				   }
				  );

	slv_param_real(p,DOPRI5_PARAM_ATOL
				   ,(SlvParameterInitReal) {
					   {"atol"
						   ,"Scalar absolute error tolerance",1
						   ,"Default value of the scalar absolute error tolerance (for cases"
						   " where not specified in oda_atol var property. See 'dopri5.h' for"
						   " details"
					   }
					   , 1e-7, 1e-15, 1e10
				   }
				  );

	slv_param_real(p,DOPRI5_PARAM_RTOL
				   ,(SlvParameterInitReal) {
					   {"rtol"
						   ,"Scalar relative error tolerance",1
						   ,"Default value of the scalar relative error tolerance (for cases"
						   " where not specified in oda_rtol var property. See 'dopri5.h' for"
						   " details"
					   }
					   , 1e-7, 1e-15, 1
				   }
				  );

/*

	slv_param_bool(p,DOPRI5_PARAMS_DENSEREPORTING
				   ,(SlvParameterInitBool) {
					   {"densereporting"
						   ,"Dense reporting?",1
						   ,"If TRUE, output will be made at every sub-timestep"
						   " during integration. If false, output is only made
						   " according to the samplelist."
					   }
					   , FALSE
				   }
				  );


	slv_param_char(p,LSODE_PARAM_METH
				   ,(SlvParameterInitChar) {
					   {"meth"
						   ,"Integration method",1
						   ,"AM=Adams-Moulton method (for non-stiff problems), BDF=Backwards"
						   " Difference Formular (for stiff problems). See 'Description and"
						   " Use of LSODE', section 3.1."
					   }
					   , "BDF"
				   }
				   , (char *[]) {"AM","BDF",NULL
				   }
				  );

	slv_param_int(p,LSODE_PARAM_MITER
				  ,(SlvParameterInitInt) {
					  {"miter"
						  ,"Corrector iteration technique", 1
						  ,"0=Functional iteration, 1=Modified Newton iteration with user-"
						  "supplied analytical Jacobian, 2=Modified Newton iteration with"
						  " internally-generated numerical Jacobian, 3=Modified Jacobi-Newton"
						  " iteration with internally generated numerical Jacobian. See "
						  " 'Description and Use of LSODE', section 3.1. Note that not all"
						  " methods described there are available via ASCEND."
					  }
					  , 1, 0, 3
				  }
				 );

	slv_param_int(p,LSODE_PARAM_MAXORD
				  ,(SlvParameterInitInt) {
					  {"maxord"
						  ,"Maximum method order", 1
						  ,"See 'Description and Use of LSODE', p 92 and p 8. Limits <=12 for BDF"
						  " and <=5 for AM. Higher values are reduced automatically. See notes on"
						  " page 92 regarding oscillatory systems."
					  }
					  , 12, 1, 12
				  }
				 );

	slv_param_bool(p,LSODE_PARAM_TIMING
				   ,(SlvParameterInitBool) {
					   {"timing"
						   ,"Output timing statistics?",1
						   ,"If TRUE, additional timing statistics will be output to the"
						   " console during integration."
					   }
					   , TRUE
				   }
				  );
*/

	asc_assert(p->num_parms == DOPRI5_PARAMS_SIZE);
	return 0;
}


/**
	Allocates, fills, and returns the rtol vector based on LSODE

	State variables missing child ode_rtol will be defaulted to RTOL

	@param is_r TRUE if we want RTOL, FALSE if we want ATOL.
*/
static double *dopri5_get_artol(IntegratorSystem *blsys, int is_r, int tolvect) {

  struct Instance *toli;
  double tolval, *tol;
  int i,len;
  symchar *tolname;

  len = blsys->n_y;

  if(!tolvect){

	// single tol for all vars
	tolval = SLV_PARAM_REAL(&(blsys->params),DOPRI5_PARAM_RTOL);
	CONSOLE_DEBUG("Using RTOL = %f for all vars", tolval);

	tol = ASC_NEW(double);
	if(tol == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
		return tol;
	}

	*tol = tolval;
	return tolval;

  }else{
    tol = ASC_NEW_ARRAY(double, blsys->n_y+1);
	if (tol == NULL) {
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
	  return tol;
	}

	tolval = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_RTOL)

    InitTolNames(); // from where?
	if(is_r)tolname = STATERTOL;
	else tolname = STATEATOL;

    for(i=0; i<len; i++){
      toli = ChildByChar(var_instance(blsys->y[i]),tolname);
      if(toli == NULL || !AtomAssigned(toli)){
        tol[i] = tolval;
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Assuming value %3g"
        	"for '%s' child undefined for state variable %ld."
        	,tol[i], SCP(tolname), blsys->y_id[i]
        );
      }else{
        tol[i] = RealAtomValue(toli);
      }
    }
  }
  tol[len] = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_RTOL);
  return tol;
}

/*------------------------------------------------------------------------------
  FUNCTION EVALUATION
*/

FcnEqDiff integrator_dopri5_fex;

/*------------------------------------------------------------------------------
  REPORTING
*/

SolTrait integrator_dopri5_reporter;

/*------------------------------------------------------------------------------
  SOLVE
*/
int integrator_dopri5_solve(IntegratorSystem *blsys
							, unsigned long start_index, unsigned long finish_index
){

	slv_status_t status;
	slv_parameters_t params;
	IntegratorLsodeData *d;

	double x[2];
	double xend,xprev;
	unsigned long nsamples, neq;
	long nobs;
	int  itol, itask, mf, lrw, liw;
	unsigned long index;
	int istate, iopt;
	double * rwork;
	int * iwork;
	double *y, *atoler, *rtoler, *obs, *dydx;
	int my_neq;
	int reporterstatus;
	const char *method; /* Table 3.1 in D&UoLSODE */
	int miter; /* Table 3.2 in D&UoLSODE */
	int maxord; /* page 92 in D&UoLSODE */

	d = (IntegratorLsodeData *)(blsys->enginedata);

	/* the numer of equations must be equal to blsys->n_y, the number of states */
	d->n_eqns = blsys->n_y;
	assert(d->n_eqns>0);

	d->input_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);
	d->output_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);

	d->y_vars = ASC_NEW_ARRAY(struct var_variable *,d->n_eqns+1);
	d->ydot_vars = ASC_NEW_ARRAY(struct var_variable *, d->n_eqns+1);

	/* set up the NLA solver here */

	/* here we assume integrators.c is in charge of dynamic loading */

	/* set up parameteers for sending to DOPRI5 */

#if 0
	method = SLV_PARAM_CHAR(&(blsys->params),LSODE_PARAM_METH);
	miter = SLV_PARAM_INT(&(blsys->params),LSODE_PARAM_MITER);
	maxord = SLV_PARAM_INT(&(blsys->params),LSODE_PARAM_MAXORD);
	if(miter < 0 || miter > 3) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unacceptable value '%d' of parameter 'miter'",miter);
		return 5;
	}
	if(strcmp(method,"BDF")==0) {
		CONSOLE_DEBUG("method = BDF");
		mf = 20 + miter;
		if(maxord > 5) {
			maxord = 5;
			CONSOLE_DEBUG("MAXORD reduced to 5 for BDF");
		}
	} else if(strcmp(method,"AM")==0) {
		CONSOLE_DEBUG("method = AM");
		if(maxord > 12) {
			maxord = 12;
			CONSOLE_DEBUG("MAXORD reduced to 12 for AM");
		}
		mf = 10 + miter;
	} else {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unacceptable value '%d' of parameter 'meth'",method);
		return 5;
	}

	CONSOLE_DEBUG("MF = %d",mf);
#endif

	nsamples = integrator_getnsamples(blsys);
	if (nsamples <2) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration will not be performed. The system has no end sample time defined.");
		d->status = lsode_nok;
		return 3;
	}
	neq = blsys->n_y;
	nobs = blsys->n_obs;

#if 0
	unsigned nrdens, /* number of components for which dense outpout is required */
	unsigned* icont, /* indexes of components for which dense output is required, >= nrdens */
	unsigned licont  /* declared length of icon */
#endif

	int iout = 1; /* SLV_PARAM_BOOL(&(blsys->params),DOPRI5_PARAM_DENSEREPORTING) */

	int tolvect = SLV_PARAM_BOOL(&(blsys->params),DOPRI5_PARAM_TOLVECT);

	/* samplelist_debug(blsys->samples); */

#if 0
	/* x[0] = integrator_get_t(blsys); */
	x[0] = integrator_getsample(blsys, 0);
	x[1] = x[0]-1; /* make sure we don't start with wierd x[1] */

	/* RWORK memory requirements: see D&UoLSODE p 82 */
	switch(mf) {
	case 10:
	case 20:
		lrw = 20 + neq * (maxord + 1) + 3 * neq;
		break;
	case 11:
	case 12:
	case 21:
	case 22:
		lrw = 22 + neq * (maxord + 1) + 3 * neq + neq*neq;
		break;
	case 13:
	case 23:
		lrw = 22 + neq * (maxord + 1) + 4 * neq;
		break;
	default:
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unknown size requirements for this value of 'mf'");
		return 4;
	}

	rwork = ASC_NEW_ARRAY_CLEAR(double, lrw+1);
	liw = 20 + neq;

#endif

	iwork = ASC_NEW_ARRAY_CLEAR(int, liw+1);
	y = integrator_get_y(blsys, NULL);
	
	rtoler = lsode_get_artol(blsys,1,tolvect);
	atoler = lsode_get_artol(blsys,0,tolvect);

	obs = integrator_get_observations(blsys, NULL);

#if 0
	dydx = ASC_NEW_ARRAY_CLEAR(double, neq+1);
	if(!y || !obs || !atoler || !rtoler || !rwork || !iwork || !dydx) {
		lsode_free_mem(y,rtoler,atoler,rwork,iwork,obs,dydx);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory for lsode.");
		d->status = lsode_nok;
		return 4;
	}
#endif

	/*
		Prepare args and call lsode.
	*/
	itol = 4;
	itask = 1;
	istate = 1;
	iopt = 1;
	rwork[4] = integrator_get_stepzero(blsys);
	rwork[5] = integrator_get_maxstep(blsys);
	rwork[6] = integrator_get_minstep(blsys);
	iwork[5] = integrator_get_maxsubsteps(blsys);
	iwork[4] = maxord;
	CONSOLE_DEBUG("MAXORD = %d",maxord);

	if(x[0] > integrator_getsample(blsys, 2)) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid initialisation time: exceeds second timestep value");
		return 5;
	}

	/* put the values from derivative system into the record */
	integrator_setsample(blsys, start_index, x[0]);

	integrator_output_init(blsys);

	/* -- store the initial values of all the stuff */
	integrator_output_write(blsys);
	integrator_output_write_obs(blsys);

	my_neq = (int)neq;

	/*
	First time entering lsode, x is input. After that,
	lsode uses x as output (y output is y(x)). To drive
	the loop ahead in time, all we need to do is keep upping
	xend.
	*/

	blsys->currentstep = 0;
	for(index = start_index; index < finish_index; index++, 	blsys->currentstep++) {
		xend = integrator_getsample(blsys, index+1);
		xprev = x[0];
		asc_assert(xend > xprev);
		/* CONSOLE_DEBUG("LSODE call #%lu: x = [%f,%f]", index,xprev,xend); */

# ifdef ASC_SIGNAL_TRAPS

		Asc_SignalHandlerPushDefault(SIGFPE);
		Asc_SignalHandlerPushDefault(SIGINT);

		if(SETJMP(g_fpe_env)==0) {
# endif /* ASC_SIGNAL_TRAPS */

			/* CONSOLE_DEBUG("Calling LSODE with end-time = %f",xend); */
			/*
			switch(mf){
			case 10:
			CONSOLE_DEBUG("Non-stiff (Adams) method; no Jacobian will be used"); break;
			case 21:
			CONSOLE_DEBUG("Stiff (BDF) method, user-supplied full Jacobian"); break;
			case 22:
			CONSOLE_DEBUG("Stiff (BDF) method, internally generated full Jacobian"); break;
			case 24:
			CONSOLE_DEBUG("Stiff (BDF) method, user-supplied banded jacobian"); break;
			case 25:
			CONSOLE_DEBUG("Stiff (BDF) method, internally generated banded jacobian"); break;
			default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid method id %d for LSODE",mf);
			return 0; * failure *
			}
			*/

			d->lastwrite = clock();

			// tolvect = 0 : scalars
			LSODE(&(LSODE_FEX), &my_neq, y, x, &xend,
				  &itol, rtoler, atoler, &itask, &istate,
				  &iopt ,rwork, &lrw, iwork, &liw, &(LSODE_JEX), &mf);

		    res = dopri5 (my_neq, &integrator_dopri5_fex, x, y, xend, rtoler, atoler, itoler, integrator_dopri5_reporter, iout,
				stdout, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, NULL, 0);

# ifdef ASC_SIGNAL_TRAPS
		} else {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in LSODE call.");
			//dopri5_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
			return 6;
		}
		Asc_SignalHandlerPopDefault(SIGFPE);
		Asc_SignalHandlerPopDefault(SIGINT);

# endif

		/* CONSOLE_DEBUG("AFTER %lu LSODE CALL\n", index); */
		/* this check is better done in fex,jex, but lsode takes no status */
		/*    if (Solv_C_CheckHalt()) {
		      if (istate >= 0) {
		        istate=-7;
		      }
		    }
		*/
		if(d->stop) {
			istate=-8;
		}

		if (istate < 0 ) {
			/* some kind of error occurred... */
			ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
			//lsode_write_istate(istate);
			FPRINTF(ASCERR, "\nFurthest point reached was t = %g.\n",x[0]);
			error_reporter_end_flush();

			//dopri5_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
			integrator_output_close(blsys);
			return 7;
		}

		if (d->status==lsode_nok) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to an error in derivative computations.");
			//lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
			//d->status = lsode_ok;		/* clean up before we go */
			//d->lastcall = lsode_none;
			integrator_output_close(blsys);
			return 8;
		}

		integrator_setsample(blsys, index+1, x[0]);
		/* record when lsode actually came back */
		integrator_set_t(blsys, x[0]);
		integrator_set_y(blsys, y);
		/* put x,y in d in case lsode got x,y by interpolation, as it does  */

		reporterstatus = integrator_output_write(blsys);

		if(reporterstatus==0) {
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration cancelled");
			//lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
			//d->status = lsode_ok;
			//d->lastcall = lsode_none;
			integrator_output_close(blsys);
			return 9;
		}

		if (nobs > 0) {
# ifdef ASC_SIGNAL_TRAPS
			if (SETJMP(g_fpe_env)==0) {
# endif /* ASC_SIGNAL_TRAPS */

				/* solve for obs since d isn't necessarily already
				   computed there though lsode's x and y may be.
				   Note that since lsode usually steps beyond xend
				   x1 usually wouldn't be x0 precisely if the x1/x0
				   scheme worked, which it doesn't anyway. */

				//LSODEDATA_SET(blsys);
				//LSODE_FEX(&my_neq, x, y, dydx);
				//LSODEDATA_RELEASE();

				/* calculate observations, if any, at returned x and y. */
				obs = integrator_get_observations(blsys, obs);

				integrator_output_write_obs(blsys);

# ifdef ASC_SIGNAL_TRAPS
			}else{
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in LSODE FEX call.");
				lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
				d->status = lsode_ok;               /* clean up before we go */
				d->lastcall = lsode_none;
				integrator_output_close(blsys);
				return 10;
			}
# endif /* ASC_SIGNAL_TRAPS */
		}
		/* CONSOLE_DEBUG("Integration completed from %3g to %3g.",xprev,x[0]); */
	}

	integrator_output_close(blsys);

	CONSOLE_DEBUG("--- DOPRI5 done ---");
	return 0; /* success */
}

#endif

