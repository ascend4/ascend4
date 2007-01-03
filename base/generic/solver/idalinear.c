#include "idalinear.h"
#include <ida/ida_impl.h>
#include <utilities/error.h>
#include <utilities/ascMalloc.h>

#include <sundials/sundials_math.h>
#define ZERO RCONST(0.0)
#define ONE  RCONST(1.0)
#define TWO  RCONST(2.0)

typedef struct IntegratorIdaAscendMemStruct{
	long                   integ_neq;   /* problem size */
	IntegratorSparseJacFn *integ_jacfn; /* sparse mtx jacobian evaluation function */
	void *                 integ_jac_data;               /* data for use by jacobvian evaluation function */
	int                    integ_lastflag;
	unsigned long          integ_nje;
	unsigned long          integ_nre;
	mtx_matrix_t           integ_sparse_jac_matrix;
} IntegratorIdaAscendMem;

/* readability replacements (see also ida_dense.c from SUNDIALS distro */

#define linit        (IDA_mem->ida_linit)
#define lsetup       (IDA_mem->ida_lsetup)
#define lsolve       (IDA_mem->ida_lsolve)
#define lperf        (IDA_mem->ida_lperf)
#define lfree        (IDA_mem->ida_lfree)
#define lmem         (IDA_mem->ida_lmem)
#define tn           (IDA_mem->ida_tn)
#define cjratio      (IDA_mem->ida_cjratio)
#define cj           (IDA_mem->ida_cj)

#define nje          (iamem->integ_nje)
#define nre          (iamem->integ_nre)
#define lastflag	 (iamem->integ_lastflag)
#define neq          (iamem->integ_neq)
#define jacfn        (iamem->integ_jacfn)
#define jacdata      (iamem->integ_jac_data)
#define JJ           (iamem->integ_sparse_jac_matrix)

#define MSGD_IDAMEM_NULL "Integrator memory is NULL."
#define MSGD_MEM_FAIL    "A memory request failed."
#define MSGD_LMEM_NULL   "IDAASCEND memory is NULL."
#define MSGD_JACFN_UNDEF "The sparse jacobian evaluation routine has not been provided."

/*------------------------------------
  Internal setup/evaluation routines required by IDA. As documented in IDA Manual Ch. 8
*/
int integrator_ida_linit(IDAMem ida_mem);

int integrator_ida_lsetup(IDAMem ida_mem, N_Vector yyp, N_Vector ypp,
	N_Vector resp, N_Vector vtemp1, N_Vector vtemp2, N_Vector vtemp3
);

int integrator_ida_lsolve(IDAMem ida_mem, N_Vector b, N_Vector weight,
	N_Vector ycur, N_Vector ypcur, N_Vector rescur
);

int integrator_ida_lfree(IDAMem ida_mem);

/*------------------------------------
  User functions (called from ida.c in this directory)
*/

int IDAASCEND(void *ida_mem, long _neq){
	IDAMem IDA_mem;
	IntegratorIdaAscendMem *iamem;

	if(ida_mem == NULL){
		IDAProcessError(NULL, IDAASCEND_MEM_NULL, "IDAASCEND", __FUNCTION__, MSGD_IDAMEM_NULL);
		return(IDAASCEND_MEM_NULL);
	}
	IDA_mem = (IDAMem)ida_mem;

	iamem = ASC_NEW(IntegratorIdaAscendMem);
	if(iamem == NULL){
		return IDAASCEND_MEM_FAIL;
	}
	lmem = (void *)iamem;

	/* free linsolver memory with the previous lfree fn, if allocated */
	if(lfree != NULL)lfree(ida_mem);

	/* set the internal-use linear solver function pointers for IDA */
	linit  = &integrator_ida_linit;
	lsetup = &integrator_ida_lsetup;
	lsolve = &integrator_ida_lsolve;
	lperf  = NULL;
	lfree  = &integrator_ida_lfree;

	/* no jacobian assigned, initially (we will throw an error if the user doesn't assign it though) */
	jacfn = NULL;
	lastflag = IDAASCEND_SUCCESS;
	neq = _neq;

	/* allocate memory required for Jacobian mtx_matrix_t ?? */

	return IDAASCEND_SUCCESS;
}

int IDAASCENDSetJacFn(void *ida_mem, IntegratorSparseJacFn *_jacfn, void *_jac_data){
	IDAMem IDA_mem;
	IntegratorIdaAscendMem *iamem;

	if(ida_mem == NULL){
		IDAProcessError(NULL, IDAASCEND_MEM_NULL, "IDAASCEND", __FUNCTION__, MSGD_IDAMEM_NULL);
		return(IDAASCEND_MEM_NULL);
	}
	IDA_mem = (IDAMem)ida_mem;

	if(lmem == NULL){
		IDAProcessError(ida_mem, IDAASCEND_LMEM_NULL, "IDAASCEND", __FUNCTION__, MSGD_LMEM_NULL);
		return(IDAASCEND_LMEM_NULL);
	}
	iamem = (IntegratorIdaAscendMem *)lmem;

	jacfn = _jacfn;

	return IDAASCEND_SUCCESS;
}

int IDAASCENDGetLastFlag(void *ida_mem, int *flag){
	IDAMem IDA_mem;
	IntegratorIdaAscendMem *iamem;

	if(ida_mem == NULL){
		IDAProcessError(NULL, IDAASCEND_MEM_NULL, "IDAASCEND", __FUNCTION__, MSGD_IDAMEM_NULL);
		return(IDAASCEND_MEM_NULL);
	}
	IDA_mem = (IDAMem)ida_mem;

	if(lmem == NULL){
		IDAProcessError(ida_mem, IDAASCEND_LMEM_NULL, "IDAASCEND", __FUNCTION__, MSGD_LMEM_NULL);
		return(IDAASCEND_LMEM_NULL);
	}
	iamem = (IntegratorIdaAscendMem *)lmem;

	*flag = lastflag;

	return IDAASCEND_SUCCESS;
}

char *IDAASCENDGetReturnFlagName(int flag){
	char *name;

	name = ASC_NEW_ARRAY(char,30);

	switch(flag) {
		case IDAASCEND_SUCCESS:
			sprintf(name,"IDAASCEND_SUCCESS");
			break;
		case IDAASCEND_MEM_NULL:
			sprintf(name,"IDAASCEND_MEM_NULL");
			break;
		case IDAASCEND_LMEM_NULL:
			sprintf(name,"IDAASCEND_LMEM_NULL");
			break;
		case IDAASCEND_MEM_FAIL:
			sprintf(name,"IDADENSE_MEM_FAIL");
			break;
		default:
			sprintf(name,"NONE");
	}

	return name;
}


/*------------------------------------
  Internal setup/evaluation routines required by IDA
*/

int integrator_ida_linit(IDAMem IDA_mem){
  	IntegratorIdaAscendMem *iamem;
	iamem = (IntegratorIdaAscendMem *)lmem;
  
	CONSOLE_DEBUG("Initialising IDA linear solver");
	nje = 0;
	nre = 0;
	jacfn = NULL;

	/* initialise anything else in the IntegratorIdaAscendMem that needs it */

	return 0;
}

int integrator_ida_lsetup(IDAMem IDA_mem
	, N_Vector yyp, N_Vector ypp, N_Vector rrp
	, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
	int retval;
	long int retfac;

  	IntegratorIdaAscendMem *iamem;
	iamem = (IntegratorIdaAscendMem *)lmem;

	CONSOLE_DEBUG("Setting up IDA linear problem");

	if(jacfn==NULL){
		lastflag = IDAASCEND_JACFN_UNDEF;
		return -1; /* unrecoverable */
	}

	/* Increment nje counter. */
	nje++;

	/* clear the jacobian matrix */
	/* ... */

	/* evaluate the jacobian */
	retval = jacfn(neq, tn, yyp, ypp, rrp, cj, jacdata, JJ, 
		tmp1, tmp2, tmp3
	);

	if(retval < 0){
		lastflag = IDAASCEND_JACFUNC_UNRECVR;
		return -1;
	}
	if (retval > 0) {
		lastflag = IDAASCEND_JACFUNC_RECVR;
		return +1;
	}

	/* do block decomposition, LU factorisation or whatever, return success or fail flag */
	/* ... */

	CONSOLE_DEBUG("Not implemented");
	return(-1);
}

/**
	This routines handles the linear solve operation for the IDAASCEND linear
	solver. It interfaces to the appropriate mtx_matrix routines for this,
	but also scales solution vector according to cjratio.

	@return IDAASCEND_SUCESS on success
*/
int integrator_ida_lsolve(IDAMem IDA_mem
	, N_Vector b, N_Vector weight
	, N_Vector ycur, N_Vector ypcur, N_Vector rrcur
){
	realtype *bd;
  
  	IntegratorIdaAscendMem *iamem;
	iamem = (IntegratorIdaAscendMem *)lmem;
  
	/* retrieve the data array for the RHS vector, 'b' */
	bd = N_VGetArrayPointer(b);

	/* call the necessary linsolqr routine here */
	/* ... */
	/* For IDADENSE, it's: DenseGETRS(JJ, pivots, bd); */

	/* not sure what this is doing yet */
	/* For IDADENSE, it's: Scale the correction to account for change in cj. */
	if(cjratio != ONE){
		N_VScale(TWO/(ONE + cjratio), b, b);
	}

	CONSOLE_DEBUG("Solving IDA linear problem (not implemented)");
	return -1;
}

int integrator_ida_lfree(IDAMem IDA_mem){
	CONSOLE_DEBUG("Freeing IDA linear solver data");

	/* free jacobian mtx_matrix_t data ?? */

	if(lmem!=NULL){
		ASC_FREE(lmem);
		lmem=NULL;
	}
	return 0;
}
