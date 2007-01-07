/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 	Copyright (C) 2006 Carnegie Mellon University

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
	Function-of-one-variable module.

	@TODO Complete documentation of func.h.

	Revision notes: If CHRIS_FUNC defined, auxillary quantities to func structure.

	@TODO 2/96 baa Probably somebody should properly set these evaluation
	defined below based on math.h when possible.
*//*
	by Tom Epperly
	Version: $Revision: 1.16 $
	Version control file: $RCSfile: func.h,v $
	Date last modified: $Date: 2001/01/31 22:23:57 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_FUNC_H
#define ASC_FUNC_H

/**	addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>
#include "functype.h"
#include "compiler.h"
#include "dimen.h"

/*
 *  the following should be ifdefed to deal with math.h values
 */
#define F_ERF_COEF    1.1283791670955130      /**< = 2 / sqrt(PI) */
#define F_LOG10_COEF  0.4342944819032518      /**< = log10(e) = 1/ln(10) */
#define F_PI          3.1415926535897932385E0
#define F_PI_HALF     1.5707963267948966192E0
#define F_LIM_EXP     709.78                  /**< approximately ln(maxdouble) */
#define F_LIM_CUBE    5.6438030941223618e101  /**< cbrt(maxdouble)*/
#define F_LIM_SQR     1.0e154                 /**< sqrt(maxdouble) */

#ifdef __STDC__
# if __STDC__
/**
 * stdc==1 --> erf, cbrt not defined in headers. user should link
 * against a library that does provide them. ASCEND is research
 * code: we aren't going to waste time reimplementing these basic
 * functions.
 */
ASC_DLLSPEC double cbrt(double);

#  ifdef HAVE_ERF
extern double erf(double);
#  endif /* HAVE_ERF */
# endif /* __STDC__ == 1*/
/**<
 * in the case where __STDC__ is defined but == 0, system headers
 * should provide cbrt, erf.
 */
#endif /* stdc defined */

extern CONST struct Func *LookupFunc(CONST char *name);
/**<
 *  Lookup the function with the given name.  If no match is found, it
 *  returns NULL.  name is the ASCEND name, not C name.
 *  <pre>
 *  Currently defined:
 *         "exp"           e^x
 *         "ln"            natural logarithm of x
 *         "log10"           logarithm of x base 10
 *         "sin"           sine of x
 *         "cos"           cosine of x
 *         "tan"           tangent of x
 *         "sqr"           x*x
 *         "sqrt"          the square root of x
 *         "arcsin"        the inverse sine of x
 *         "arccos"        the inverse cosine of x
 *         "arctan"        the inverse tangent of x
 *
 *         "erf"           the error function
 *         "lnm"           modified natural log:
 *                         x>epsilon? ln(x): x/epsilon +ln(epsilon) -1
 *         "sinh"          hyperbolic sine
 *         "cosh"          hyperbolic cosine
 *         "tanh"          hyperbolic tangent
 *         "arcsinh"       inv hyperbolic sine
 *         "arccosh"       inv hyperbolic cosine
 *         "arctanh"       inv hyperbolic tangent
 *
 *         "cube"          the cube of x
 *         "cbrt"          the cube root of x
 *         "abs"           absolute value of x
 *
 *         "hold"          returns the current value of x.
 *  </pre>
 *  The last 10 are not fully supported for chris.[ch]
 *  All functions return principal (postive) values if there is a choice.
 *  None of them perform range checking. ln(-1) -> float error.
 */

extern CONST struct Func *LookupFuncById(enum Func_enum id);
/**<
 *  Lookups a function by its enumerated type rather than by a string as
 *  in the above function.
 */

extern double FuncGetLnmEpsilon(void);
/**<
 *  Return the current epsilon for the modified log function lnm.
 */
#define FuncGetLnmEpsilon() (g_lnm_epsilon)

extern void FuncSetLnmEpsilon(double e);
/**<
 *  Change the current epsilon for the modified log function lnm.
 *  epsilon > 0.0.
 */
ASC_DLLSPEC double g_lnm_epsilon;
#define FuncSetLnmEpsilon(e) \
   (e>(double)0.0 ? g_lnm_epsilon=e : FPRINTF(ASCERR,"bad lnm eps"))

/**<
 *  declare cbrt() and erf() since some vendors put
 *  these functions in odd headers
 */
ASC_DLLSPEC double cbrt(double);
#ifdef HAVE_ERF
extern double erf(double);
#endif /* HAVE_ERF */

#ifdef NDEBUG
# define ascnint(d) (((int) (d)>=0.0 ? floor((d) + 0.5) : -floor(0.5 - (d))))
/**<
 *  Converts a double to the nearest integer (release version).
 *  @param d double, the real number to convert.
 *  @return The nearest integer as an int.
 */
#else
# define ascnint(a) ascnintF(a)
/**<
 *  Converts a double to the nearest integer (debug version).
 *  @param d double, the real number to convert.
 *  @return The nearest integer as an int.
 */
ASC_DLLSPEC int ascnintF(double);
/**<
 *  Implementation function for debug version of ascnint().
 *  Do not call this function directly - use ascnint() instead.
 */

#endif

ASC_DLLSPEC double dln(double x);
ASC_DLLSPEC double dln2(double x);
ASC_DLLSPEC double dlog10(double x);
ASC_DLLSPEC double dlog102(double x);
ASC_DLLSPEC double lnm(double x);
ASC_DLLSPEC double dlnm(double x);
ASC_DLLSPEC double dlnm2(double x);
/**<
 *  Modified natural log function and derivatives.
 *  <pre>
 *
 *                   ( ln(x)                       if x.ge.epsilon
 *         lnm(x)=   (
 *                   ( x/epsilon + ln(epsilon) - 1      otherwise
 *
 *
 *                                  ( 1/x          if x.ge.epsilon
 *         d( lnm(x) )/dx       =   (
 *                                  ( 1/epsilon    otherwise
 *
 *
 *                                  ( -1/x^2       if x.ge.epsilon
 *         d^2( lnm(x) )/dx^2   =   (
 *                                  ( 0            otherwise
 *
 *  epsilon determined by FuncGet/SetLnmEpsilon.
 *  </pre>
 */

ASC_DLLSPEC double dtanh(double x);
ASC_DLLSPEC double dtanh2(double x);
ASC_DLLSPEC double arcsinh(double x);
ASC_DLLSPEC double arccosh(double x);
ASC_DLLSPEC double arctanh(double x);
ASC_DLLSPEC double darcsinh(double x);
ASC_DLLSPEC double darcsinh2(double x);
ASC_DLLSPEC double darccosh(double x);
ASC_DLLSPEC double darccosh2(double x);
ASC_DLLSPEC double darctanh(double x);
ASC_DLLSPEC double darctanh2(double x);
/**<
 *  Zero, first and second partials of (inverse) hyperbolic functions.
 */

ASC_DLLSPEC double sqr(double x);
ASC_DLLSPEC double dsqr(double x);
ASC_DLLSPEC double dsqr2(double x);
ASC_DLLSPEC double cube(double x);
ASC_DLLSPEC double dcube(double x);
ASC_DLLSPEC double dcube2(double x);
/**<
 *  Zero, first and second partials of x for sqr, cube.
 */

ASC_DLLSPEC double asc_ipow(double a, int n);
ASC_DLLSPEC double asc_d1ipow(double a, int n);
ASC_DLLSPEC double asc_d2ipow(double a, int n);
/**<
 *  Integer power function, a^n, and its first and second derivatives.
 *  d = asc_ipow(a,n);
 *  d1 = asc_d1ipow(a,n);
 *  d2 = asc_d2ipow(a,n);
 *
 *  Special cases ipow:
 *  a^0 = 1, 0^n = 0, 0^0=1 -- the last is mathematically undefined,
 *  so this function should not be called with 0.0,0.
 *
 *  Special cases d1ipow,d2ipow:
 */

ASC_DLLSPEC double hold(double x);
/**<
 *  Returns the value it is passed.
 *  The primary purpose is as an operator so we can write
 *  equations of the form x = hold(x) which act essentially
 *  as a fixed flag when a solver is partitioning.
 *  If x is fixed, the equation is singular.
 *  hold(x) is a constant value, so its derivatives are 0.
 */

ASC_DLLSPEC double dsqrt(double x);
ASC_DLLSPEC double dsqrt2(double x);
ASC_DLLSPEC double dcbrt(double x);
ASC_DLLSPEC double dcbrt2(double x);
ASC_DLLSPEC double dfabs(double x);
ASC_DLLSPEC double dfabs2(double x);
ASC_DLLSPEC double dhold(double x);
#define dhold2 dhold
/**<
 *  first and second partials of sqrt cbrt fabs hold
 *  dfabs is undefined at 0. We take the standard kluge: d(abs(x=0))/dx=0
 *  dfabs2(x) = 0. It might be thought of as infinite at x=0, but not here.
 *  dhold, dhold2 = 0 for all x.
 */

ASC_DLLSPEC double dasin(double x);
ASC_DLLSPEC double dasin2(double x);
ASC_DLLSPEC double dcos(double x);
ASC_DLLSPEC double dcos2(double x);
ASC_DLLSPEC double dacos(double x);
ASC_DLLSPEC double dacos2(double x);
ASC_DLLSPEC double dtan(double x);
ASC_DLLSPEC double dtan2(double x);
ASC_DLLSPEC double datan(double x);
ASC_DLLSPEC double datan2(double x);
/**<
 *  First and second partials of the cosine, tangent, arctangent functions
 */

#ifdef HAVE_ERF
ASC_DLLSPEC double derf(double x);
ASC_DLLSPEC double derf2(double x);
#endif /* HAVE_ERF */
/**<
 *  First and second derivatives of erf()
 */

ASC_DLLSPEC CONST char *FuncName(CONST struct Func *f);
/**<
 *  Return the ASCEND language name of the function.
 *  Not a symchar.
 */

extern CONST char *FuncCName(CONST struct Func *f);
/**<
 *  Return the C language name of the function, if any.
 */

extern CONST char *FuncDeriv1CName(CONST struct Func *f);
/**<
 *  Return the C language name of the function first derivative, if any.
 */

extern CONST char *FuncDeriv2CName(CONST struct Func *f);
/**<
 *  Return the C language name of the function second derivative, if any.
 */

ASC_DLLSPEC enum Func_enum FuncId(CONST struct Func *f);
/**<
 *  Return the identification of the function.
 */

extern CONST dim_type *FuncDimens(CONST struct Func *f);
/**<
 *  Return the dimensionality required for the arg of the function.
 *  sin, cos, tan -> P.
 *  arc(sin,cos,tan),exp,ln,log,lnm,erf ->dimensionless.
 *  sqrt->wilddimension (user must check for even poweredness of arg)
 *  as this may be the case for an infinite # of different dims.
 *  sqr ->wilddimension (sqr, abs, cube anything you like.)
 */

extern double FuncEval(CONST struct Func *f, double u);
/**<
 *  Return f(u).
 */

extern double FuncEvalSafe(CONST struct Func *f,
                           double u,
                           enum safe_err *not_safe);
/**<
 *  Return f(u) (safe version).
 */

extern double FuncDeriv(CONST struct Func *f, double u);
/**<
 *  Return df/du evaluated at u.
 */

extern double FuncDerivSafe(CONST struct Func *f,
                            double u,
                            enum safe_err *not_safe);
/**<
 *  Return df/du evaluated at u (safe version).
 */

extern double FuncDeriv2(CONST struct Func *f, double u);
/**<
 *  Return the second derivative (d^2f/du^2) evaluated at u.
 */

extern double FuncDeriv2Safe(CONST struct Func *f,
                             double u,
                             enum safe_err *not_safe);
/**<
 *  Return the second derivative (d^2f/du^2) evaluated at u (safe version).
 */

#ifdef CHRIS_FUNC

extern struct Interval FuncRange(CONST struct Func *f, struct Interval i);
/**<
 *  Return a bound on the range of the function over the given interval.
 */

extern void FuncSlope(CONST struct Func *f,
                      unsigned long nvar,
                      struct Interval *center,
                      struct Interval *range,
                      struct Interval *slope);
/**<
 *  Perform the interval slope calculation.
 */

extern struct Interval FuncIDeriv(CONST struct Func *f, struct Interval i);
/**<
 *  ...
 */

extern double ArgMin(CONST struct Func *f, double lower, double upper);
/**<
 *  Return the arg min of the function over the range.
 */

extern double ArgMax(CONST struct Func *f, double lower, double upper);
/**<
 *  Return the arg max of the function over the range.
 */

extern double ConvexEnv(CONST struct Func *f, double x,
                        double lower, double upper);
/**<
 *  Return the value of the convex envelope of the function at the value
 *  x which ranges from lower to upper.
 */

extern double ConvexEnvDeriv(CONST struct Func *f, double x,
                             double lower, double upper);
/**<
 *  Return the derivative of the convex envelope of the function at the
 *  value x which ranges from lower to upper.
 */

extern double ConcaveEnv(CONST struct Func *f, double x,
                         double lower, double upper);
/**<
 *  Return the value of the concave envelope of the function at the value
 *  x which ranges from lower to upper.
 */

extern double ConcaveEnvDeriv(CONST struct Func *f, double x,
                              double lower, double upper);
/**<
 *  Return the derivative of the concave envelope of the function at the
 *  value x which ranges from lower to upper.
 */

#endif /* CHRIS_FUNC */

/* @} */

#endif /* ASC_FUNC_H */
