/*
AUTHOR : Shrikanth Ranganadham
Based on asc_lsode and asc_dopri5
*/
#include <malloc.h>
#include <ascend/general/platform.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/utilities/error.h>
#include <ascend/general/ospath.h>
#include <ascend/integrator/integrator.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/solver/solver.h>
#include "radau.h"
#define INTEG_RADAU5 6 	// Guess this is solver id for ascend purpose 
			// Since dopri5 had 5 i used 6 here

/**
//DEBUGGING STATEMENTS

#define memory_debug

*/
/**
END DEBUGGING
*/
/* REGISTERING THE SOLVER */

IntegratorCreateFn integrator_radau5_create;
IntegratorParamsDefaultFn integrator_radau5_params_default;
IntegratorSolveFn integrator_radau5_solve;
IntegratorFreeFn integrator_radau5_free;
IntegratorWriteMatrixFn integrator_radau5_write_matrix;

const IntegratorInternals integrator_radau5_internals = {
	integrator_radau5_create
	,integrator_radau5_params_default
	,integrator_analyse_ode /* note, this routine is back in integrator.c */
	,integrator_radau5_solve
	,NULL /*integrator_radau5_write_matrix not implemented */
	,NULL /* debugfn */
	,integrator_radau5_free
	,INTEG_RADAU5
	,"RADAU5"
};

extern ASC_EXPORT int radau5_register(void){
	CONSOLE_DEBUG("Registering RADAU5...");
	return integrator_register(&integrator_radau5_internals);
}

/*END REGISTERATION*/
/**
FORTRAN REFERENCE 
     IDID        REPORTS ON SUCCESSFULNESS UPON RETURN:
                   IDID= 1  COMPUTATION SUCCESSFUL,
                   IDID= 2  COMPUT. SUCCESSFUL (INTERRUPTED BY SOLOUT)
                   IDID=-1  INPUT IS NOT CONSISTENT,
                   IDID=-2  LARGER NMAX IS NEEDED,
                   IDID=-3  STEP SIZE BECOMES TOO SMALL,
                   IDID=-4  MATRIX IS REPEATEDLY SINGULAR
*/

enum radau5_status{
	RADAU5_SUCCESS=1
	,RADAU5_INRPT=2
	,RADAU5_BADINPUT=-1
	,RADAU5_ITERLIMIT=-2
	,RADAU5_STEPSMALL=-3
	,RADAU5_SINGULAR=-4
};

typedef struct IntegratorRadau5DataStruct
{
	long n_eqns;                     /**< dimension of state vector */
	int *input_indices;              /**< vector of state vars indexes */
	int *output_indices;             /**< vector of derivative var indexes */
	struct var_variable **y_vars;    /**< NULL-terminated list of states vars */
	struct var_variable **ydot_vars; /**< NULL-terminated list of derivative vars*/
	struct rel_relation **rlist;     /**< NULL-terminated list of relevant rels 
					to be differentiated */
	long currentsample;
	char stop;			/* stop requested? */
	int partitioned;            	/* partioned func evals or not */
	double *yinter;			/* interpolated y values */

	clock_t lastwrite;		 /* time of last call to the reporter 'write' function */
}
IntegratorRadau5Data;

/** DONT KNOW IF THIS STUFF IS NEEDED*/
static IntegratorSystem *l_blsys = 0;
/** Macro to declare a local var and fetch the 'enginedata' stuff into it from l_radau5_blsys. */
#define RADAU5DATA_GET(N) \
	IntegratorRadau5Data *N; \
	asc_assert(l_blsys!=NULL); \
	N = (IntegratorRadau5Data *)l_blsys->enginedata; \
	asc_assert(N!=NULL)

/** Macro to set the global l_radau5_blsys to the currently blsys ptr. */
#define RADAU5DATA_SET(N) \
	asc_assert(l_blsys==NULL); \
	asc_assert(N!=NULL); \
	l_blsys = N

#define RADAU5DATA_RELEASE() \
	asc_assert(l_blsys!=NULL); \
	l_blsys = NULL;

/** END DONT KNOW STUFF */
/**
ALLOCATE AND FREE MEMORY 
*/
void integrator_radau5_create(struct IntegratorSystemStruct *blsys){
	IntegratorRadau5Data *d;
	d = ASC_NEW_CLEAR(IntegratorRadau5Data);
	d->n_eqns=0;
	d->input_indices=NULL;
	d->output_indices=NULL;
	d->y_vars=NULL;
	d->ydot_vars=NULL;
	d->rlist=NULL;
	blsys->enginedata=(void*)d;
	integrator_radau5_params_default(blsys);
	CONSOLE_DEBUG("CREATED RADAU5");
}

/**
END MEMORY ALLOCATION
*/

/**
Cleanup the data struct that belongs to RADAU5
*/
void integrator_radau5_free(void *enginedata){
	IntegratorRadau5Data d;
	d = *((IntegratorRadau5Data *)enginedata);

	if(d.input_indices)ASC_FREE(d.input_indices);
	d.input_indices = NULL;

	if(d.output_indices)ASC_FREE(d.output_indices);
	d.output_indices = NULL;

	if(d.y_vars)ASC_FREE(d.y_vars);
	d.y_vars = NULL;

	if(d.ydot_vars)ASC_FREE(d.ydot_vars);
	d.ydot_vars = NULL;

	if(d.rlist)ASC_FREE(d.rlist);
	d.rlist =  NULL;

	if(d.yinter)ASC_FREE(d.yinter);
	d.yinter = NULL;

	d.n_eqns = 0L;
}
/**
PARAMETERS REQUIRED FOR RADAU5 CODE
REF: FORTRAN code
     RTOL,ATOL   RELATIVE AND ABSOLUTE ERROR TOLERANCES. THEY
                 CAN BE BOTH SCALARS OR ELSE BOTH VECTORS OF LENGTH N.
     ITOL        SWITCH FOR RTOL AND ATOL:
                   ITOL=0: BOTH RTOL AND ATOL ARE SCALARS.
                     THE CODE KEEPS, ROUGHLY, THE LOCAL ERROR OF
                     Y(I) BELOW RTOL*ABS(Y(I))+ATOL
                   ITOL=1: BOTH RTOL AND ATOL ARE VECTORS.
                     THE CODE KEEPS THE LOCAL ERROR OF Y(I) BELOW
                     RTOL(I)*ABS(Y(I))+ATOL(I).
     IJAC        SWITCH FOR THE COMPUTATION OF THE JACOBIAN:
                    IJAC=0: JACOBIAN IS COMPUTED INTERNALLY BY FINITE
                       DIFFERENCES, SUBROUTINE "JAC" IS NEVER CALLED.
                    IJAC=1: JACOBIAN IS SUPPLIED BY SUBROUTINE JAC.

     MLJAC       SWITCH FOR THE BANDED STRUCTURE OF THE JACOBIAN:
                    MLJAC=N: JACOBIAN IS A FULL MATRIX. THE LINEAR
                       ALGEBRA IS DONE BY FULL-MATRIX GAUSS-ELIMINATION.
                    0<=MLJAC<N: MLJAC IS THE LOWER BANDWITH OF JACOBIAN 
                       MATRIX (>= NUMBER OF NON-ZERO DIAGONALS BELOW
                       THE MAIN DIAGONAL).

     MUJAC       UPPER BANDWITH OF JACOBIAN  MATRIX (>= NUMBER OF NON-
                 ZERO DIAGONALS ABOVE THE MAIN DIAGONAL).
                 NEED NOT BE DEFINED IF MLJAC=N.

C     IMAS       GIVES INFORMATION ON THE MASS-MATRIX:
C                    IMAS=0: M IS SUPPOSED TO BE THE IDENTITY
C                       MATRIX, MAS IS NEVER CALLED.
C                    IMAS=1: MASS-MATRIX  IS SUPPLIED.
C
C     MLMAS       SWITCH FOR THE BANDED STRUCTURE OF THE MASS-MATRIX:
C                    MLMAS=N: THE FULL MATRIX CASE. THE LINEAR
C                       ALGEBRA IS DONE BY FULL-MATRIX GAUSS-ELIMINATION.
C                    0<=MLMAS<N: MLMAS IS THE LOWER BANDWITH OF THE
C                       MATRIX (>= NUMBER OF NON-ZERO DIAGONALS BELOW
C                       THE MAIN DIAGONAL).
C                 MLMAS IS SUPPOSED TO BE .LE. MLJAC.
C
C     MUMAS       UPPER BANDWITH OF MASS-MATRIX (>= NUMBER OF NON-
C                 ZERO DIAGONALS ABOVE THE MAIN DIAGONAL).
C                 NEED NOT BE DEFINED IF MLMAS=N.
C                 MUMAS IS SUPPOSED TO BE .LE. MUJAC.

*/


enum radau5_parameters{
	RADAU5_PARAM_RTOL
	,RADAU5_PARAM_ATOL
	,RADAU5_PARAM_ITOL
	,RADAU5_PARAM_IJAC
	,RADAU5_PARAM_IMAS
	,RADAU5_PARAMS_SIZE // DONT KNOW USE OF THIS
		/*^^^  (this automatically takes on the next number in the sequence, hence, by declaring it, we automatically calculates the required size of the parameters array) -- JP */
};


/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the blsys->params structure.

	@return 0 on success
*/
int integrator_radau5_params_default(IntegratorSystem *blsys){

	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_RADAU5);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL){
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, RADAU5_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	}else{
		asc_assert(p->num_parms == RADAU5_PARAMS_SIZE);
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;


	slv_param_real(p,RADAU5_PARAM_ATOL
			,(SlvParameterInitReal){{"atol"
			,"Scalar absolute error tolerance",1
			,"Default value of the scalar absolute error tolerance (for cases"
			" where not specified in oda_atol var property. See 'radau5.f' for"
			" details"
		}, 1e-6, 1e-15, 1e10 }
	);

	slv_param_real(p,RADAU5_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Default value of the scalar relative error tolerance (for cases"
			" where not specified in oda_rtol var property. See 'radau5.f' for"
			" details"
		}, 1e-6, 1e-15, 1 }
	);


	slv_param_int(p,RADAU5_PARAM_ITOL
			,(SlvParameterInitInt){{"itol"
			,"Switch for rtol and atol",1
			,"ITOL=0: BOTH RTOL AND ATOL ARE SCALARS."
                     	"THE CODE KEEPS, ROUGHLY, THE LOCAL ERROR OF "
                     	" Y(I) BELOW RTOL*ABS(Y(I))+ATOL"
		},0,1,0}
	);


	slv_param_int(p,RADAU5_PARAM_IJAC
			,(SlvParameterInitInt){{"ijac"
			,"GIVES INFORMATION ON THE MASS-MATRIX",1
			,"IJAC=0: JACOBIAN IS NOT SUPPLIED"
			"MATRIX, JACOBIAN IS COMPUTED INTERNALLY"
			"IJAC=1: JACOBIAN IS SUPPLIED."
			"See 'radau5.f' for details"
		},0,1,0}
	);

	slv_param_int(p,RADAU5_PARAM_IMAS
			,(SlvParameterInitInt){{"imas"
			,"GIVES INFORMATION ON THE MASS-MATRIX",1
			,"IMAS=0: M IS SUPPOSED TO BE THE IDENTITY"
			"MATRIX, MAS IS NEVER CALLED"
			"IMAS=1: MASS-MATRIX  IS SUPPLIED."
			"See 'radau5.f' for details"
		},0,1,0}
	);

// SOME MORE WORK NEEDS TO BE DONE HERE >>> ADD MORE OPTIONS 

	asc_assert(p->num_parms == RADAU5_PARAMS_SIZE);
	return 0;
}


/** DEFINING FUNCTIONS FOR SOLVER USAGE */
//prototypes
static void integrator_radau5_fex(int*,double*,double*,double*,double*,int*);
static void integrator_radau5_jex(int*,double*,double*,double*,int*,double*,double*);
static void integrator_radau5_mex(int*,double*,int*,int*,int*);
static void integrator_radau5_solout(int*,double*,double*,double*,double*,int*,int*,double*,int*,int*);
// FUNCTION
//FCN(int*,double*,double*,double*,double*,int*)
static void integrator_radau5_fex(
		int *n_eq, double *t, double *y, double *ydot,
		double *rpar, int* ipar)
{
	slv_status_t status;
	RADAU5DATA_GET(radau5data);
	int i;
	unsigned long res;

	//CONSOLE_DEBUG("Calling for a function evaluation");
	//CONSOLE_DEBUG("t = %e",*t);
	//CONSOLE_DEBUG("t = %p",blsys);
	/* pass the time and the unknowns back to the System */
	integrator_set_t (l_blsys, *t);
	//CONSOLE_DEBUG("%e",*t);
	integrator_set_y (l_blsys, y);

	//CONSOLE_DEBUG("t = %p",t);

	asc_assert(l_blsys->system);

	slv_resolve(l_blsys->system);

	if((res = slv_solve(l_blsys->system))){
		CONSOLE_DEBUG("solver returns error %ld",res);
	}

	slv_get_status(l_blsys->system, &status);


	if(slv_check_bounds(l_blsys->system,0,-1,"")){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Variables went outside boundaries...");
		// TODO relay that system has gone out of bounds
	}

	/* pass the NLA solver status to the integrator */
	res = integrator_checkstatus(status);

	integrator_output_write(l_blsys);

  	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve for derivatives (%d)",res);
#if 1
		ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
		FPRINTF(ASCERR,"Unable to compute the vector of derivatives with the following values for the state variables:\n");
		for (i = 0; i< *n_eq; i++) {
			FPRINTF(ASCERR,"y[%4d] = %g\n",i, y[i]);
		}
		error_reporter_end_flush();
#endif
#ifdef ASC_SIGNAL_TRAPS
		raise(SIGINT);
#endif
	}else{
		/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"lsodedata->status = %d",lsodedata->status); */
	}

	integrator_get_ydot(l_blsys, ydot);
	//CONSOLE_DEBUG("Check");

#ifdef RADAU5_DEBUG
	CONSOLE_DEBUG("y[0]=%e,y[1]=%e --> ydot[0]=%e,ydot[1]=%e",y[0],y[1],ydot[0],ydot[1]);
#endif
	//CONSOLE_DEBUG("ydot[0] = %f",ydot[0]);
	//CONSOLE_DEBUG("DONE, OK");
}
// JACOBIAN .... 
/**
Dummy Jacobian ... more work needs to be done here 
compatible with default settings
*/
static void integrator_radau5_jex(int *n, double *x, double *y, double *dfy,
		   int *ldfy, double *rpar, double *ipar){
	/* TODO implement this */
	/*abort();*/
	CONSOLE_DEBUG("Dummy Jacobian");
}

// MASS FUNCTION
/**
Dummy Mass function ... more work needs to be done
compatible with default settings
*/
static void integrator_radau5_mex(int *n,double *am, int *lmas,int *rpar, int *ipar){
	//CONSOLE_DEBUG("Dummy mass function");
	/* TODO implement this */
	/*abort();*/

}
static void integrator_radau5_solout(int *nr,double *xold,double *x,
			double *y,
			double *cont,int *lrc,
			int *n,
			double *rpar,int *ipar,
			int *irtrn)
{	//CONSOLE_DEBUG("solout initiated");
	
	double t = *x;
	double ts;
	RADAU5DATA_GET(d);

	ts = integrator_getsample(l_blsys,d->currentsample);

	if(t>ts){
		//CONSOLE_DEBUG("t=%f > ts=%f (currentsample = %ld",t,ts,d->currentsample);
		integrator_output_write_obs(l_blsys);
		while(t>ts){
			d->currentsample++;
			l_blsys->currentstep++;
			ts = integrator_getsample(l_blsys,d->currentsample);
		}
	}

	//CONSOLE_DEBUG("t = %f, y[0] = %f",t,y[0]);
	integrator_output_write(l_blsys);

}
/** END DEFINING*/


/**
  STATS
*/

/**
	Several functions provide access to different values :

	nstepRead   Number of used steps.
	naccptRead  Number of accepted steps.
	nrejctRead  Number of rejected steps.
	nfcnRead    Number of function calls.
*/
typedef struct IntegratorRadau5StatsStruct{
	long nfcn;
	long nstep;
	long naccpt;
	long nrejct;
	double h;
	double x;
} IntegratorRadau5Stats;


/**	
	SOLVE
	(work in progress)
*/

#define RADAU5_FREE CONSOLE_DEBUG("FREE RADAU5")

int integrator_radau5_solve(IntegratorSystem *blsys,
unsigned long start_index, unsigned long finish_index
)
{
	IntegratorRadau5Data *d;
	slv_status_t status;
	
	unsigned long nsamples, neq;
	long nobs;
	int my_neq;

	d = (IntegratorRadau5Data *)(blsys->enginedata);

	/* the numer of equations must be equal to blsys->n_y, the number of states */
	d->n_eqns = blsys->n_y;
	assert(d->n_eqns>0);

	d->input_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);
	d->output_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);

	d->y_vars = ASC_NEW_ARRAY(struct var_variable *,d->n_eqns+1);
	d->ydot_vars = ASC_NEW_ARRAY(struct var_variable *, d->n_eqns+1);

	d->yinter = ASC_NEW_ARRAY(double,d->n_eqns);

	/** 
	set up the NLA solver here
	 */
	// THIS STUFF IS NOT CLEAR
	if(strcmp(slv_solver_name(slv_get_selected_solver(blsys->system)),"QRSlv") != 0)
	{
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"QRSlv must be selected before integration.");
		return 1;
	}

	CONSOLE_DEBUG("Solver selected is '%s'",slv_solver_name(slv_get_selected_solver(blsys->system)));

	slv_get_status(blsys->system, &status);

	if(status.struct_singular)
	{
		ERROR_REPORTER_HERE(ASC_USER_WARNING
			,"The system (according to QRSlv) is structurally singular."
			" The ODE system may also be singular, but not necessarily."
		);
	}

	/* here we assume integrators.c is in charge of dynamic loading */

	/* set up parameters for sending to RADAU5 */

	nsamples = integrator_getnsamples(blsys);
	if (nsamples <2)
	{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration will not be performed. The system has no end sample time defined.");
		//d->status = Radau5_nok;
		return 3;
	}
	neq = blsys->n_y;
	nobs = blsys->n_obs;
	my_neq = (int)neq;

/**
	SOME PARAMETERS
*/
	int ns = 3 ;
	int lwork;
	//lwork = (ns+1)*my_neq*my_neq + (1*ns+3)*my_neq + 20; //full jacobian setting 
	int liwork;
	liwork = (2+(ns-1)/2)*my_neq + 20; // full jacobian setting
	double *work;
	int *iwork;
	lwork = my_neq*(4*my_neq+12)+ 20;
	work= malloc(lwork * sizeof(double) );
	iwork= malloc(liwork * sizeof(int) );
	//double work[lwork];
	//int iwork[liwork];
	int i,idid;
	double x,xend;
	double h,hmax;
	double rpar=0.0;
	int mljac = my_neq;
	int mujac=0;
	int mlmas=0;
	int mumas;
	int ipar=0;
	int iout=1;
	double *y, atol, rtol, *obs;

# ifdef memory_debug
	CONSOLE_DEBUG("lwork = %d,liwork= %d,equations = %d",lwork,liwork,my_neq);
# endif

	for(i=0; i<20; ++i)
	{
		work[i]=0.0;
	}
	for(i=0; i<20; ++i)
	{
		iwork[i]=0;
	}
	enum radau5_status res;

	atol = SLV_PARAM_REAL(&(blsys->params),RADAU5_PARAM_ATOL);
	rtol = SLV_PARAM_REAL(&(blsys->params),RADAU5_PARAM_RTOL);	
	int ijac = SLV_PARAM_INT(&(blsys->params),RADAU5_PARAM_IJAC);
	int imas = SLV_PARAM_INT(&(blsys->params),RADAU5_PARAM_IMAS);
	int itol = SLV_PARAM_INT(&(blsys->params),RADAU5_PARAM_ITOL);	
	//CONSOLE_DEBUG("atol=%e,rtol=%e,ijac=%d,imas=%d,itol=%d",atol,rtol,ijac,imas,itol);
	/* samplelist_debug(blsys->samples); */

	x = integrator_getsample(blsys, 0);
	d->currentsample = 1;

	y = integrator_get_y(blsys, NULL);
	obs = integrator_get_observations(blsys, NULL);

	// TODO check memory allocations

	h = integrator_get_stepzero(blsys);
	hmax = integrator_get_maxstep(blsys);
	work[6]=hmax;
	//CONSOLE_DEBUG("init step = %f, max step = %f", h, hmax);

	/* rwork[6] = integrator_get_minstep(blsys); */ /* ignored */
	iwork[1] = integrator_get_maxsubsteps(blsys); // NMAX PARAMETER 

	if(x > integrator_getsample(blsys, 2))
	{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid initialisation time: exceeds second timestep value");
		return 5;
	}

	/* put the values from derivative system into the record */
	//integrator_setsample(blsys, start_index, x);

	///integrator_output_init(blsys);

	/* -- store the initial values of all the stuff */
	//integrator_output_write(blsys);

	integrator_output_write_obs(blsys);

	blsys->currentstep = 0;

	xend = integrator_getsample(blsys, finish_index);

	# ifdef ASC_SIGNAL_TRAPS

	Asc_SignalHandlerPushDefault(SIGFPE);
	Asc_SignalHandlerPushDefault(SIGINT);

	if(SETJMP(g_fpe_env)==0)
	{
	# endif /* ASC_SIGNAL_TRAPS */
	d->lastwrite = clock();
#ifdef PARAMETER_DEBUG
	CONSOLE_DEBUG("printing everythin %d %e %e %e %d %d %d %d %d %e %d %d",my_neq,x,xend,h,mljac,mujac,mlmas,mumas,iout,rpar,ipar,idid);
#endif
	CONSOLE_DEBUG("solving started");
	RADAU5DATA_SET(blsys);
	radau5_( &my_neq, &integrator_radau5_fex,	// Function
		&x, y, &xend, &h, &rtol, &atol, &itol,
		&integrator_radau5_jex,	// jacobian
		 &ijac, &mljac, &mujac,	
		&integrator_radau5_mex,	// mass matrix
		 &imas, &mlmas, &mumas,
		&integrator_radau5_solout,// sol_out 
		 &iout, work, &lwork, iwork, &liwork, &rpar, &ipar, &idid);
	RADAU5DATA_RELEASE();
	res = idid; // res takes idid value
	free(work);
	free(iwork); 
	CONSOLE_DEBUG("solving ended");
# ifdef ASC_SIGNAL_TRAPS
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in RADAU5 call.");
			RADAU5_FREE;//DOPRI5_FREE; /* FIXME what's this for? */
			return 6;
		}
		Asc_SignalHandlerPopDefault(SIGFPE);
		Asc_SignalHandlerPopDefault(SIGINT);

# endif

	switch(res)
	{
		case RADAU5_SUCCESS:
			break;
		case RADAU5_INRPT:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"RADAU5 interrupted by user");
			break;
		case RADAU5_BADINPUT:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad input to RADAU5");
			break;
		case RADAU5_ITERLIMIT:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Iteration limit exceeded in RADAU5");
			break;
		case RADAU5_STEPSMALL:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Step size became too small in RADAU5");
			break;
		case RADAU5_SINGULAR:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"singular matrix encountered in RADAU5");
			break;
	}

	if(res<0)
	{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Furthest point reached was t = %g.\n",x);
		RADAU5_FREE;
		return 7;
	}

	/* write final step output */
	CONSOLE_DEBUG("solving has reached this level \n blsys = %p",blsys);
	integrator_output_write_obs(blsys);
	integrator_output_close(blsys);

#ifdef STATS_DEBUG
	IntegratorRadau5Stats stats;
	if(0 == integrator_radau5_stats(blsys, &stats)){
		integrator_radau5_write_stats(&stats);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch stats!?!?");
	}
#endif

	CONSOLE_DEBUG("--- RADAU5 done ---");
	return 0; /* success */
}


