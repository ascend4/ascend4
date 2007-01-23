/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.16 $
 *  Version control file: $RCSfile: safe.c,v $
 *  Date last modified: $Date: 1998/06/11 15:28:35 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1996 Benjamin Andrew Allan, Kenneth Tyner
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

#include <math.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>

#include <utilities/ascPanic.h>


#include "functype.h"
#include "func.h"
#include "safe.h"

#define BIGNUM 1.0e8

/*boolean safe_ok = TRUE;*/
static int safe_print_errors = TRUE;

void safe_error_to_stderr(enum safe_err *not_safe){
  if (!safe_print_errors) {
    return;
  }
  switch(*not_safe)
    {
    case safe_problem:  /* error msg should have already been printed */
    case safe_ok:
      return;
    case safe_div_by_zero:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Division by zero");
    case safe_complex_result:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Complex result");
      break;
    case safe_overflow:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Overflow");
      break;
    case safe_underflow:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Underflow");
      break;
    case safe_range_error:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Function range error");
      break;
    }
}


static double rec_1_p_sqr_Dn(double x,int n, enum safe_err *safe)
/**
 ***  Calculates n-th derivative of 1/(1+x^2).
 **/
{
   /* g[n](x) = -1/(1+x^2) * (n*(n-1)*g[n-2](x) + 2*n*x*g[n-1](x))
      where g[n] := n-th derivative of 1/(1+x^2). */
   int k;
   double prev[3],val;   /* prev[j] := g[k-j](x), val = g[0](x) */

   prev[2] = val = 1.0/(1.0 + safe_sqr_D0(x,safe));
   if( n==0 )
      return(val);
   prev[1] = -2.0*x*val*val;   /* first derivative */
   for( k=2 ; k<=n ; ++k ) {
      prev[0] = -val*(safe_mul_D0((double)(k*(k-1)),prev[2],safe) +
		      safe_mul_D0(2*k*x,prev[1],safe));
      prev[2] = prev[1];
      prev[1] = prev[0];
   }

   return(prev[1]);
}

static double *alloc_poly(int order)
/**
 ***  Allocates a polynominal of given order and returns it.  The
 ***  polynominal need not be freed, but this function should not be
 ***  called again until the old polynominal is not needed anymore.
 **/
{
   static double *poly = NULL;
   static int poly_cap = 0;

   if( order + 1 > poly_cap ) {
      poly_cap = order+1;
      if( poly != NULL ) {
         ascfree( poly );
      }
      poly = ASC_NEW_ARRAY(double,poly_cap );
   }
   return(poly);
}

static double safe_poly(double *poly, int order,
                        double x, double x2, enum safe_err *safe)
/* x2 = x^2 */
/**
 ***  Calculates the value of the given polynomial, where only the
 ***  (order%1 ? odd : even) degree terms are used.
 **/
{
   double val;
   (void)safe;

   for( val=poly[order] ; (order -= 2) >= 0 ; ) {
      val = safe_mul_D0(val,x2,safe) + poly[order];
   }
   return( order==-1 ? safe_mul_D0(val,x,safe) : val );
}

#ifdef HAVE_ERF
static double exp_msqr_Dn(double x,int n,enum safe_err *safe)
/**
 ***  Computes n-th derivative of exp(-x^2).
 **/
{
   /**
    ***  n-th derivative of exp(-x^2) = f[n](x)*exp(-x^2), where f[n] is an
    ***  n-th degree polynominal of definite parity satisfying f[0](x) = 1
    ***  & f[n+1](x) = -2x*f[n](x) + f[n]'(x).
    **/

   double x2 = safe_sqr_D0(x,safe);
   int k,r;
   double *poly;
   poly = alloc_poly(n);

   poly[0] = exp(-x2);
   for( k=0 ; k<n ; ++k ) {   /* Calculate f[k+1] from f[k] */
      poly[k+1] = 0.0;
      for( r=k ; r >= 1 ; r -= 2 )
         poly[r-1] = (double)r * poly[r];
      for( r=k ; r >= 0 ; r -= 2 )
         poly[r+1] += -2.0*poly[r];
   }
   return( safe_poly(poly,n,x,x2,safe) );
}
#endif

static double sqrt_rec_1_m_sqr_Dn(double x,int n,enum safe_err *safe)
/**
 ***  Calculates n-th derivative of (1-x^2)^-.5
 **/
{
   /**
    ***  n-th derivative of (1-x^2)^-.5 = f[n](x) * (1-x^2)^-(n+.5), where
    ***  f[n] is an n-degree polynominal of definite parity satisfying
    ***  f[0] = 1 and f[n+1](x) = f[n]'(x)*(1+x^2) + (2n+1)*x*f[n](x).
    **/

   int k,r;
   double x2;
   double *poly;
   x2 = safe_sqr_D0(x,safe);
   poly = alloc_poly(n);

   (void)safe;

   poly[0] = safe_rec(safe_upower(safe_sqrt_D0(1.0-x2,safe),2*n+1,safe),safe);
   for( k=0 ; k<n ; ++k ) {   /* Calculate f[k+1] from f[k] */
      poly[k+1] = 0.0;
      for( r=k ; r >= 1 ; r -= 2 )
         poly[r-1] = (double)r * poly[r];
      for( r=k ; r >= 0 ; r -= 2 )
         poly[r+1] += (double)(r+2*k+1) * poly[r];
   }
   return( safe_poly(poly,n,x,x2,safe) );
}


double safe_upower(double x, unsigned n, enum safe_err *safe)
{
  double y = 1.0;
  (void)safe;

  for( ; n-- > 0 ; y = safe_mul_D0(y,x,safe) );
  return(y);
}

double safe_factorial(unsigned n, enum safe_err *safe)
{
   double x,y;
   (void)safe;

   for( x = (double)n , y = 1.0 ; x >= 0.5 ; y = safe_mul_D0(y,x--,safe) )
      ;
   return(y);
}

double safe_rec(double x,enum safe_err *safe)
{
   if( x == 0.0 ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Divide by zero in 1/x expr: returning %g",bogus);
      }
      *safe = safe_div_by_zero;
      return( bogus );
   }
   return( 1.0/x );
}

#ifndef INV_CUBRTHUGE
#define INV_CUBRTHUGE      1.7718548704178434e-103
/* smallest cubeable number in an 8 byte ieee double */
#endif
#ifndef CUBRTHUGE
#define CUBRTHUGE      5.6438030941223618e102
/* largest cubeable number in an 8 byte ieee double */
#endif
double safe_cube(register double x,enum safe_err *safe)
{
   if( fabs(x) > CUBRTHUGE || fabs(x) < INV_CUBRTHUGE ) {
      double bogus;
      if ( fabs(x) > CUBRTHUGE) {
        bogus=BIGNUM;
        if( safe_print_errors ) {
           ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Overflow in x^3 expr: returning %g.",bogus);
        }
      } else {
        bogus=0.0;
        if( safe_print_errors ) {
             ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Underflow in x^3 expr: returning %g.",bogus);
        }
      }
      *safe = safe_underflow;
      return( bogus );
   }
   return ( x*x*x );
}

double safe_Dn_rec(double x,int n,enum safe_err *safe)
{
   return( -safe_upower(-safe_rec(x,safe),n+1,safe) * safe_factorial(n,safe) );
}

#ifdef HAVE_ERF
double safe_erf_inv(double x,enum safe_err *safe)
{
#define CONV (1e-7)
   double y,dy,sign;

   sign = (x<0.0) ? -1.0 : 1.0;
   x *= sign;
   if( x >= 1.0 ) {
      double bogus = sign*BIGNUM;
      if( safe_print_errors ) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"inv_erf undefined at %g: returning %g",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }

   y = 0.0;
   do {
      dy = (x-safe_erf_D0(y,safe))*
	  safe_exp_D0(safe_mul_D0(y,y,safe),safe)/safe_ERF_COEF;
      y += dy;
      /**
       ***  Since erf is concave and x >= erf(y0), dy should
       ***  always be positive (?).
       **/
      if( dy < 0.0 && safe_print_errors ) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Found negative slope of %g on erf_inv.",dy);
      }
   } while( safe_ok && dy > CONV );
   return( y * sign );
#undef CONV
}
#endif /* HAVE_ERF */

double safe_lnm_inv(double x,enum safe_err *safe)
{
   register double eps=FuncGetLnmEpsilon();

   if( x > (DBL_MAX > 1.0e308 ? 709.196209 : log(DBL_MAX)) ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
         ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Argument %g too large in lnm_inv: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   /* since lnm(eps)= log(eps), use cheaper one. eps known >0 */
   return( (x >= log(eps)) ? safe_exp_D0(x,safe) : eps*(x+1.0-log(eps)) );
}

int safe_is_int(double x,enum safe_err *safe)
{
#define TOLERANCE (1e-4)
  unsigned n;
  (void)safe;

  if( x<0.0 ) {
    x = -x;
  }

   /**
    ***  Theorem: Define P(n) :<==> x-TOLERANCE <= n <= x+TOLERANCE.  Then
    ***  P(n) for some integer n <==> P(floor(x+TOLERANCE))
    ***                          <==> x-TOLERANCE <= floor(x+TOLERANCE).
    ***
    ***  Proof: Since floor(x+TOLERANCE) <= x+TOLERANCE, the second
    ***  equivalence holds.  The backward direction of the first
    ***  equivalence is a tautology.  If P(n) holds for some n, then
    ***  n <= floor(x+TOLERANCE) <= x+TOLERANCE, the second inequality
    ***  follows since floor(z) <= z, and the first inequality follows
    ***  since floor(z) is the largest integer with that property.
    **/
  if( x-TOLERANCE <= (double)(n = (unsigned)(x+TOLERANCE)) ) {
    return( n&01 );
  } else {
    return( -1 );
  }
#undef TOLERANCE
}

double safe_sin_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return(sin(x));
}

double safe_sinh_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return(sinh(x));
}

double safe_cos_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return(cos(x));
}

double safe_cosh_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return(cosh(x));
}

double safe_tan_D0(double x,enum safe_err *safe)
{
   return( safe_div_D0( sin(x) , cos(x) , safe) );
}

double safe_tanh_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( tanh(x) );
}

double safe_arcsin_D0(double x,enum safe_err *safe)
{
   if( x < -1.0 || 1.0 < x ) {
      double bogus = 0.0;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arcsin_D0: arcsin undefined at %g: Returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( asin(x) );
}

double safe_arcsinh_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( arcsinh(x) );
}

double safe_arccos_D0(double x,enum safe_err *safe)
{
   if( x < -1.0 || 1.0 < x ) {
      double bogus = 0.0;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arccos_D0: arccos undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( acos(x) );
}

double safe_arccosh_D0(double x,enum safe_err *safe)
{
   if( x < 1.0 ) {
      double bogus = 0.0;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arccosh_D0: undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }

   return( arccosh(x) );
}

double safe_arctan_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( atan(x) );
}

double safe_arctanh_D0(double x,enum safe_err *safe)
{
   if( x < -1.0 || 1.0 < x ) {
      double bogus = 0.0;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arctanh_D0: undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( arctanh(x) );
}

#ifdef HAVE_ERF
double safe_erf_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( erf(x) );
}
#endif /* HAVE_ERF */

double safe_exp_D0(double x,enum safe_err *safe)
{
   if( x > (DBL_MAX > 1.0e308 ? 709.196209 : log(DBL_MAX)) ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"exp_D0: Argument %g too large: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( exp(x) );
}

double safe_ln_D0(double x,enum safe_err *safe)
{
   if( x <= 0.0 ) {
      double bogus = -BIGNUM;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"ln_D0: undefined at %g: Returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( log(x) );
}

double safe_lnm_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( lnm(x) );
}

double safe_log10_D0(double x,enum safe_err *safe)
{
   return( safe_LOG10_COEF * safe_ln_D0(x,safe) );
}

double safe_sqr_D0(double x,enum safe_err *safe)
{
  (void)safe;
  return( sqr(x) );
}

double safe_sqrt_D0(double x,enum safe_err *safe)
{
   if( x < 0.0 ) {
      double bogus;
      bogus = -sqrt(-x);
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"sqrt_D0: undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_complex_result;
      return(bogus);
   }
   return(sqrt(x));
}

double safe_cube_D0(double x,enum safe_err *safe)
{
  (void)safe;
   return( cbrt(x) );
}

double safe_cbrt_D0(double x,enum safe_err *safe)
{
  (void)safe;
   return( cbrt(x) );
}

double safe_fabs_D0(double x,enum safe_err *safe)
{
  (void)safe;
   return( fabs(x) );
}

double safe_hold_D0(double x,enum safe_err *safe)
{
  (void)safe;
   return x;
}

double safe_sin_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( cos(x) );
}

double safe_sinh_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( cosh(x) );
}

double safe_cos_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( sin(x) );
}

double safe_cosh_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( sinh(x) );
}

double safe_tan_D1(double x,enum safe_err *safe)
{
   x = cos(x);
   return( safe_rec(safe_sqr_D0(x,safe),safe) );
}

double safe_tanh_D1(double x,enum safe_err *safe)
{
  (void)safe;
  if (x < -200 || x > 200) {
    return 0.0;
  }
  return( dtanh(x) );
}

double safe_arcsin_D1(double x,enum safe_err *safe)
{
   return( safe_rec(safe_sqrt_D0(1.0 - safe_sqr_D0(x,safe),safe),safe) );
}

double safe_arcsinh_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( darcsinh(x) );
}

double safe_arccos_D1(double x,enum safe_err *safe)
{
   return( -safe_arcsin_D1(x,safe) );
}

double safe_arccosh_D1(double x,enum safe_err *safe)
{
   return( safe_rec(safe_sqrt_D0(safe_sqr_D0(x,safe) - 1.0,safe),safe) );
}

double safe_arctan_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( datan(x) );
}

double safe_arctanh_D1(double x,enum safe_err *safe)
{
   return( safe_rec(1.0 - safe_sqr_D0(x,safe),safe) );
}

#ifdef HAVE_ERF
double safe_erf_D1(double x,enum safe_err *safe)
{
   return( safe_ERF_COEF * safe_exp_D0(-safe_sqr_D0(x,safe),safe) );
}
#endif /* HAVE_ERF */

double safe_exp_D1(double x,enum safe_err *safe)
{
   return( safe_exp_D0(x,safe) );
}

double safe_ln_D1(double x,enum safe_err *safe)
{
   return( safe_rec(x,safe) );
}

double safe_lnm_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( dlnm(x) );
}

double safe_log10_D1(double x,enum safe_err *safe)
{
   return( safe_LOG10_COEF * safe_ln_D1(x,safe) );
}

double safe_sqr_D1(double x,enum safe_err *safe)
{
  (void)safe;
   return( dsqr(x) );
}

double safe_sqrt_D1(double x,enum safe_err *safe)
{
   return( 0.5 * safe_rec(safe_sqrt_D0(x,safe),safe) );
}

double safe_cube_D1(double x,enum safe_err *safe)
{
   return( 3 * safe_sqr_D0(x,safe) );
}

double safe_cbrt_D1(double x,enum safe_err *safe)
{
   return( 0.3333333333333333*
	  safe_rec(safe_sqr_D0(safe_cbrt_D0(x,safe),safe),safe) );
}

double safe_fabs_D1(double x,enum safe_err *safe)
{
  (void)safe;
  (void)x;  /* stop gcc whine about unused parameter */
  return( 1.0 );
}

double safe_hold_D1(double x,enum safe_err *safe)
{
  (void)x;  /* stop gcc whine about unused parameter */
  (void)safe;
  return( 0.0 );
}

double safe_sin_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( dcos(x) );
}

double safe_sinh_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( sinh(x) );
}

double safe_cos_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( dcos2(x) );
}

double safe_cosh_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( cosh(x) );
}

double safe_tan_D2(double x,enum safe_err *safe)
{
   if( fabs(cos(x)) == 0.0 ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
         ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"tan_D2: Infinite at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return dtan2(x);
}

double safe_tanh_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( dtanh2(x) );
}

double safe_arcsin_D2(double x,enum safe_err *safe)
{
   register double t;
   t = safe_rec(1.0 - safe_sqr_D0(x,safe),safe);
   return( safe_mul_D0( safe_mul_D0(x,t,safe) , safe_mul_D0(t,t,safe) ,safe) );
}

double safe_arcsinh_D2(double x,enum safe_err *safe)
{
  (void)safe;
  return( darcsinh2(x) );
}

double safe_arccos_D2(double x,enum safe_err *safe)
{
   return( -safe_arcsin_D2(x,safe) );
}

double safe_arccosh_D2(double x,enum safe_err *safe)
{
   if( fabs(x) <= 1.0 ) {
      double bogus = -BIGNUM;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arccosh_D2: Undefined at %g: returning %g",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return darccosh2(x);
}

double safe_arctan_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( datan2(x) );
}

double safe_arctanh_D2(double x,enum safe_err *safe)
{
   if( fabs(x) == 1.0 ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"arctanh_D2: undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return( darctanh2(x) );
}

#ifdef HAVE_ERF
double safe_erf_D2(double x,enum safe_err *safe)
{
   return( -ldexp(safe_ERF_COEF *
		  safe_mul_D0(x,safe_exp_D0(-safe_sqr_D0(x,safe),safe),safe),1) );
}
#endif /* HAVE_ERF */

double safe_exp_D2(double x,enum safe_err *safe)
{
   return( safe_exp_D0(x,safe) );
}

double safe_ln_D2(double x,enum safe_err *safe)
{
   return( -safe_rec(safe_sqr_D0(x,safe),safe) );
}

double safe_lnm_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( dlnm2(x) );
}

double safe_log10_D2(double x,enum safe_err *safe)
{
   return( safe_LOG10_COEF * safe_ln_D2(x,safe) );
}

double safe_sqr_D2(double x,enum safe_err *safe)
{
  (void)safe;
   return( dsqr2(x) );
}

double safe_sqrt_D2(double x,enum safe_err *safe)
{
   return(-ldexp(safe_rec(safe_mul_D0(x,safe_sqrt_D0( x,safe),safe),safe),-2));
}

double safe_cube_D2(double x,enum safe_err *safe)
{
   (void)safe;
   return( safe_mul_D0(6,x,safe) );
}

double safe_cbrt_D2(double x,enum safe_err *safe)
{
   if( fabs(x) == 0.0 ) {
      double bogus = BIGNUM;
      if( safe_print_errors ) {
	 ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"cbrt_D2: undefined at %g: returning %g.",x,bogus);
      }
      *safe = safe_range_error;
      return(bogus);
   }
   return dcbrt2(x);
}

double safe_fabs_D2(double x,enum safe_err *safe)
{
  (void)x;  /* stop gcc whine about unused parameter */
  (void)safe;
  return( 0.0 );
}

double safe_arcsin_Dn(double x,int n,enum safe_err *safe)
{
   return( n==0 ? safe_arcsin_D0(x,safe) : sqrt_rec_1_m_sqr_Dn(x,n-1,safe) );
}

double safe_arccos_Dn(double x,int n,enum safe_err *safe)
{
   return( n==0 ? safe_arccos_D0(x,safe) : -sqrt_rec_1_m_sqr_Dn(x,n-1,safe) );
}

double safe_arctan_Dn(double x,int n,enum safe_err *safe)
{
   return( n==0 ? safe_arctan_D0(x,safe) : rec_1_p_sqr_Dn(x,n-1,safe) );
}

double safe_cos_Dn(double x,int n,enum safe_err *safe)
{
   switch( n%4 ) {
      case 0:
         return( cos(x) );
      case 1:
         return( -sin(x) );
      case 2:
         return( -cos(x) );
      case 3:
         return( sin(x) );
   }
   if( safe_print_errors ) {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"cos_D: Unreachable point reached?!?");
   }
   *safe = safe_range_error;
   return 0.0;
}

double safe_sin_Dn(double x,int n,enum safe_err *safe)
{
   return( safe_cos_Dn(x,n+3,safe) );
}

#ifdef HAVE_ERF
double safe_erf_Dn(double x,int n,enum safe_err *safe)
{
   return( (n==0)
           ? (safe_erf_D0(x,safe))
           : (safe_ERF_COEF*exp_msqr_Dn(x,n-1,safe)) );
}
#endif /* HAVE_ERF */

double safe_exp_Dn(double x,int n,enum safe_err *safe)
{
  (void)n;  /* stop gcc whine about unused parameter */
  return( safe_exp_D0(x,safe) );
}

double safe_ln_Dn(double x,int n,enum safe_err *safe)
{
   return( n==0 ? safe_ln_D0(x,safe) : safe_Dn_rec(x,n-1,safe) );
}

double safe_log10_Dn(double x,int n,enum safe_err *safe)
{
   return( safe_LOG10_COEF * safe_ln_Dn(x,n,safe) );
}

double safe_sqr_Dn(double x,int n,enum safe_err *safe)
{
   switch(n) {
      case 0:
         return( safe_sqr_D0(x,safe) );
      case 1:
         return( 2.0*x );
      case 2:
         return(2.0);
      default:
         return(0.0);
   }
}

double safe_sqrt_Dn(double x,int n,enum safe_err *safe)
{
   double a,b;

   for( b = 1.0 , a = 1.5-n ; a < 1.0 ; ++a )
      b *= a;
   return( b * safe_sqrt_D0(x,safe) * safe_rec(safe_upower(x,n,safe),safe) );
}

static double tan_k_Dn(double u,int k,int n,enum safe_err *safe)
/* k >= 0 */
/**
 ***  nth derivative of tan^k evaluated at x, where u=tan(x)
 **/
{
   /* (tan^k)' = k * (tan^(k+1) + tan^(k-1)) */
   if( n == 0 )
      return( safe_upower(u,k,safe) );
   else if( k == 0 )
      return( 0.0 );
   else
      return( k * (tan_k_Dn(u,k+1,n-1,safe) + tan_k_Dn(u,k-1,n-1,safe)) );
}

double safe_tan_Dn(double x,int n,enum safe_err *safe)
{
   return( tan_k_Dn(safe_tan_D0(x,safe),1,n,safe) );
}

static double cheap_pow(double x,double y,enum safe_err *safe)
/**
 ***  Computes x^y, x>0
 **/
{
   return( safe_exp_D0(safe_mul_D0(y,safe_ln_D0(x,safe),safe),safe) );
}

double safe_pow_D0(double x,double y,enum safe_err *safe)
{
   if( x > 0.0 )
      return( cheap_pow(x,y,safe) );
   else if( x == 0.0 ) {
      if( y <= 0.0 ) {
	 double bogus;
	 bogus = y < 0.0 ? BIGNUM : 0.0;
	 if( safe_print_errors ) {
	    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"pow_D0: %g raised to power %g undefined: returning %g",x,y,bogus);
	 }
	 *safe = safe_range_error;
	 return(bogus);
      }
      return( 0.0 );
   }

   /* x < 0.0 */
   switch( safe_is_int(y,safe) ) {
      case 0:
         return( cheap_pow(-x,y,safe) );
      case 1:
         return( -cheap_pow(-x,y,safe) );
      default: {
	 double bogus = cheap_pow(-x,y,safe);
	 if( safe_print_errors ) {
	    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"pow_D0: %g raised to power %g undefined: returning %g",x,y,bogus);

	 }
	 *safe = safe_range_error;
	 return(bogus);
      }
   }
}

double safe_div_D1(double x,double y,int wrt,enum safe_err *safe)
{
  y = safe_rec(y,safe);
  switch( wrt ) {
  case 0:
    return(y);
  case 1:
    return( -safe_mul_D0(safe_sqr_D0(y,safe),x,safe) );
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_ipow_D1( double x,double y, int wrt,enum safe_err *safe)
{
  (void)safe;

  switch( wrt ) {
  case 0:
    return( safe_mul_D0(y,safe_ipow_D0(x,y-1,safe),safe) );
  case 1:
    return(0.0); /* d(x^i)/di where i is integer is 0.0 since we
                  * assume integers are constant */
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_pow_D1(double x,double y,int wrt,enum safe_err *safe)
{
  switch( wrt ) {
  case 0:
    return( safe_mul_D0(y,safe_pow_D0(x,y-1.0,safe),safe) );
  case 1:
    return( safe_mul_D0( safe_ln_D0(x,safe) , safe_pow_D0(x,y,safe) ,safe) );
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_div_D2(double x,double y,int wrt1,int wrt2,enum safe_err *safe)
{
  (void)x;  /* stop gcc whine about unused parameter */
  switch( wrt1+wrt2 ) {
  case 0:
    return(0.0);
  case 1:
    return( -safe_rec(safe_sqr_D0(y,safe),safe) );
  case 2:
    return( safe_rec(0.5*safe_mul_D0(y,safe_sqr_D0(y,safe),safe),safe) );
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_ipow_D2( double x,double y, int wrt1,int wrt2,enum safe_err *safe)
{
  double lnx;
  int yint;
  yint = ascnint(y);
  switch( wrt1+wrt2 ) {
  case 0:
    return yint*(yint-1)*safe_ipow_D0(x,y-2,safe);
  case 1:
    lnx = safe_ln_D0(x,safe);
    return(
           safe_mul_D0(safe_rec(x,safe)+
		       safe_sqr_D0(lnx,safe),safe_ipow_D0(x,y,safe),safe));
  case 2:
    lnx = safe_ln_D0(x,safe);
    return( safe_mul_D0(safe_sqr_D0(lnx,safe) ,
                        safe_ipow_D0(x,y,safe),safe) );
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_pow_D2(double x,double y,int wrt1,int wrt2,enum safe_err *safe)
{
  double lnx;
  switch( wrt1+wrt2 ) {
  case 0:
    return( safe_mul_D0( safe_mul_D0(y,y-1.0,safe),
                         safe_pow_D0(x,y-2.0,safe),safe ) );
  case 1:
    lnx = safe_ln_D0(x,safe);
    return( safe_mul_D0(safe_rec(x,safe)+safe_sqr_D0(lnx,safe),
                        safe_pow_D0(x,y,safe),safe) );
  case 2:
    lnx = safe_ln_D0(x,safe);
    return( safe_mul_D0(safe_sqr_D0(lnx,safe) ,
                        safe_pow_D0(x,y,safe),safe) );
  default:
    ASC_PANIC("'wrt' out of range!");
    
  }
}

double safe_add_Dn(double x,double y,int nwrt0,int nwrt1,enum safe_err *safe)
{
  (void)safe;
  switch( nwrt0+nwrt1 ) {
  case 0:
    return( x+y );
  case 1:
    return( 1.0 );
  default:
    return( 0.0 );
  }
}

double safe_sub_Dn(double x,double y,int nwrt0,int nwrt1,enum safe_err *safe)
{
  (void)safe;
   switch( nwrt0+nwrt1 ) {
      case 0:
         return( x-y );
      case 1:
         return( nwrt1==0 ? 1.0 : -1.0 );
      default:
         return( 0.0 );
   }
}

double safe_mul_Dn(double x,double y,int nwrt0,int nwrt1,enum safe_err *safe)
{
   (void)safe;

   switch( nwrt0 ) {
      case 0:
         break;
      case 1:
         x = 1.0;
         break;
      default:
         return(0.0);
   }

   switch( nwrt1 ) {
      case 0:
         return( safe_mul_D0(x,y,safe) );
      case 1:
         return(x);
      default:
         return(0.0);
   }
}

double safe_div_Dn(double x,double y,int nwrt0,int nwrt1,enum safe_err *safe)
{
   switch( nwrt0 ) {
      case 1:
         x = 1.0;
      case 0:   /* FALL THROUGH */
         return( safe_mul_D0(x,safe_Dn_rec(y,nwrt1,safe),safe) );
      default:
         return(0.0);
   }
}

double safe_pow_Dn(double x,double y,int nwrt0,int nwrt1,enum safe_err *safe)
/**
 ***  The n-th derivative wrt x of (lnx)^m * x^y (the m-th derivative wrt y
 ***  of x^y) = x^(y-n) * P[n](lnx), where P[0](z) = z^m and
 ***  P[n+1](z) = (y-n)*P[n](z) + P[n]'(z).  (n==nwrt0,m==nwrt1)
 **/
{
   double *poly;
   double lnx,lnx2;   /* To be calculated later if necessary */
   int k,r;
   poly = alloc_poly(nwrt1);

/*   mem_zero_byte_cast(poly,0,nwrt1*sizeof(double));*/
   ascbzero((void *)poly,nwrt1*sizeof(double));
   poly[nwrt1] = safe_pow_D0(x,y-(double)nwrt0,safe);
   for( k=0 ; k<nwrt0 ; ++k ) {   /* calculate P[k+1] from P[k] */
      for( r=0 ; r<nwrt1 ; ++r )
         poly[r] = (y-k)*poly[r] + (r+1)*poly[r+1];
      poly[nwrt1] *= y-k;
   }

   if( nwrt1 == 0 )
      return( poly[0] );   /* polynominal is just a constant */

   lnx2 = safe_sqr_D0( lnx = safe_ln_D0(x,safe),safe );
   return( safe_poly(poly,nwrt1,lnx,lnx2,safe)+
	  safe_poly(poly,nwrt1-1,lnx,lnx2,safe) );
}
