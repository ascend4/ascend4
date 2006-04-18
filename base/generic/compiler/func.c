/*
 *  Function Module
 *  by Tom Epperly
 *  Created: 8/11/1990
 *  Version: $Revision: 1.18 $
 *  Version control file: $RCSfile: func.c,v $
 *  Date last modified: $Date: 2001/01/31 22:23:53 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#include<math.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "safe.h"
#include "func.h"

#ifndef M_PI
#define M_PI F_PI
#endif
#ifndef M_LOG10E
#define M_LOG10E F_LOG10_COEF
#endif

#ifndef NULL
#define NULL 0L
#endif

#ifndef lint
static CONST char FunctionEvalRCSid[]="$Id: func.c,v 1.18 2001/01/31 22:23:53 ballan Exp $";
#endif


double g_lnm_epsilon = 1.0e-8;


#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
double cbrt(register double d)
{
  return pow(d,(double)0.3333333333333333333333);
}
#endif

int ascnintF(register double d)
{
  return ((d)>=0.0 ? (int)floor((d) + 0.5) : -(int)floor(0.5 - (d)));
}


double dln(register double d)
{
  return 1.0/d;
}

double dln2(register double d)
{
  return -1.0/(d*d);
}


double lnm(register double d)
{
  return (d>g_lnm_epsilon?log(d):d/g_lnm_epsilon + (log(g_lnm_epsilon) -1));

}

double dlnm(register double d)
{
  return ( d>g_lnm_epsilon ? (double)1.0/d : 1/g_lnm_epsilon);
}

double dlnm2(register double d)
{
  return (d>g_lnm_epsilon ? (double)-1.0/(d*d) : (double)0.0);
}

double dlog10(register double d)
{
  return M_LOG10E/d;
}

double dlog102(register double d)
{
  return -M_LOG10E/(d*d);
}

double dcos(register double d)
{
  return -sin(d);
}

double dcos2(register double d)
{
  return -cos(d);
}

double dtan(register double d)
{
  register double t;
  t=cos(d);
  return 1.0/(t*t);
}

double dtan2(register double d)
{
  register double t;
  t=cos(d);
  return ldexp(tan(d)/(t*t),1);
}

double sqr(register double d)
{
  return d*d;
}

double dsqr(register double d)
{
  return ldexp(d,1);
}

double dsqr2(register double d)
{
  (void)d;  /*  stop gcc whine about unused parameter */
  return 2.0;
}

double hold(double d)
{
  return d;
}

double dsqrt(register double d)
{
  return 1.0/(ldexp(sqrt(d),1));
}

double dsqrt2(register double d)
{
  return -1.0/ldexp(sqrt(d)*d,2);
}

double dfabs(register double d)
{
  return ((d > 0.0) ? 1.0 : ((d<0.0 ) ? -1 : 0));
}

double dfabs2(register double d)
{
  (void)d;  /*  stop gcc whine about unused parameter */
  return 0.0;
}

double dhold(double d)
{
  (void)d;  /*  stop gcc whine about unused parameter */
  return 0;
}

 /* The next 4 are new */
double asc_ipow(register double d, int i) {
  unsigned negative = 0;
  negative = (i<0);
  if (negative) i = (-i);
  if (d==0 && i!=0) return 0.0;
  switch (i) {
  case 0: return 1.0; /* a^0 = 1, for a==0 pow is undefined. */
  case 1: break;
  case 2: d *= d; break;
  case 3: d = d*d*d; break;
  case 4: d = d*d*d*d;break;
  case 5: d = d*d*d*d*d; break;
  case 6: d = d*d*d*d*d*d; break;
  case 7: d = d*d*d*d*d*d*d; break;
  case 8: d = d*d*d*d*d*d*d*d; break;
  case 9: d = d*d*d*d*d*d*d*d*d; break;
  default:
    {
      register double res;
      res = d;
      for (--i; i > 0; i--) res *= d;
      d = res;
    }
    break;
  }
  return (!negative ? d : 1.0/d);
}

/*
 * Note that the following derivative functions do not
 * set calc_ok to FALSE in the event of errors.  This
 * checking is done in the solver so we are baisicaly
 * double checking now -> this should be fixed
 */

double asc_d1ipow(double d, int i) {
    if (d == 0 && i <= 1) {
	FPRINTF(stderr,"ERROR:\t(calc) calc_ipow_D1\n");
	FPRINTF(stderr,
                "\t1st derivative, %g raised to %d <= 1 power undefined.\n",
                d,i);
	FPRINTF(stderr,"\tReturning %g.\n",0.0);
	return(0.0);
    }
    return( i * asc_ipow(d,i-1));
}

double asc_d2ipow(double d, int i) {
    if (d == 0 && i <= 2) {
	FPRINTF(stderr,"ERROR:\t(calc) calc_ipow_D2\n");
	FPRINTF(stderr,
                "\t2nd derivative, %g raised to %d <= 2 power undefined.\n",
                d,i);
	FPRINTF(stderr,"\tReturning %g.\n",0.0);
	return(0.0);
    }
    return( i * (i - 1) * asc_ipow(d,i-2));
}


double cube(register double d)
{
  return d*d*d;
}
double dcube(register double d)
{
  return 3.0*d*d;
}
double dcube2(register double d)
{
  return 6.0*d;
}

double dcbrt(register double d)
{
  register double c;
  c=cbrt(d);
  return (double)0.3333333333333333/(c*c);
}

double dcbrt2(register double d)
{
  register double c;
  c=cbrt(d);
  return (double)-0.2222222222222222/pow(c,5.0);
}

double dasin(register double d)
{
  return 1.0/sqrt(1.0-d*d);
}

double dasin2(register double d)
{
  register double c;
  c=1.0-d*d;
  return d/(c*sqrt(c));
}

double dacos(register double d)
{
  return -1.0/sqrt(1-d*d);
}

double dacos2(register double d)
{
  register double c;
  c=1.0-d*d;
  return -d/(c*sqrt(c));
}

double datan(register double d)
{
  return 1.0/(1.0+d*d);
}

double datan2(register double d)
{
  return -ldexp(d/(1.0+d*d),1);
}

#ifdef HAVE_ERF
double derf(register double d)
{
  return ldexp(exp(-(d*d))/sqrt(M_PI),1);
}

double derf2(register double d)
{
  return -ldexp(d*exp(-(d*d))/sqrt(M_PI),2);
}
#endif /* HAVE_ERF */

double dtanh(register double d)
{
  register double c;
  c = cosh(d);
  c = 1/(c*c);
  return c;
}

double dtanh2(register double d)
{
  register double c;
  c = cosh(d);
  return -ldexp(tanh(d),1)/(c*c);
}

double arcsinh(register double d)
{
  return log(d+sqrt(d*d+1.0));
}

double darcsinh(register double d)
{
  return 1.0/sqrt(d*d+1.0);
}

double darcsinh2(register double d)
{
  register double c;
  c=d*d+1.0;
  return -d/sqrt(c*c*c);
}

double arccosh(register double d)
{
  return log(d+sqrt(d*d-1.0));
}

double darccosh(register double d)
{
  return 1.0/sqrt(d*d-1.0);
}

double darccosh2(register double d)
{
  register double c;
  c=d*d-1.0;
  return -d/sqrt(c*c*c);
}

double arctanh(register double d)
{
  return  ldexp( log((d+1.0)/(1.0-d)) ,-1);
/* an alternative, more expensive but perhaps less exception prone
*  coding of arctanh is:
* return log(sqrt((d+1.0)/(1.0-d)));
* which for d near -1 will be less likely to underflow and send 0
* to the log function. Until otherwise noted we are running the
* cheap version.
*/
}

double darctanh(register double d)
{
  return  1.0/(1-d*d);
}

double darctanh2(register double d)
{
  register double c;
  c=1.0-d*d;
  return ldexp( d/(c*c) ,1);
}

#ifdef CHRIS_FUNC
void ExpSlope(unsigned long int nvar,
	      struct Interval *center, struct Interval *range,
	      struct Interval *slope)
{
  *center = ExpInterval(*center);
  *range = ExpInterval(*range);
  while (nvar--){
    *slope = MulIntervals(*range,*slope);
    slope++;
  }
}

void LnSlope(unsigned long int nvar,
	     struct Interval *center, struct Interval *range,
	     struct Interval *slope)
{
  while(nvar--){
    *slope = DivIntervals(*slope,*range);
    slope++;
  }
  *center = LnInterval(*center);
  *range = LnInterval(*range);
}

void LogSlope(unsigned long int nvar,
	      struct Interval *center, struct Interval *range,
	      struct Interval *slope)
{
  struct Interval temp;
  temp = MulIntervals(CreateThin(M_LN10),*range);
  while(nvar--){
    *slope = DivIntervals(*slope,temp);
    slope++;
  }
  *center = LogInterval(*center);
  *range = LogInterval(*range);
}

void SqrSlope(unsigned long int nvar,
	      struct Interval *center, struct Interval *range,
	      struct Interval *slope)
{
  struct Interval temp;
  temp = AddIntervals(*center,*range);
  while(nvar--){
    *slope = MulIntervals(temp,*slope);
    slope++;
  }
  *center = SqrInterval(*center);
  *range = SqrInterval(*range);
}

void SqrtSlope(unsigned long int nvar,
	       struct Interval *center, struct Interval *range,
	       struct Interval *slope)
{
  struct Interval temp;
  *center = SqrtInterval(*center);
  *range = SqrtInterval(*range);
  temp = AddIntervals(*center,*range);
  while(nvar--){
    *slope = DivIntervals(*slope,temp);
    slope++;
  }
}

#ifdef HAVE_ERF
void ErfSlope(unsigned long int nvar,
	      struct Interval *center, struct Interval *range,
	      struct Interval *slope)
{
  struct Interval temp;
  temp =
    DivIntervals(MulIntervals(CreateThinInteger(2l),
			      ExpInterval(NegInterval(SqrInterval(*range)))),
		 SqrtInterval(CreateThin(M_PI)));
  while(nvar--){
    *slope =
      MulIntervals(temp,*slope);
    slope++;
  }
  *center = ErfInterval(*center);
  *range = ErfInterval(*range);
}

struct Interval ErfDeriv(struct Interval i)
{
  return
    DivIntervals(MulIntervals(CreateThinInteger(2l),
			      ExpInterval(NegInterval(SqrInterval(i)))),
		 SqrtInterval(CreateThin(M_PI)));
}
#endif /* HAVE_ERF */

struct Interval LnDeriv(struct Interval i)
{
  return DivIntervals(CreateThinInteger(1L),i);
}

struct Interval LogDeriv(struct Interval i)
{
  return DivIntervals(CreateThin(M_LOG10E),i);
}

struct Interval SqrDeriv(struct Interval i)
{
  return MulIntervals(CreateThinInteger(2L),i);
}

struct Interval SqrtDeriv(struct Interval i)
{
  return DivIntervals(CreateThinInteger(1L),
		      MulIntervals(CreateThinInteger(2L),
				   SqrtInterval(i)));
}

double MinOfRange(double lower, double upper)
{
  return lower;
}

double MaxOfRange(double lower, double upper)
{
  return upper;
}

double ArgMinSqr(double lower, double upper)
{
  if (upper < 0.0) return upper;
  if (lower > 0.0) return lower;
  return 0.0;
}

double ArgMaxSqr(double lower, double upper)
{
  return (ABS(lower)>ABS(upper))?lower:upper;
}

double ConvexOfSqr(double x, double lower, double upper,
		   double (*value) (/* ??? */))
{
  return x*x;
}

double ConvexDOfSqr(double x, double lower, double upper,
		    double (*value) (/* ??? */))
{
  return 2*x;
}

double ConcaveOfSqr(double x, double lower, double upper,
		    double (*value) (/* ??? */))
{
  return (lower+upper)*x-lower*upper;
}

double ConcaveDOfSqr(double x, double lower, double upper,
		     double (*value) (/* ??? */))
{
  return lower+upper;
}

double ConvexOfExp(double x, double lower, double upper,
		   double (*value) (/* ??? */))
{
  return exp(x);
}

double ConcaveOfLn(double x, double lower, double upper,
		   double (*value) (/* ??? */))
{
  return log(x);
}

double ConcaveDOfLn(double x, double lower, double upper,
		    double (*value) (/* ??? */))
{
  return 1.0/x;
}

double ConcaveOfLog(double x, double lower, double upper,
		    double (*value) (/* ??? */))
{
  return log10(x);
}

double ConcaveDOfLog(double x, double lower, double upper,
		     double (*value) (/* ??? */))
{
  return M_LOG10E/x;
}

double ConcaveOfSqrt(double x, double lower, double upper,
		     double (*value) (/* ??? */))
{
  return sqrt(x);
}

double ConcaveDOfSqrt(double x, double lower, double upper,
		      double (*value) (/* ??? */))
{
  return 0.5/sqrt(x);
}

double Interpolate(double x, double lower, double upper,
		   double (*value) (/* ??? */))
{
  register double vl,vu;
  vl = (*value)(lower);
  vu = (*value)(upper);
  return ((vu-vl)*x+upper*vl-lower*vu)/(upper-lower);
}

double InterpolateD(double x, double lower, double upper,
		    double (*value) (/* ??? */))
{
  return ((*value)(upper)-(*value)(lower))/(upper-lower);
}

#endif

struct Func g_exp_f = {
  "exp",
  "exp",
  "exp",
  "exp",
  F_EXP,
  exp,
  exp,
  exp,
  safe_exp_D0,
  safe_exp_D1,
  safe_exp_D2,
#ifdef CHRIS_FUNC
  ExpInterval,
  ExpSlope,
  ExpInterval,
  MinOfRange,
  MaxOfRange,
  ConvexOfExp,
  ConvexOfExp,
  Interpolate,
  InterpolateD
#endif
};

struct Func g_ln_f = {
  "ln",
  "log",
  "dln",
  "dln2",
  F_LN,
  log,
  dln,
  dln2,
  safe_ln_D0,
  safe_ln_D1,
  safe_ln_D2,
#ifdef CHRIS_FUNC
  LnInterval,
  LnSlope,
  LnDeriv,
  MinOfRange,
  MaxOfRange,
  Interpolate,
  InterpolateD,
  ConcaveOfLn,
  ConcaveDOfLn
#endif
};

struct Func g_lnm_f = {
  "lnm",
  "lnm",
  "dlnm",
  "dlnm2",
  F_LNM,
  lnm,
  dlnm,
  dlnm2,
  safe_lnm_D0,
  safe_lnm_D1,
  safe_lnm_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_log10_f = {
  "log10",
  "log10",
  "dlog10",
  "dlog102",
  F_LOG10,
  log10,
  dlog10,
  dlog102,
  safe_log10_D0,
  safe_log10_D1,
  safe_log10_D2,
#ifdef CHRIS_FUNC
  Log10Interval,
  Log10Slope,
  Log10Deriv,
  MinOfRange,
  MaxOfRange,
  Interpolate,
  InterpolateD,
  ConcaveOfLog10,
  ConcaveDOfLog10
#endif
};

struct Func g_sin_f = {
  "sin",
  "sin",
  "cos",
  "dcos",
  F_SIN,
  sin,
  cos,
  dcos,
  safe_sin_D0,
  safe_sin_D1,
  safe_sin_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_cos_f = {
  "cos",
  "cos",
  "dcos",
  "dcos2",
  F_COS,
  cos,
  dcos,
  dcos2,
  safe_cos_D0,
  safe_cos_D1,
  safe_cos_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_tan_f = {
  "tan",
  "tan",
  "dtan",
  "dtan2",
  F_TAN,
  tan,
  dtan,
  dtan2,
  safe_tan_D0,
  safe_tan_D1,
  safe_tan_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_sqr_f = {
  "sqr",
  "sqr",
  "dsqr",
  "dsqr2",
  F_SQR,
  sqr,
  dsqr,
  dsqr2,
  safe_sqr_D0,
  safe_sqr_D1,
  safe_sqr_D2,
#ifdef CHRIS_FUNC
  SqrInterval,
  SqrSlope,
  SqrDeriv,
  ArgMinSqr,
  ArgMaxSqr,
  ConvexOfSqr,
  ConvexDOfSqr,
  ConcaveOfSqr,
  ConcaveDOfSqr
#endif
};

struct Func g_sqrt_f = {
  "sqrt",
  "sqrt",
  "dsqrt",
  "dsqrt2",
  F_SQRT,
  sqrt,
  dsqrt,
  dsqrt2,
  safe_sqrt_D0,
  safe_sqrt_D1,
  safe_sqrt_D2,
#ifdef CHRIS_FUNC
  SqrtInterval,
  SqrtSlope,
  SqrtDeriv,
  MinOfRange,
  MaxOfRange,
  Interpolate,
  InterpolateD,
  ConcaveOfSqrt,
  ConcaveDOfSqrt
#endif
};

struct Func g_abs_f = {
  "abs",
  "fabs",
  "dfabs",
  "dfabs2",
  F_ABS,
  fabs,
  dfabs,
  dfabs2,
  safe_fabs_D0,
  safe_fabs_D1,
  safe_fabs_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_hold_f = {
  "hold",
  "hold",
  "dhold",
  "dhold2",
  F_HOLD,
  hold,
  dhold,
  dhold2,
  safe_hold_D0,
  safe_hold_D1,
  safe_hold_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arcsin_f = {
  "arcsin",
  "asin",
  "dasin",
  "dasin2",
  F_ARCSIN,
  asin,
  dasin,
  dasin2,
  safe_arcsin_D0,
  safe_arcsin_D1,
  safe_arcsin_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arccos_f = {
  "arccos",
  "acos",
  "dacos",
  "dacos2",
  F_ARCCOS,
  acos,
  dacos,
  dacos2,
  safe_arccos_D0,
  safe_arccos_D1,
  safe_arccos_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arctan_f = {
  "arctan",
  "atan",
  "datan",
  "datan2",
  F_ARCTAN,
  atan,
  datan,
  datan2,
  safe_tan_D0,
  safe_tan_D1,
  safe_tan_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

#ifdef HAVE_ERF
struct Func g_erf_f = {
  "erf",
  "erf",
  "derf",
  "derf2",
  F_ERF,
  erf,
  derf,
  derf2,
  safe_erf_D0,
  safe_erf_D1,
  safe_erf_D2,
#ifdef CHRIS_FUNC
  ErfInterval,
  ErfSlope,
  ErfDeriv
#endif
};
#endif /* HAVE_ERF */

struct Func g_sinh_f = {
  "sinh",
  "sinh",
  "cosh",
  "sinh",
  F_SINH,
  sinh,
  cosh,
  sinh,
  safe_sinh_D0,
  safe_sinh_D1,
  safe_sinh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_cosh_f = {
  "cosh",
  "cosh",
  "sinh",
  "cosh",
  F_COSH,
  cosh,
  sinh,
  cosh,
  safe_cosh_D0,
  safe_cosh_D1,
  safe_cosh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_tanh_f = {
  "tanh",
  "tanh",
  "dtanh",
  "dtanh2",
  F_TANH,
  tanh,
  dtanh,
  dtanh2,
  safe_tanh_D0,
  safe_tanh_D1,
  safe_tanh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arcsinh_f = {
  "arcsinh",
  "arcsinh",
  "darcsinh",
  "darcsinh2",
  F_ARCSINH,
  arcsinh,
  darcsinh,
  darcsinh2,
  safe_arcsinh_D0,
  safe_arcsinh_D1,
  safe_arcsinh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arccosh_f = {
  "arccosh",
  "arccosh",
  "darccosh",
  "darccosh2",
  F_ARCCOSH,
  arccosh,
  darccosh,
  darccosh2,
  safe_arccosh_D0,
  safe_arccosh_D1,
  safe_arccosh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_arctanh_f = {
  "arctanh",
  "arctanh",
  "darctanh",
  "darctanh2",
  F_ARCTANH,
  arctanh,
  darctanh,
  darctanh2,
  safe_arctanh_D0,
  safe_arctanh_D1,
  safe_arctanh_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_cube_f = {
  "cube",
  "cube",
  "dcube",
  "dcube2",
  F_CUBE,
  cube,
  dcube,
  dcube2,
  safe_cube,
  safe_cube_D1,
  safe_cube_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

struct Func g_cbrt_f = {
  "cbrt",
  "cbrt",
  "dcbrt",
  "dcbrt2",
  F_CBRT,
  cbrt,
  dcbrt,
  dcbrt2,
  safe_cbrt_D0,
  safe_cbrt_D1,
  safe_cbrt_D2,
#ifdef CHRIS_FUNC
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};


struct Func *g_func_list[]={
  &g_log10_f,
  &g_ln_f,
  &g_exp_f,
  &g_sin_f,
  &g_cos_f,
  &g_tan_f,
  &g_sqr_f,
  &g_sqrt_f,
  &g_arcsin_f,
  &g_arccos_f,
  &g_arctan_f,
#ifdef HAVE_ERF
  &g_erf_f,
#endif /* HAVE_ERF */
  &g_lnm_f,
  &g_sinh_f,
  &g_cosh_f,
  &g_tanh_f,
  &g_arcsinh_f,
  &g_arccosh_f,
  &g_arctanh_f,
  &g_cube_f,
  &g_cbrt_f,
  &g_abs_f,
  &g_hold_f,
  NULL	/* must be last */
};

CONST struct Func *LookupFunc(CONST char *name)
{
  unsigned f=0;
  while(g_func_list[f]!=NULL){
    if(strcmp(g_func_list[f]->name,name)==0)
      return g_func_list[f];
    f++;
  }
  return NULL;
}

CONST struct Func *LookupFuncById(enum Func_enum id)
{
  unsigned f=0;
  while(g_func_list[f]!=NULL){
    if (g_func_list[f]->id==id)
      return g_func_list[f];
    f++;
  }
  return NULL;
}

CONST char *FuncName(CONST struct Func *f)
{
  return f->name;
}

CONST char *FuncCName(CONST struct Func *f)
{
  return f->cname;
}

CONST char *FuncDeriv1CName(CONST struct Func *f)
{
  return f->deriv1cname;
}

CONST char *FuncDeriv2CName(CONST struct Func *f)
{
  return f->deriv2cname;
}

enum Func_enum FuncId(CONST struct Func *f)
{
  return f->id;
}

CONST dim_type *FuncDimens(CONST struct Func *f)
{
  if (!f) return Dimensionless();
  switch (FuncId(f)) {
    case F_LOG10:
    case F_LN:
    case F_EXP:
#ifdef HAVE_ERF
    case F_ERF:
#endif /* HAVE_ERF */
    case F_LNM:
    case F_ARCSIN:
    case F_ARCCOS:
    case F_ARCTAN:
    case F_SINH:
    case F_COSH:
    case F_TANH:
    case F_ARCSINH:
    case F_ARCCOSH:
    case F_ARCTANH:
      return Dimensionless();
    case F_SQR:
    case F_SQRT:
    case F_CUBE:
    case F_CBRT:
    case F_ABS:
    case F_HOLD:
      return WildDimension();
    case F_SIN:
    case F_COS:
    case F_TAN:
      return TrigDimension();
    default: return Dimensionless();
  }
}

double FuncEval(CONST struct Func *f, double d)
{
  return (*(f->value))(d);
}

double FuncEvalSafe(CONST struct Func *f, double d,enum safe_err *not_safe)
{
  return (*(f->safevalue))(d,not_safe);
}

double FuncDeriv(CONST struct Func *f, double d)
{
  return (*(f->deriv))(d);
}

double FuncDerivSafe(CONST struct Func *f, double d,enum safe_err *not_safe)
{
  return (*(f->safederiv))(d,not_safe);
}

double FuncDeriv2(CONST struct Func *f, double d)
{
  return (*(f->deriv2))(d);
}

double FuncDeriv2Safe(CONST struct Func *f, double d,enum safe_err *not_safe)
{
  return (*(f->safederiv2))(d,not_safe);
}

#ifdef CHRIS_FUNC
struct Interval FuncRange(CONST struct Func *f, struct Interval u)
{
  return (*(f->ivalue))(u);
}

void FuncSlope(CONST struct Func *f, unsigned long int nvar,
	       struct Interval *center, struct Interval *range,
	       struct Interval *slope)
{
  (*(f->slope))(nvar,center,range,slope);
}

struct Interval FuncIDeriv(CONST struct Func *f, struct Interval i)
{
  return (*(f->ideriv))(i);
}

double ArgMin(CONST struct Func *f, double lower, double upper)
{
  return (*(f->tmin))(lower,upper);
}

double ArgMax(CONST struct Func *f, double lower, double upper)
{
  return (*(f->tmax))(lower,upper);
}

double ConvexEnv(CONST struct Func *f, double x, double lower, double upper)
{
  assert((x>=lower)&&(x<=upper));
  return (*(f->e))(x,lower,upper,f->value);
}

double ConvexEnvDeriv(CONST struct Func *f, double x,
		      double lower, double upper)
{
  assert((x>=lower)&&(x<=upper));
  return (*(f->ed))(x,lower,upper,f->value);
}

double ConcaveEnv(CONST struct Func *f, double x, double lower, double upper)
{
  assert((x>=lower)&&(x<=upper));
  return (*(f->E))(x,lower,upper,f->value);
}

double ConcaveEnvDeriv(CONST struct Func *f, double x,
		       double lower, double upper)
{
  assert((x>=lower)&&(x<=upper));
  return (*(f->Ed))(x,lower,upper,f->value);
}
#endif
