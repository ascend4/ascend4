/*
 *  External Definitions of CONOPT Subroutines
 *  by Vicente Rico Ramirez
 *  Created: 05/97
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: conopt.h,v $
 *  Date last modified: $Date: 1998/02/26 15:57:56 $
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

#ifndef conopt__already_included
#define conopt__already_included

/* 
 * Macros defined because of the different convention of Fortran from C about  
 * the use of an index in arrays (starting from zero or from one).
 */
#define F2C(x) x - 1
#define C2F(x) x + 1

/*
 * Parameter required for CONOPT subroutines
 */
#define NINTGR 3
#define MAX_INT 20000
#define MAX_REAL 10e300

/* 
 * CONOPT data structure.
 */
struct conopt_data {
  int32 n;                      /* number of columns */
  int32 m;                      /* number of rows */
  int32 nz;                     /* number of nonzeros */
  int32 maxrow;                 /* number of elements in densest row */
  int32 modsta;                 /* model status */
  int32 solsta;                 /* solver status */
  int32 iter;                   /* # of conopt iterations */
  real64 obj;                   /* objective value */

  real64 *work;                 /* work space */
  int32 minmem;                 /* minimum memory suggested by conopt */
  int32 estmem;                 /* estimated memory suggested by conopt */
  int32 lwork;                  /* size of allocated workspace */
  int32 nintgr;                 /* size of problem size vector */
  int32 ipsz[NINTGR];           /* problem size vector */

  int32 kept;                   /* if 1 can call warm conopt restart */

  int32 optimized;              /* has conopt been called? */
  int32 maxusd;                 /* maximum work space used */
  int32 curusd;                 /* current work space used */
  int32 opt_count;              /* count of calls to coiopt */
  int32 progress_count;         /* count of calls to coiprg */
};

/*
 * Structure of function pointers, so that a particular solver can define
 * its user-defined CONOPT subroutines independently
 */
struct conopt_function_pointers {
  void (*coirms_ptr)();
  void (*coifbl_ptr)();
  void (*coifde_ptr)();
  void (*coista_ptr)();
  void (*coirs_ptr)();
  void (*coiusz_ptr)();
  void (*coiopt_ptr)();
  void (*coipsz_ptr)();
  void (*coimsg_ptr)();
  void (*coiscr_ptr)();
  void (*coiec_ptr)();
  void (*coier_ptr)();
  void (*coienz_ptr)();
  void (*coiprg_ptr)();
  void (*coiorc_ptr)();
};

/*
 * Pointer to the previous structure
 */
typedef struct conopt_function_pointers *conopt_pointers;


/*
 * is CONOPT available ?
 */
#if (defined(STATIC_CONOPT) || defined(DYNAMIC_CONOPT))
#define CONOPT_ACTIVE TRUE
#else  /* defined(STATIC_CONOPT) */
#define CONOPT_ACTIVE FALSE
#endif /* defined(STATIC_CONOPT) */


#if CONOPT_ACTIVE  /* code used if CONOPT is available */
/*
 *  Take care of fortran underbar madness
 */
#ifdef sun
#define FORTRAN_UNDERBARS
#endif /* sun */

#ifdef __alpha
#define FORTRAN_UNDERBARS
#endif /* __alpha */

#ifdef __sgi
#define FORTRAN_UNDERBARS
#endif /* __sgi */

#ifdef __WIN32__
/* dec visual fortran */
#define COICSM COICSM
#define COIMEM COIMEM
#define COIRMS COIRMS
#define COIFBL COIFBL
#define COIFDE COIFDE
#define COISTA COISTA
#define COIRS  COIRS
#define COIUSZ COIUSZ
#define COIOPT COIOPT
#define COIPSZ COIPSZ
#define COIMSG COIMSG
#define COISCR COISCR
#define COIEC COIEC
#define COIER COIER
#define COIENZ COIENZ
#define COIPRG COIPRG
#define COIORC COIORC
#else
/* unixisms */
#ifdef FORTRAN_UNDERBARS
#define COICSM coicsm_
#define COICRM coicrm_
#define COIMEM coimem_
#define COIRMS coirms_
#define COIFBL coifbl_
#define COIFDE coifde_
#define COISTA coista_
#define COIRS  coirs_
#define COIUSZ coiusz_
#define COIOPT coiopt_
#define COIPSZ coipsz_
#define COIMSG coimsg_
#define COISCR coiscr_
#define COIEC coiec_
#define COIER coier_
#define COIENZ coienz_
#define COIPRG coiprg_
#define COIORC coiorc_
#else
#define COICSM coicsm
#define COICRM coicrm
#define COIMEM coimem
#define COIRMS coirms
#define COIFBL coifbl
#define COIFDE coifde
#define COISTA coista
#define COIRS  coirs
#define COIUSZ coiusz
#define COIOPT coiopt
#define COIPSZ coipsz
#define COIMSG coimsg
#define COISCR coiscr
#define COIEC coiec
#define COIER coier
#define COIENZ coienz
#define COIPRG coiprg
#define COIORC coiorc
#endif  /* FORTRAN_UNDERBARS */
#endif /* !WIN */


/*
 *  Optimization subroutines for CONOPT
 *  ---------------------------------
 */

extern void COIRMS(real64 *, real64 *, real64 *, int32 *, int32 *, real64 *,
		   real64 *, int32 *, int32 *, int32 *, real64 *, int32 *,
		   int32 *, int32 *, int32 *, int32 *, real64 *);
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


extern void COIFBL(real64 *, real64 *, int32 *, int32 *, int32 *, int32 *, 
		   real64 *, int32 *, int32 *, int32 *, int32 *, int32 *,
	           int32 *, int32 *, int32 *, int32 *, int32 *, int32 *, 
		   int32 *, int32 *, int32 *, real64 *);
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


extern void COIFDE(real64 *, real64 *, real64 *, int32 *, int32 *, int32 *,
		   int32 *, int32 *, int32 *, int32 *, real64 *);
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


extern void COISTA(int32 *, int32 *, int32 *, real64 *, real64 *);
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


extern void COIRS(real64 *, real64 *, int32 *, int32 *, real64 *, real64 *,
		  int32 *, int32 *, int32 *, int32 *, real64 *);
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


extern void COIUSZ(int32 *, int32 *, int32 *, real64 *, real64 *);
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


extern void COIOPT(char *, real64 *, int32 *, int32 *, real64 *);
/*
 * COIOPT communicates non-default option values to CONOPT
 * COIOPT(name, rval, ival, lval, usrmem)
 * name   - the name of a CONOPT CR-cell defined by the modeler
 * rval   - the value to be assigned to name if the cells contains a real
 * ival   - the value to be assigned to name if the cells contains an int
 * lval   - the value to be assigned to name if the cells contains a log value
 * usrmem - user memory
 */


extern void COIPSZ(int32 *, int32 *, int32 *, real64 *, real64 *);
/*
 * COIPSZ communicates the model size and structure to CONOPT
 * COIPSZ(nintg, ipsz, nreal, rpsz, usrmem)
 *
 * ningt  - number of positions in ipsz
 * ipsz   - array describing problem size and options
 * nreal  - number of positions in rpsz
 * rpsz   - array of reals describing problem size and options
 * usrmem - user memory
 */

extern void COIMSG (int32 *nmsg, int32 *smsg, int32 *llen,
      char msgv[80*15],real64 *usrmem);

extern void COISCR (char msg[80], int32 *len);

extern void COIEC (int32 *colno, int32 *msglen, char msg[80],real64 *usrmem);

extern void COIER (int32 *rowno, int32 *msglen, char msg[80],real64 *usrmem);

extern void COIENZ (int32 *colno, int32 *rowno, int32 *posno,
      int32 *msglen, char msg[80],real64 *usrmem);

extern void COIPRG (int32 *nintgr, int32 *intrep, int32 *nreal,
      real64 *rl, real64 *x, real64 *usrmem, int32 *finish);

extern void COIORC (int32 *colno, int32 *rowno, real64 *value,
      real64 *resid,real64 *usrmem);

/*
 * IMPORTANT: The use of the following functions is a   H A C K   to avoid
 * unresolved externals while linking to the CONOPT library. For some
 * reason, the linker wants the calls to the provided subroutines 
 * COICSM and COIMEM in the same file as the definition of the user 
 * defined CONOPT subroutines
 */

/*
 * Passes arguments to COIMEM
 */
extern void conopt_estimate_memory(int32 *, int32 *, int32 *, int32 *);

/*
 * Passes arguments to COICRM
 */
extern void conopt_restart(int32 *, real64 **, int32 *, real64 *, int32 *, 
			   int32 *);

/*
 * Passes arguments to COICSM
 */
extern void conopt_start(int32 *, real64 **, int32 *, real64 *, int32 *, 
			 int32 *);


#if defined(DYNAMIC_CONOPT)
/*
 * conopt_load attempts to dynamically load CONOPT.
 * Returns 0 for success, 1 for failure.
 */
int32 conopt_load(void);
#else
/*
 * CONOPT Provided Subroutines.Only the external definition. The
 * code for the subroutines is provided by CONOPT.
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
extern void COIMEM(int32 *, int32 *, int32 *, int32 *);

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
extern void COICRM(int32 *, real64 *, int32 *, real64 *, int32 *, int32 *);

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
extern void COICSM(int32 *, real64 *, int32 *, real64 *, int32 *, int32 *);
#endif /* DYNAMIC_CONOPT */
#endif /* if CONOPT_ACTIVE */
#endif /* conopt__already_included */


