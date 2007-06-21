/**
	This is a header file to be used when calling TRON from C.

	TRON is written by Chih-Jen Lin and Jorge J. More', but is not
	included in the ASCEND distribution due to licensing restrictions.
*/
#ifndef TRON_H
#define TRON_H

#ifdef linux
# define DTRON dtron_
# define FNAME_LCASE_DECOR
#else
# error "System-dependent information required"
#endif

int DTRON(const int *n, double *x, const double *xl,
    const double *xu, double *f, double *g, double *a,
    double *adiag, int *acol_ptr, int *arow_ind,
    double *frtol, double *fatol, double *fmin, double *cgtol,
	int *itermax, double *delta, char *task, double *b,
    double *bdiag, int *bcol_ptr, int *brow_ind,
    double *l, double *ldiag, int *lcol_ptr, int *lrow_ind,
	double *xc, double *s, int *indfree, int *isave,
	double *dsave, double *wa, int *iwa
);
/**<
	This is the TRON trust region Newton method solver for the
	solution of large bound-constrained optimization problems

	      min { f(x) : xl <= x <= xu }

	where the Hessian matrix is sparse. The user must evaluate the
	function, gradient, and the Hessian matrix.

	@param n number of variables (in)
	@param x double precision array of dimension n (in/out)
	    On entry x specifies the vector x.
	    On exit x is the final minimizer.

	@param xl vector of length n, containing lower bound for each var (in)
	@param xu vector of length n, containing upper bound for each var (in)

	@param f double precision variable.
	    On entry f must contain the function at x.
	    On exit f is unchanged.

	@param g double precision array of dimension n.
	    On entry g must contain the gradient at x.
	    On exit g is unchanged.

	@param a double precision array of dimension nnz.
	    On entry a must contain the strict lower triangular part
	       of A in compressed column storage.
	    On exit a is unchanged.

	@param adiag double precision array of dimension n.
	    On entry adiag must contain the diagonal elements of A.
	    On exit adiag is unchanged.

	@param acol_ptr integer array of dimension n + 1.
	    On entry acol_ptr must contain pointers to the columns of A.
	       The nonzeros in column j of A must be in positions
	       acol_ptr(j), ... , acol_ptr(j+1) - 1.
	    On exit acol_ptr is unchanged.

	@param arow_ind integer array of dimension nnz.
	    On entry arow_ind must contain row indices for the strict
	       lower triangular part of A in compressed column storage.
	    On exit arow_ind is unchanged.

	@param frtol double precision variable.
	    On entry frtol specifies the relative error desired in the
	       function. Convergence occurs if the estimate of the
	       relative error between f(x) and f(xsol), where xsol
	       local minimizer, is less than frtol.
	    On exit frtol is unchanged.

	@param fatol double precision variable.
	    On entry fatol specifies the absolute error desired in the
	       function. Convergence occurs if the estimate of the
	       absolute error between f(x) and f(xsol), where xsol
	       is a local minimizer, is less than fatol.
	    On exit fatol is unchanged.

	@param fmin double precision variable.
	    On entry fmin specifies a lower bound for the function.
	       The subroutine exits with a warning if f < fmin.
	    On exit fmin is unchanged.

	@param cgtol double precision variable.
	    On entry cgtol specifies the convergence criteria for
	       the conjugate gradient method.
	    On exit cgtol is unchanged.

	@param itermax max number of conjugate gradient iterations (input)

	@param delta trust region bound (input)

	@param task character variable of length at least 60.
	    On initial entry task must be set to 'START'.
	    On exit task indicates the required action:

	       If task(1:1) = 'F' then evaluate the function at x.

	       If task(1:2) = 'GH' then evaluate the gradient and the
	       Hessian matrix at x.

	       If task(1:4) = 'CONV' then the search is successful.

	       If task(1:4) = 'WARN' then the subroutine is not able
	       to satisfy the convergence conditions. The exit value
	       of x contains the best approximation found.

	---
	@param b double precision array of dimension nnz + n*p.
	    On entry b need not be specified.
	    On exit b contains the strict lower triangular part
	       of B in compressed column storage.

	@param bdiag diagonal elements of B (returned into user-allocated space of size n)

	@param bcol_ptr integer array of dimension n + 1.
	    On entry bcol_ptr need not be specified
	    On exit bcol_ptr contains pointers to the columns of B.
	       The nonzeros in column j of B are in the
	       bcol_ptr(j), ... , bcol_ptr(j+1) - 1 positions of b.

	@param brow_ind integer array of dimension nnz.
	    On entry brow_ind need not be specified.
	    On exit brow_ind contains row indices for the strict lower
	       triangular part of B in compressed column storage.
	---
	@param l double precision array of dimension nnz + n*p.
	    On entry l need not be specified.
	    On exit l contains the strict lower triangular part
	       of L in compressed column storage.

	@param ldiag double precision array of dimension n.
	    On entry ldiag need not be specified.
	    On exit ldiag contains the diagonal elements of L.

	@param lcol_ptr integer array of dimension n + 1.
	    On entry lcol_ptr need not be specified.
	    On exit lcol_ptr contains pointers to the columns of L.
	       The nonzeros in column j of L are in the
	       lcol_ptr(j), ... , lcol_ptr(j+1) - 1 positions of l.

	@param lrow_ind integer array of dimension nnz + n*p.
	    On entry lrow_ind need not be specified.
	    On exit lrow_ind contains row indices for the strict lower
	       triangular part of L in compressed column storage.
    ---

	@param xc double precision working array of dimension n.

	@param s double precision working array of dimension n.

	@param indfree integer working array of dimension n.

	@param isave integer working array of dimension 3.

	@param dsave is  double precision working array of dimension 3.

	@param wa double precision work array of dimension 7*n.

	@param iwa integer work array of dimension 3*n.


	This subroutine uses reverse communication.
	The user must choose an initial approximation x to the minimizer,
	and make an initial call with task set to 'START'.
	On exit task indicates the required action.

	A typical invocation has the following outline:

	Compute a starting vector x.
	Compute the sparsity pattern of the Hessian matrix and
	store in compressed column storage in (acol_ptr,arow_ind).

	char task[60] = "START";
	while(search){

		if(strcmp(task,'F')==0 || strcmp(task,"START")==0){

			// Evaluate the function at x and store in f.

		}
		if(strcmp(task,'GH')==0 || strcmp(task,"START")==0){

			// Evaluate the gradient at x and store in g.

			// Evaluate the Hessian at x and store in compressed
				column storage in (a,adiag,acol_ptr,arow_ind)
		}

		dtron(n,x,xl,xu,f,g,a,adiag,acol_ptr,arow_ind,
			frtol,fatol,fmin,cgtol,itermax,delta,task,
			b,bdiag,bcol_ptr,brow_ind,
			l,ldiag,lcol_ptr,lrow_ind,
			xc,s,indfree,
			isave,dsave,wa,iwa
		);

		if(strncmp(task,"CONV",4)==0)search = FALSE;

	}

	NOTE: The user must not alter work arrays between calls.

	TRON calls the following routines from MINPACK-2:
		dcauchy, dspcg, dssyax

	TRON also calls the 'dcopy' routine from Level 1 BLAS:
*/

/* the following macros help with dlopening etc */

#define TRON_DTRON_ARGS (const int *n, double *x, const double *xl, \
    const double *xu, double *f, double *g, double *a,\
    double *adiag, int *acol_ptr, int *arow_ind,\
    double *frtol, double *fatol, double *fmin, double *cgtol,\
	int *itermax, double *delta, char *task, double *b,\
    double *bdiag, int *bcol_ptr, int *brow_ind,\
    double *l, double *ldiag, int *lcol_ptr, int *lrow_ind,\
	double *xc, double *s, int *indfree, int *isave,\
	double *dsave, double *wa, int *iwa)

#define TRON_DTRON_VALS (n,x,xl,xu,f,g,a,adiag,acol_ptr,arow_ind,\
			frtol,fatol,fmin,cgtol,itermax,delta,task,\
			b,bdiag,bcol_ptr,brow_ind,\
			l,ldiag,lcol_ptr,lrow_ind,\
			xc,s,indfree,\
			isave,dsave,wa,iwa)

typedef int dtron_fn_t TRON_DTRON_ARGS;

#endif /* TRON_H */

