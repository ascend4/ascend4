/*
 *  Definitions of CONOPT Subroutines
 *  by Vicente Rico-Ramirez
 *  Created: 07/97
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: conopt.c,v $
 *  Date last modified: $Date: 1998/02/27 14:33:23 $
 *  Last modified by: $Author: mthomas $
 *  
 *  This file is part of the SLV solver.
 *  
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *  
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

#include <utilities/ascConfig.h>
#include "conopt.h"
#if defined(DYNAMIC_CONOPT)
#include <utilities/ascDynaLoad.h>
#include "conoptdll.h"
#endif

/*
 * is CONOPT available ?
 */
#if (defined(STATIC_CONOPT) || defined(DYNAMIC_CONOPT))
#define CONOPT_ACTIVE TRUE
#else   /* defined(STATIC_CONOPT) */
#define CONOPT_ACTIVE FALSE
#endif /* defined(STATIC_CONOPT) */

#if CONOPT_ACTIVE  /* code used if CONOPT is available */

/*
 *  Optimization subroutines for CONOPT
 *  ---------------------------------
 */

/*
 *  User-defined subroutines
 *  ------------------------
 */

/*
 * COIRMS Based on the information provided in Coispz, CONOPT will
 * allocate the number of vectors into which the user can define
 * the details of the model. The details of the model are defined
 * here.
 *
 * COIRMS(lower, curr, upper, vsta, type,rhs, fv, esta, colsta,
 * rowno, value, nlflag, n, m, n1, nz, usrmem)
 *
 * lower - lower bounds on the variables
 * curr  - intial values of the variables
 * upper - upper bounds on the variables
 * vsta  - initial status of the variable(o nonbasic, 1 basic)
 * type  - types of equations (0 equality,1 greater,2 less)
 * rhs   - values of the right hand sides
 * fv    - sum of the nonlinear terms in the initial point
 * esta  - initial status of the slack in the constraint (nonbasic,basic)
 * colsta- start of column pointers
 * rowno - row or equation numbers of the nonzeros
 * value - values of the jacobian elements
 * nlflag- nonlinearity flags(0 nonzero constant,1 varying)
 * n     - number of variables
 * m     - number of constraints
 * n1    - n+1
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt 
 */
void COIRMS(real64 *lower, real64 *curr, real64 *upper, int32 *vsta, 
	    int32 *type, real64 *rhs, real64 *fv, int32 *esta, int32 *colsta,
	    int32 *rowno, real64 *value, int32 *nlflag, int32 *n, int32 *m,
	    int32 *n1, int32 *nz, real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem; 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coirms_ptr == NULL) {
    return;
  }
  conopt_ptrs->coirms_ptr(lower, curr, upper, vsta, type, rhs, fv, esta, 
			  colsta, rowno, value, nlflag, n, m, n1, nz, 
			  usr_mem[1]);
}


/*
 * COIFBL Defines the nonlinearities of the model by returning
 * numerical values. It works on a block of rows during each call.
 * COIFBL( x, g, otn, nto, from, to, jac, stcl, rnum, cnum, nl, strw,
 *         llen, indx, mode, errcnt, n, m, n1, m1, nz, usrmem)
 *
 * x     - punt of evaluation provided by conopt
 * g     - vector of function values
 * otn   - old to new permutation vector
 * nto   - new to old permutation vector
 * from  - range in permutation
 * to    - range in permutation
 * jac   - vector of jacobian values.
 *         The following are vectors defining the jacobian structure
 * stcl  - start of column pointers
 * rnum  - row numbers
 * cnum  - column numbers
 * nl    - nonlinearity flags
 * strw  - start row pointers
 * llen  - count of linear jacobian elements
 * indx  - pointers from the row-wise representation
 * mode  - indicator of mode of evaluation
 * errcnt- number of function evaluation errors
 * n     - umber of variables
 * m     - number of constraints
 * n1    - n+1
 * m1    - m+1
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt 
 */
void COIFBL(real64 *x, real64 *g, int32 *otn, int32 *nto, int32 *from, 
	    int32 *to, real64 *jac, int32 *stcl, int32 *rnum, int32 *cnum, 
	    int32 *nl, int32 *strw, int32 *llen, int32 *indx, int32 *mode, 
	    int32 *errcnt, int32 *n, int32 *m, int32 *n1, int32 *m1, 
	    int32 *nz, real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem; 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coifbl_ptr == NULL) {
    return;
  }
  conopt_ptrs->coifbl_ptr(x, g, otn, nto, from, to, jac, stcl, rnum, cnum, 
			  nl, strw, llen, indx, mode, errcnt, n, m, n1, m1, 
			  nz, usr_mem[1]);

}

/*
 * COIFDE Defines the nonlinearities of the model by returning
 * numerical values. It works on one row or equation at a time
 * COIFDE(x, g, jac, rowno, jcnm, mode, errcnt, newpt, n, nj, usrmem)
 *
 * x      - punt of evaluation provided by conopt
 * g      - function value
 * jac    - jacobian values
 * rowno  - number of the row for which nonlinearities will be eval
 * jcnm   - list of column number fon the NL nonzeros
 * mode   - indicator of mode of evaluation
 * errcnt - sum of number of func evaluation errors thus far
 * newpt  - new point indicator
 * nj     - number of nonlinear nonzero jacobian elements
 * n      - number of variables
 * usrmem - user memory
 */
void COIFDE(real64 *x, real64 *g, real64 *jac, int32 *rowno, int32 *jcnm,
	    int32 *mode, int32 *errcnt, int32 *newpt, int32 *n, int32 *nj,
	    real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coifde_ptr == NULL) {
    return;
  }
  conopt_ptrs->coifde_ptr(x, g, jac, rowno, jcnm, mode, errcnt, newpt, n, 
			  nj, usr_mem[1]);
}



/*
 * COISTA Pass the solution from CONOPT to the modeler. It returns
 * completion status
 * COISTA(modsta, solsts, iter, objval, usrmem)
 *
 * modsta - model status
 * solsta - solver status
 * iter   - number of iterations
 * objval - objective value
 * usrmem - user memory
 */
void COISTA(int32 *modsta, int32 *solsta, int32 *iter, real64 *objval,
	    real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coista_ptr == NULL) {
    return;
  }
  conopt_ptrs->coista_ptr(modsta, solsta, iter, objval, usr_mem[1]);
}


/*
 * COIRS passes the solution from CONOPT to the modeler. It returns
 * solution values
 * COIRS(val, xmar, xbas, xsta, yval, ymar, ybas, ysta, n, m, usrmem)
 *
 * xval   - the solution values of the variables
 * xmar   - corresponding marginal values
 * xbas   - basis indicator for column (at bound, basic, nonbasic)
 * xsta   - status of column (normal, nonoptimal, infeasible,unbounded)
 * yval   - values of the left hand side in all the rows
 * ymar   - corresponding marginal values
 * ybas   - basis indicator for row
 * ysta   - status of row
 * n      - number of variables
 * m      - number of constraints
 * usrmem - user memory
 */
void COIRS(real64 *xval, real64 *xmar, int32 *xbas, int32 *xsta, real64 *yval,
	   real64 *ymar, int32 *ybas, int32 * ysta, int32 *n, int32 *m,
	   real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coirs_ptr == NULL) {
    return;
  }
  conopt_ptrs->coirs_ptr(xval, xmar, xbas, xsta, yval, ymar, ybas, ysta, 
			 n, m, usr_mem[1]);
}


/*
 * COIUSZ communicates and update of an existing model to CONOPT
 * COIUSZ(nintg, ipsz, nreal, rpsz, usrmem)
 *
 * nintg - number of positions in ipsz
 * ipsz  - array describing problem size and options
 * nreal - number of positions in rpsz
 * rpsz  - array of reals describing problem size and options
 * usrmem- user memory
 */
void COIUSZ(int32 *nintg, int32 *ipsz, int32 *nreal, real64 *rpsz, 
	    real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coiusz_ptr == NULL) {
    return;
  }
  conopt_ptrs->coiusz_ptr(nintg, ipsz, nreal, rpsz, usr_mem[1]);
}


/*
 * COIOPT communicates non-default option values to CONOPT
 * COIOPT(name, rval, ival, lval, usrmem)
 * name   - the name of a CONOPT CR-cell defined by the modeler
 * rval   - the value to be assigned to name if the cells contains a real
 * ival   - the value to be assigned to name if the cells contains an int
 * lval   - the value to be assigned to name if the cells contains a log value
 * usrmem - user memory
 */
void COIOPT(char *name, real64 *rval, int32 *ival, int32 *logical, 
	    real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coiopt_ptr == NULL) {
    return;
  }
  conopt_ptrs->coiopt_ptr(name, rval, ival, logical, usr_mem[1]);
}


/*
 * COIPSZ communicates the model size and structure to CONOPT
 * COIPSZ(nintgr, ipsz, nreal, rpsz, usrmem)
 *
 * ningtr - number of positions in ipsz
 * ipsz   - array describing problem size and options
 * nreal  - number of positions in rpsz
 * rpsz   - array of reals describing problem size and options
 * usrmem - user memory
 */
void COIPSZ(int32 *nintgr, int32 *ipsz, int32 *nreal, real64 *rpsz, 
	    real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coipsz_ptr == NULL) {
    return;
  }
  conopt_ptrs->coipsz_ptr(nintgr, ipsz, nreal, rpsz, usr_mem[1]);
}


extern void COIMSG (int32 *nmsg, int32 *smsg, int32 *llen,
      char msgv[80*15],real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coimsg_ptr == NULL) {
    return;
  }
  conopt_ptrs->coimsg_ptr(nmsg, smsg, llen, msgv, usr_mem[1]);
}

extern void COISCR (char msg[80], int32 *len)
{
  FPRINTF(stdout,"%.*s\n",*len,&msg[0]);
}

extern void COIEC (int32 *colno, int32 *msglen, char msg[80],real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coiec_ptr == NULL) {
    return;
  }
  conopt_ptrs->coiec_ptr(colno, msglen, msg, usr_mem[1]);
}

extern void COIER (int32 *rowno, int32 *msglen, char msg[80],real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coier_ptr == NULL) {
    return;
  }
  conopt_ptrs->coier_ptr(rowno, msglen, msg, usr_mem[1]);
}

extern void COIENZ (int32 *colno, int32 *rowno, int32 *posno,
      int32 *msglen, char msg[80],real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coienz_ptr == NULL) {
    return;
  }
  conopt_ptrs->coienz_ptr(colno, rowno, posno, msglen, msg, usr_mem[1]);
}

extern void COIPRG (int32 *nintgr, int32 *intrep, int32 *nreal,
      real64 *rl, real64 *x, real64 *usrmem, int32 *finish)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coiprg_ptr == NULL) {
    return;
  }
  conopt_ptrs->coiprg_ptr(nintgr, intrep, nreal, rl, x, usr_mem[1], finish);
}

extern void COIORC (int32 *colno, int32 *rowno, real64 *value,
      real64 *resid,real64 *usrmem)
{
  conopt_pointers conopt_ptrs;
  real64 **usr_mem;

  usr_mem = (real64 **)usrmem;
 
  conopt_ptrs = (conopt_pointers)usr_mem[0];
  if (conopt_ptrs->coiorc_ptr == NULL) {
    return;
  }
  conopt_ptrs->coiorc_ptr(colno, rowno, value, resid, usr_mem[1]);
}

#if defined(DYNAMIC_CONOPT)
REGISTER_CONOPT_FUNCTION_FUNC *register_conopt_function;
UNREGISTER_CONOPT_FUNCTION_FUNC *unregister_conopt_function;
COICRM_FUNC *COICRM;
COICSM_FUNC *COICSM;
COIMEM_FUNC *COIMEM;
#endif /* DYNAMIC_CONOPT */

/*
 *  Provided subroutines coicsm and coimem
 *  --------------------------------------
 */
/*
 * IMPORTANT: The use of the following functions is a   H A C K   to aovid
 * unresolved externals while linking to the CONOPT library. For some
 * reason, the linker wants the calls to the provided subroutines 
 * COICSM and COIMEM in the same file as the definition of the user 
 * defined CONOPT subroutines
 */
/* 
 * Memory estimation by using CONOPT subroutine coimem
 *
 * COIMEM Estimates the amount of memory needed by CONOPT
 * COIMEM(nintgr, ipsz, minmem, estmem)
 *
 * nintgr   - number of elements in the array ipsz. Should be 3.
 * ipsz     - vector of integers to describe the size of the model
 * minmem   - Minimum estimate for the memory needed. Measured in
 *            number of real elements of work
 * estmem   - Estimate of the amount of memory
 */
void conopt_estimate_memory(int32 *nintgr, int32 *ipsz, int32 *minmem,
			    int32 *estmem)
{
  COIMEM(nintgr, ipsz, minmem, estmem);
}


/*
 * COICRM restarts CONOPT with user memory
 * COICRM(kept, usrmem, lwork, work, maxusd, curusd)
 *
 * kept   - Whether CONOPT has kept the model after solving it or not
 * usrmem - array passed to all subroutines. If not needed is dummy array
 * lwork  - lenght of working array work
 * work   - working array supplied by the user
 * maxusd - maximum amount of memory in work used during optimization
 * curusd - current amount of memory in use
 */
void conopt_restart(int32 *kept, real64 **usrmem, int32 *lwork, real64 *work, 
		    int32 *maxusd, int32 *curusd)
{ 
  real64 *usr_mem;
  usr_mem = (real64 *)usrmem;

  FPRINTF(ASCERR,"\n");
  FPRINTF(ASCERR,"Restarting Conopt\n");
  FPRINTF(ASCERR,"\n");

  COICRM(kept, usr_mem, lwork, work, maxusd,curusd);
}


/*
 * COICSM starts up CONOPT with user memory
 * COICSM(kept, usrmem, lwork, work, maxusd, curusd)
 *
 * kept   - Whether CONOPT has kept the model after solving it or not
 * usrmem - array passed to all subroutines. If not needed is dummy array
 * lwork  - lenght of working array work
 * work   - working array supplied by the user
 * maxusd - maximum amount of memory in work used during optimization
 * curusd - current amount of memory in use
 */
void conopt_start(int32 *kept, real64 **usrmem, int32 *lwork, real64 *work, 
		  int32 *maxusd, int32 *curusd)
{ 
  conopt_pointers conopt_ptrs;
  real64 *usr_mem;
  usr_mem = (real64 *)usrmem;

  FPRINTF(ASCERR,"\n");
  FPRINTF(ASCERR,"Starting Conopt\n");
  FPRINTF(ASCERR,"\n");

#if defined(DYNAMIC_CONOPT)
  conopt_ptrs = (conopt_pointers)usrmem[0];
  
  register_conopt_function(COIRMS_ENUM,
    conopt_ptrs->coirms_ptr == NULL ? NULL : (void *)COIRMS);
  register_conopt_function(COIFBL_ENUM,
    conopt_ptrs->coifbl_ptr == NULL ? NULL : (void *)COIFBL);
  register_conopt_function(COIFDE_ENUM,
    conopt_ptrs->coifde_ptr == NULL ? NULL : (void *)COIFDE);
  register_conopt_function(COIRS_ENUM,
    conopt_ptrs->coirs_ptr == NULL ? NULL : (void *)COIRS);
  register_conopt_function(COISTA_ENUM,
    conopt_ptrs->coista_ptr == NULL ? NULL : (void *)COISTA);
  register_conopt_function(COIUSZ_ENUM,
    conopt_ptrs->coiusz_ptr == NULL ? NULL : (void *)COIUSZ);
  register_conopt_function(COIOPT_ENUM,
    conopt_ptrs->coiopt_ptr == NULL ? NULL : (void *)COIOPT);
  register_conopt_function(COIPSZ_ENUM,
    conopt_ptrs->coipsz_ptr == NULL ? NULL : (void *)COIPSZ);

  register_conopt_function(COIMSG_ENUM,
    conopt_ptrs->coimsg_ptr == NULL ? NULL : (void *)COIMSG);
  register_conopt_function(COISCR_ENUM,(void *)COISCR);
  register_conopt_function(COIEC_ENUM,
    conopt_ptrs->coiec_ptr == NULL ? NULL : (void *)COIEC);
  register_conopt_function(COIER_ENUM,
    conopt_ptrs->coier_ptr == NULL ? NULL : (void *)COIER);
  register_conopt_function(COIENZ_ENUM,
    conopt_ptrs->coienz_ptr == NULL ? NULL : (void *)COIENZ);
  register_conopt_function(COIPRG_ENUM,
    conopt_ptrs->coiprg_ptr == NULL ? NULL : (void *)COIPRG);
  register_conopt_function(COIORC_ENUM,
    conopt_ptrs->coiorc_ptr == NULL ? NULL : (void *)COIORC);

#endif /* DYNAMIC_CONOPT */
  COICSM(kept, usr_mem, lwork, work, maxusd,curusd);
}
#if defined(DYNAMIC_CONOPT)
int32 conopt_loaded = 1;

int32 conopt_load(void) {
  int32 status;
  if (conopt_loaded == 0) {
    return 0; /* allready loaded */
  }
  status = Asc_DynamicLoad("dllcnsub.dll", NULL);
  if (status != 0) {
    return 1; /* failure */
  }
  register_conopt_function =
    (REGISTER_CONOPT_FUNCTION_FUNC *)Asc_DynamicFunction("dllcnsub.dll",
    "register_conopt_function");
  unregister_conopt_function =
    (UNREGISTER_CONOPT_FUNCTION_FUNC *)Asc_DynamicFunction("dllcnsub.dll",
    "unregister_conopt_function");
  COICRM = (COICRM_FUNC *)Asc_DynamicFunction("dllcnsub.dll","COICRM");
  COICSM = (COICSM_FUNC *)Asc_DynamicFunction("dllcnsub.dll","COICSM");
  COIMEM = (COIMEM_FUNC *)Asc_DynamicFunction("dllcnsub.dll","COIMEM");
  if (register_conopt_function == NULL ||
    unregister_conopt_function == NULL ||
    COICRM == NULL ||
    COICSM == NULL ||
    COIMEM == NULL) {
    return 1; /* failure */
  }
  conopt_loaded = 0;
  return 0;
}
#endif /* DYNAMIC_CONOPT */
#endif /* CONOPT_ACTIVE */
