/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: calc.c,v $
 *  Date last modified: $Date: 1998/02/05 15:59:18 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/mem.h"
#include "compiler/fractions.h"
#include "utilities/ascPanic.h"
#include "compiler/functype.h"
#include "compiler/dimen.h"
#include "compiler/func.h"
#include "solver/calc.h"

#define BIGNUM 1.0e8

int32 calc_ok = TRUE;
boolean calc_print_errors = TRUE;

static real64 rec_1_p_sqr_Dn(x,n)
real64 x;
int n;
/**
 ***  Calculates n-th derivative of 1/(1+x^2).
 **/
{
   /* g[n](x) = -1/(1+x^2) * (n*(n-1)*g[n-2](x) + 2*n*x*g[n-1](x))
      where g[n] := n-th derivative of 1/(1+x^2). */
   int k;
   real64 prev[3],val;   /* prev[j] := g[k-j](x), val = g[0](x) */

   prev[2] = val = 1.0/(1.0 + calc_sqr_D0(x));
   if( n==0 )
      return(val);
   prev[1] = -2.0*x*val*val;   /* first derivative */
   for( k=2 ; k<=n ; ++k ) {
      prev[0] = -val*(calc_mul_D0((double)(k*(k-1)),prev[2]) +
		      calc_mul_D0(2*k*x,prev[1]));
      prev[2] = prev[1];
      prev[1] = prev[0];
   }

   return(prev[1]);
}

static real64 *alloc_poly(order)
int order;
/**
 ***  Allocates a polynominal of given order and returns it.  The
 ***  polynominal need not be freed, but this function should not be
 ***  called again until the old polynominal is not needed anymore.
 **/
{
   static real64 *poly = NULL;
   static int poly_cap = 0;

   if( order + 1 > poly_cap ) {
      poly_cap = order+1;
      if( poly != NULL )
         ascfree( (POINTER)poly );
      poly = (real64 *)ascmalloc( poly_cap * sizeof(real64) );
   }
   return(poly);
}

static real64 calc_poly(poly,order,x,x2)
real64 *poly;
int order;
real64 x,x2;   /* x2 = x^2 */
/**
 ***  Calculates the value of the given polynomial, where only the
 ***  (order%1 ? odd : even) degree terms are used.
 **/
{
   real64 val;

   for( val=poly[order] ; (order -= 2) >= 0 ; )
      val = calc_mul_D0(val,x2) + poly[order];
   return( order==-1 ? calc_mul_D0(val,x) : val );
}

static real64 exp_msqr_Dn(x,n)
real64 x;
int n;
/**
 ***  Computes n-th derivative of exp(-x^2).
 **/
{
   /**
    ***  n-th derivative of exp(-x^2) = f[n](x)*exp(-x^2), where f[n] is an
    ***  n-th degree polynominal of definite parity satisfying f[0](x) = 1
    ***  & f[n+1](x) = -2x*f[n](x) + f[n]'(x).
    **/

   real64 x2 = calc_sqr_D0(x);
   int k,r;
   real64 *poly;
   poly = alloc_poly(n);

   poly[0] = exp(-x2);
   for( k=0 ; k<n ; ++k ) {   /* Calculate f[k+1] from f[k] */
      poly[k+1] = 0.0;
      for( r=k ; r >= 1 ; r -= 2 )
         poly[r-1] = (double)r * poly[r];
      for( r=k ; r >= 0 ; r -= 2 )
         poly[r+1] += -2.0*poly[r];
   }
   return( calc_poly(poly,n,x,x2) );
}

static real64 sqrt_rec_1_m_sqr_Dn(x,n)
real64 x;
int n;
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
   real64 x2;
   real64 *poly;
   x2 = calc_sqr_D0(x);
   poly = alloc_poly(n);

   poly[0] = calc_rec(calc_upower(calc_sqrt_D0(1.0-x2),2*n+1));
   for( k=0 ; k<n ; ++k ) {   /* Calculate f[k+1] from f[k] */
      poly[k+1] = 0.0;
      for( r=k ; r >= 1 ; r -= 2 )
         poly[r-1] = (double)r * poly[r];
      for( r=k ; r >= 0 ; r -= 2 )
         poly[r+1] += (double)(r+2*k+1) * poly[r];
   }
   return( calc_poly(poly,n,x,x2) );
}


real64 calc_upower(real64 x, unsigned n)
{
   double y = 1.0;
   for( ; n-- > 0 ; y = calc_mul_D0(y,x) )
      ;
   return(y);
}

real64 calc_factorial(unsigned n)
{
   double x,y;
   for( x = (double)n , y = 1.0 ; x >= 0.5 ; y = calc_mul_D0(y,x--) )
      ;
   return(y);
}

real64 calc_rec(real64 x)
{
   if( x == 0.0 ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_rec\n");
	 FPRINTF(stderr,"        Divide by zero undefined.\n");
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
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
real64 calc_cube(x)
register real64 x;
{
   if( fabs(x) > CUBRTHUGE || fabs(x) < INV_CUBRTHUGE ) {
      real64 bogus;
      if ( fabs(x) > CUBRTHUGE) {
        bogus=BIGNUM;
        if( calc_print_errors ) {
           FPRINTF(stderr,"ERROR:  (calc) calc_cube\n");
           FPRINTF(stderr,"        Overflow calculation requested \n");
           FPRINTF(stderr,"        Returning %g.\n",bogus);
        }
      } else {
        bogus=0.0;
        if( calc_print_errors ) {
             FPRINTF(stderr,"ERROR:  (calc) calc_cube\n");
             FPRINTF(stderr,"        Underflow calculation requested \n");
             FPRINTF(stderr,"        Returning %g.\n",bogus);
        }
      }
      calc_ok = FALSE;
      return( bogus );
   }
   return ( x*x*x );
}

real64 calc_Dn_rec(x,n)
real64 x;
int n;
{
   return( -calc_upower(-calc_rec(x),n+1) * calc_factorial(n) );
}

#ifdef HAVE_ERF
real64 calc_erf_inv(x)
real64 x;
{
#define CONV (1e-7)
   real64 y,dy,sign;

   sign = (x<0.0) ? -1.0 : 1.0;
   x *= sign;
   if( x >= 1.0 ) {
      real64 bogus = sign*BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_erf_inv\n");
	 FPRINTF(stderr,"        Inverse erf is undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }

   y = 0.0;
   do {
      dy = (x-calc_erf_D0(y))*calc_exp_D0(calc_mul_D0(y,y))/calc_ERF_COEF;
      y += dy;
      /**
       ***  Since erf is concave and x >= erf(y0), dy should
       ***  always be positive (?).
       **/
      if( dy < 0.0 && calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_erf_inv\n");
	 FPRINTF(stderr,"        Found negative slope of %g on erf.\n",dy);
      }
   } while( calc_ok && dy > CONV );
   return( y * sign );
#undef CONV
}
#endif /* HAVE_ERF */

real64 calc_lnm_inv(x)
real64 x;
{
   register double eps=FuncGetLnmEpsilon();
   if( x > (MAXDOUBLE > 1.0e308 ? 709.196209 : log(MAXDOUBLE)) ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
         FPRINTF(stderr,"ERROR:  (calc) calc_lnm_inv\n");
         FPRINTF(stderr,"        Argument %g too large.\n",x);
         FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   /* since lnm(eps)= log(eps), use cheaper one. eps known >0 */
   return( (x >= log(eps)) ? calc_exp_D0(x) : eps*(x+1.0-log(eps)) );
}

int calc_is_int(x)
real64 x;
{
#define TOLERANCE (1e-4)
   unsigned n;

   if( x<0.0 )
      x = -x;

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
   if( x-TOLERANCE <= (double)(n = (unsigned)(x+TOLERANCE)) )
      return( n&01 );
   else
      return( -1 );
#undef TOLERANCE
}


real64 calc_tan_D0(x)
real64 x;
{
   return( calc_div_D0( sin(x) , cos(x) ) );
}

real64 calc_arcsin_D0(x)
real64 x;
{
   if( x < -1.0 || 1.0 < x ) {
      real64 bogus = 0.0;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arcsin_D0\n");
	 FPRINTF(stderr,"        Function arcsin is undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( asin(x) );
}

real64 calc_arccos_D0(x)
real64 x;
{
   if( x < -1.0 || 1.0 < x ) {
      real64 bogus = 0.0;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arccos_D0\n");
	 FPRINTF(stderr,"        Function arccos is undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( acos(x) );
}

real64 calc_arccosh_D0(x)
real64 x;
{
   if( x < 1.0 ) {
      real64 bogus = 0.0;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arccosh_D0\n");
	 FPRINTF(stderr,"        Function arccosh is undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }

   return( arccosh(x) );
}

real64 calc_arctanh_D0(x)
real64 x;
{
   if( x < -1.0 || 1.0 < x ) {
      real64 bogus = 0.0;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arctanh_D0\n");
	 FPRINTF(stderr,"        Function arctanh is undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( arctanh(x) );
}

real64 calc_exp_D0(x)
real64 x;
{
   if( x > (MAXDOUBLE > 1.0e308 ? 709.196209 : log(MAXDOUBLE)) ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_exp_D0\n");
	 FPRINTF(stderr,"        Argument %g too large.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( exp(x) );
}

real64 calc_ln_D0(x)
real64 x;
{
   if( x <= 0.0 ) {
      real64 bogus = -BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_ln_D0\n");
	 FPRINTF(stderr,"        Natural log undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( log(x) );
}

real64 calc_log_D0(x)
real64 x;
{
   return( calc_LOG10_COEF * calc_ln_D0(x) );
}

real64 calc_sqrt_D0(x)
real64 x;
{
   if( x < 0.0 ) {
      real64 bogus;
      bogus = -sqrt(-x);
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_sqrt_D0\n");
	 FPRINTF(stderr,"        Square root undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return(sqrt(x));
}

real64 calc_tan_D1(x)
register real64 x;
{
   x=cos(x);
   return( calc_rec(calc_sqr_D0(x)) );
}

real64 calc_arcsin_D1(x)
real64 x;
{
   return( calc_rec(calc_sqrt_D0(1.0 - calc_sqr_D0(x))) );
}

real64 calc_arccos_D1(x)
real64 x;
{
   return( -calc_arcsin_D1(x) );
}

real64 calc_arccosh_D1(x)
real64 x;
{
   return( calc_rec(calc_sqrt_D0(calc_sqr_D0(x) - 1.0)) );
}

real64 calc_arctanh_D1(x)
real64 x;
{
   return( calc_rec(1.0 - calc_sqr_D0(x)) );
}

#ifdef HAVE_ERF
real64 calc_erf_D1(x)
real64 x;
{
   return( calc_ERF_COEF * calc_exp_D0(-calc_sqr_D0(x)) );
}
#endif /* HAVE_ERF */

real64 calc_log_D1(x)
real64 x;
{
   return( calc_LOG10_COEF * calc_ln_D1(x) );
}

real64 calc_sqrt_D1(x)
real64 x;
{
   return( 0.5 * calc_rec(calc_sqrt_D0(x)) );
}

real64 calc_cbrt_D1(x)
real64 x;
{
   return( 0.3333333333333333*calc_rec(calc_sqr_D0(calc_cbrt_D0(x))) );
}
/* KHACK */
real64 calc_fabs_D1(x)
real64 x;
{
   (void)x;
   return( 1.0 );
}

real64 calc_tan_D2(x)
real64 x;
{
   if( fabs(cos(x)) == 0.0 ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
         FPRINTF(stderr,"ERROR:  (calc) calc_tan_D2\n");
         FPRINTF(stderr,"        Tan derivative infinite at %g.\n",x);
         FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return dtan2(x);
}

real64 calc_arcsin_D2(x)
real64 x;
{
   register double t;
   t = calc_rec(1.0 - calc_sqr_D0(x));
   return( calc_mul_D0( calc_mul_D0(x,t) , calc_mul_D0(t,t) ) );
}

real64 calc_arccos_D2(x)
real64 x;
{
   return( -calc_arcsin_D2(x) );
}

real64 calc_arccosh_D2(x)
real64 x;
{
   if( fabs(x) <= 1.0 ) {
      real64 bogus = -BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arccosh_D2\n");
	 FPRINTF(stderr,"        Arccosh undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return darccosh2(x);
}

real64 calc_arctanh_D2(x)
real64 x;
{
   if( fabs(x) == 1.0 ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_arctanh_D2\n");
	 FPRINTF(stderr,"        Arctanh undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return( darctanh2(x) );
}

#ifdef HAVE_ERF
real64 calc_erf_D2(x)
real64 x;
{
   return( -ldexp(calc_ERF_COEF * calc_mul_D0(x,calc_exp_D0(-calc_sqr_D0(x))),
                  1) );
}
#endif /* HAVE_ERF */

real64 calc_ln_D2(x)
real64 x;
{
   return( -calc_rec(calc_sqr_D0(x)) );
}

real64 calc_log_D2(x)
real64 x;
{
   return( calc_LOG10_COEF * calc_ln_D2(x) );
}

real64 calc_sqrt_D2(x)
real64 x;
{
   return( -ldexp( calc_rec( calc_mul_D0(x,calc_sqrt_D0(x))), -2) );
}

real64 calc_cbrt_D2(x)
real64 x;
{
   if( fabs(x) == 0.0 ) {
      real64 bogus = BIGNUM;
      if( calc_print_errors ) {
	 FPRINTF(stderr,"ERROR:  (calc) calc_cbrt_D2\n");
	 FPRINTF(stderr,"        Cbrt undefined at %g.\n",x);
	 FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
   }
   return dcbrt2(x);
}
/* KHACK */
real64 calc_fabs_D2(x)
real64 x;
{
   (void)x;
   return( 0.0 );
}

real64 calc_arcsin_Dn(x,n)
real64 x;
int n;
{
   return( n==0 ? calc_arcsin_D0(x) : sqrt_rec_1_m_sqr_Dn(x,n-1) );
}

real64 calc_arccos_Dn(x,n)
real64 x;
int n;
{
   return( n==0 ? calc_arccos_D0(x) : -sqrt_rec_1_m_sqr_Dn(x,n-1) );
}

real64 calc_arctan_Dn(x,n)
real64 x;
int n;
{
   return( n==0 ? calc_arctan_D0(x) : rec_1_p_sqr_Dn(x,n-1) );
}

real64 calc_cos_Dn(x,n)
real64 x;
int n;
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
   if( calc_print_errors ) {
      FPRINTF(stderr,"ERROR:  (calc) calc_cos_Dn\n");
      FPRINTF(stderr,"        Unreachable point reached.\n");
   }
   calc_ok = FALSE;
   return 0.0;
}

real64 calc_sin_Dn(x,n)
real64 x;
int n;
{
   return( calc_cos_Dn(x,n+3) );
}

#ifdef HAVE_ERF
real64 calc_erf_Dn(x,n)
real64 x;
int n;
{
   return( n==0 ? calc_erf_D0(x) : calc_ERF_COEF*exp_msqr_Dn(x,n-1) );
}
#endif /* HAVE_ERF */

real64 calc_exp_Dn(x,n)
real64 x;
int n;
{
   (void)n;
   return( calc_exp_D0(x) );
}

real64 calc_ln_Dn(x,n)
real64 x;
int n;
{
   return( n==0 ? calc_ln_D0(x) : calc_Dn_rec(x,n-1) );
}

real64 calc_log_Dn(x,n)
real64 x;
int n;
{
   return( calc_LOG10_COEF * calc_ln_Dn(x,n) );
}

real64 calc_sqr_Dn(x,n)
real64 x;
int n;
{
   switch(n) {
      case 0:
         return( calc_sqr_D0(x) );
      case 1:
         return( 2.0*x );
      case 2:
         return(2.0);
      default:
         return(0.0);
   }
}

real64 calc_sqrt_Dn(x,n)
real64 x;
int n;
{
   double a,b;

   for( b = 1.0 , a = 1.5-n ; a < 1.0 ; ++a )
      b *= a;
   return( b * calc_sqrt_D0(x) * calc_rec(calc_upower(x,n)) );
}

/*
 *  nth derivative of tan^k evaluated at x, where u=tan(x)
 *      k >= 0
 */
static double tan_k_Dn(double u, int k, int n)
{
   /* (tan^k)' = k * (tan^(k+1) + tan^(k-1)) */
   if( n == 0 )
      return( calc_upower(u,k) );
   else if( k == 0 )
      return( 0.0 );
   else
      return( k * (tan_k_Dn(u,k+1,n-1) + tan_k_Dn(u,k-1,n-1)) );
}

extern real64 calc_tan_Dn(real64 x, int n)
{
  return( tan_k_Dn(calc_tan_D0(x),1,n) );
}

/*
 *  Computes x^y, x>0
 */
static double cheap_pow(double x, double y)
{
  return( calc_exp_D0(calc_mul_D0(y,calc_ln_D0(x))) );
}

extern real64 calc_pow_D0(real64 x, real64 y)
{
  if( x > 0.0 ) {
    return( cheap_pow(x,y) );
  }
  else if( x == 0.0 ) {
    if( y <= 0.0 ) {
      real64 bogus;
      bogus = y < 0.0 ? BIGNUM : 0.0;
      if( calc_print_errors ) {
        FPRINTF(stderr,"ERROR:  (calc) calc_pow_D0\n");
        FPRINTF(stderr,"        %g raised to %g power undefined.\n",x,y);
        FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
    }
    return( 0.0 );
  }

  /* x < 0.0 */
  switch( calc_is_int(y) ) {
  case 0:
    return( cheap_pow(-x,y) );
  case 1:
    return( -cheap_pow(-x,y) );
  default: {
      real64 bogus = cheap_pow(-x,y);
      if( calc_print_errors ) {
        FPRINTF(stderr,"ERROR:  (calc) calc_pow_D0\n");
        FPRINTF(stderr,"        %g raised to %g power undefined.\n",x,y);
        FPRINTF(stderr,"        Returning %g.\n",bogus);
      }
      calc_ok = FALSE;
      return(bogus);
    }
  }
}

extern real64 calc_div_D1(real64 x, real64 y, int wrt)
{
  y = calc_rec(y);
  switch( wrt ) {
  case 0:
    return(y);
  case 1:
    return( -calc_mul_D0(calc_sqr_D0(y),x) );
  default:
    Asc_Panic(2, "calc_div_D1",
              "calc_div_D1: Passed %d as value for \'wrt\'", wrt);
    exit(2);/* Needed to keep gcc from whining */
  }
}

extern real64 calc_pow_D1(real64 x, real64 y, int wrt)
{
  switch( wrt ) {
  case 0:
    return( calc_mul_D0(y,calc_pow_D0(x,y-1.0)) );
  case 1:
    return( calc_mul_D0( calc_ln_D0(x) , calc_pow_D0(x,y) ) );
  default:
    Asc_Panic(2, "calc_pow_D1",
              "calc_pow_D1: Passed %d as value for \'wrt\'", wrt);
    exit(2);/* Needed to keep gcc from whining */
  }
}

extern real64 calc_div_D2(real64 x, real64 y, int wrt1, int wrt2)
{
  (void)x;
  switch( wrt1+wrt2 ) {
  case 0:
    return(0.0);
  case 1:
    return( -calc_rec(calc_sqr_D0(y)) );
  case 2:
    return( calc_rec(0.5*calc_mul_D0(y,calc_sqr_D0(y))) );
  default:
    Asc_Panic(2, "calc_div_D2",
              "calc_div_D2: Passed bad values for \'wrt1\' and \'wrt2\'");
    exit(2);/* Needed to keep gcc from whining */
  }
}

real64 calc_pow_D2(real64 x, real64 y, int wrt1, int wrt2)
{
  double lnx;
  switch( wrt1+wrt2 ) {
  case 0:
    return( calc_mul_D0( calc_mul_D0(y,y-1.0),calc_pow_D0(x,y-2.0) ) );
  case 1:
    lnx = calc_ln_D0(x);
    return( calc_mul_D0(calc_rec(x)+calc_sqr_D0(lnx), calc_pow_D0(x,y)) );
  case 2:
    lnx = calc_ln_D0(x);
    return( calc_mul_D0(calc_sqr_D0(lnx) , calc_pow_D0(x,y)) );
  default:
    Asc_Panic(2, "calc_pow_D2",
              "calc_pow_D2: Passed bad values for \'wrt1\' and \'wrt2\'");
    exit(2);/* Needed to keep gcc from whining */
  }
}

real64 calc_add_Dn(real64 x, real64 y, int nwrt0, int nwrt1)
{
  switch( nwrt0+nwrt1 ) {
  case 0:
    return( x+y );
  case 1:
    return( 1.0 );
  default:
    return( 0.0 );
  }
}

real64 calc_sub_Dn(real64 x, real64 y, int nwrt0, int nwrt1)
{
  switch( nwrt0+nwrt1 ) {
  case 0:
    return( x-y );
  case 1:
    return( nwrt1==0 ? 1.0 : -1.0 );
  default:
    return( 0.0 );
  }
}

real64 calc_mul_Dn(real64 x, real64 y, int nwrt0, int nwrt1)
{
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
    return( calc_mul_D0(x,y) );
  case 1:
    return(x);
  default:
    return(0.0);
  }
}

real64 calc_div_Dn(real64 x, real64 y, int nwrt0, int nwrt1)
{
  switch( nwrt0 ) {
  case 1:
    x = 1.0;
    /*** FALL THROUGH ***/
  case 0:
    return( calc_mul_D0(x,calc_Dn_rec(y,nwrt1)) );
  default:
    return(0.0);
  }
}


/*
 *  The n-th derivative wrt x of (lnx)^m * x^y (the m-th derivative wrt y
 *  of x^y) = x^(y-n) * P[n](lnx), where P[0](z) = z^m and
 *  P[n+1](z) = (y-n)*P[n](z) + P[n]'(z).  (n==nwrt0,m==nwrt1)
 */
real64 calc_pow_Dn(real64 x, real64 y, int nwrt0, int nwrt1)
{
  real64 *poly;
  real64 lnx,lnx2;   /* To be calculated later if necessary */
  int k,r;
  poly = alloc_poly(nwrt1);

  mem_zero_byte_cast(poly,0,nwrt1*sizeof(real64));
  poly[nwrt1] = calc_pow_D0(x,y-(double)nwrt0);
  for( k=0 ; k<nwrt0 ; ++k ) {
    /* calculate P[k+1] from P[k] */
    for( r=0 ; r<nwrt1 ; ++r ) {
      poly[r] = (y-k)*poly[r] + (r+1)*poly[r+1];
    }
    poly[nwrt1] *= y-k;
  }

  if( nwrt1 == 0 ) {
    /* polynominal is just a constant */
    return( poly[0] );
  }

  lnx2 = calc_sqr_D0( lnx = calc_ln_D0(x) );
  return( calc_poly(poly,nwrt1,lnx,lnx2)+calc_poly(poly,nwrt1-1,lnx,lnx2) );
}
