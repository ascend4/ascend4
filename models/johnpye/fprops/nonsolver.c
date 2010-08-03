#include "nonsolver.h"
#include "derivs.h"

#include <stdio.h>
#include <gsl/gsl_multiroots.h>

/*------------------------------------------------------------------------------
  Iterative two-way solver for the non-saturation region, making use of GSL.
*/

typedef double PropertyFunction(double, double,const HelmholtzData *);

typedef struct{
	FPROPS_CHAR A,B;
	PropertyFunction *Afn, *Bfn;
	double a,b;
	const HelmholtzData *D;
} Solver2Data;

static PropertyFunction *nonsolver_propfn(FPROPS_CHAR A){
	switch(A){
		case 'p': return &helmholtz_p_raw;
		case 'u': return &helmholtz_u;
		case 's': return &helmholtz_s;
		case 'h': return &helmholtz_h;
		default: return 0;
	}
}

static int nonsolver_f(const gsl_vector *x, void *user_data, gsl_vector *f){
#define U ((Solver2Data *)user_data)
	double T = gsl_vector_get(x,0);
	double rho = gsl_vector_get(x,1);
	gsl_vector_set(f, 0, (*(U->Afn))(rho,T,U->D) - (U->a));
	gsl_vector_set(f, 1, (*(U->Bfn))(rho,T,U->D) - (U->b));
	return GSL_SUCCESS;
#undef U
}

static int nonsolver_df(const gsl_vector *x, void *user_data, gsl_matrix *J){
#define U ((Solver2Data *)user_data)
	double T = gsl_vector_get(x,0);
	double rho = gsl_vector_get(x,1);
	//SteamState S = freesteam_region3_set_rhoT(rho,T); /* FIXME work out the best call sig for dZdv_T etc */
	gsl_matrix_set(J, 0, 0, -1./SQ(rho) * fprops_non_dZdv_T(U->A,T,rho,U->D));
	gsl_matrix_set(J, 0, 1, fprops_non_dZdT_v(U->A,T,rho,U->D));
	gsl_matrix_set(J, 1, 0, -1./SQ(rho) * fprops_non_dAdv_T(U->B,T,rho,U->D));
	gsl_matrix_set(J, 1, 1, fprops_non_dAdT_v(U->B,T,rho,U->D));
	return GSL_SUCCESS;
#undef U
}

static int nonsolver_fdf(const gsl_vector *x, void *user_data, gsl_vector *f, gsl_matrix *J){
	return nonsolver_f(x, user_data, f) || nonsolver_df(x, user_data, J);
}

#ifdef NONSOLVER_DEBUG
static void nonsolver_print_state(size_t iter, gsl_multiroot_fdfsolver *s){
	double T = gsl_vector_get(x,0);
	double rho = gsl_vector_get(x,1);
	fprintf(stderr,"iter = %lu: rho = %g, T = %g\n", iter,rho,T);
}
#endif

/**
	Two-way solver for the non-saturation region.
	@param T input initial temperature guess, output final solved value.
	@param rho input intital density guess, output final solved value.
	@return 0 on success.
	@param A the first property to solve for, 'p', 'u', 'h' or 's'
	@param B the second property to solver for, options as for A.
	@param atarget the desired value of property A.
	@param btarget the desired value of property B.
*/
int fprops_nonsolver(FPROPS_CHAR A, FPROPS_CHAR B, double atarget, double btarget, double *T, double *rho, const HelmholtzData *D){
	gsl_multiroot_fdfsolver *s;
	int status;
	size_t iter = 0;
	const size_t n = 2;

	Solver2Data U = {A,B,nonsolver_propfn(A), nonsolver_propfn(B), atarget,btarget,D};

	gsl_multiroot_function_fdf f = {&nonsolver_f, &nonsolver_df, &nonsolver_fdf, n, &U};

	/* set initial guesses */
	gsl_vector *x = gsl_vector_alloc(n);
	gsl_vector_set(x, 0, *rho);
	gsl_vector_set(x, 1, *T);

	/* configure GSL solver */
	s = gsl_multiroot_fdfsolver_alloc(gsl_multiroot_fdfsolver_gnewton, n);
	gsl_multiroot_fdfsolver_set(s, &f, x);
#ifdef NONSOLVER_DEBUG
	nonsolver_print_state(iter, s);
#endif

	do{
		iter++;
		status = gsl_multiroot_fdfsolver_iterate(s);
#ifdef NONSOLVER_DEBUG
		nonsolver_print_state(iter, s);
#endif
		if(status){
			/* check if solver is stuck */
			break;
		}
		status = gsl_multiroot_test_residual(s->f, 2e-6);
	} while(status == GSL_CONTINUE && iter < 50);

	*T = gsl_vector_get(x,0);
	*rho = gsl_vector_get(x,1);
#ifdef NONSOLVER_DEBUG
	nonsolver_print_state(iter,s);
#endif
	gsl_multiroot_fdfsolver_free(s);
	gsl_vector_free(x);
	if(status)fprintf(stderr,"%s (%s:%d): %s: ",__func__,__FILE__,__LINE__,gsl_strerror(status));
	return status;
}


