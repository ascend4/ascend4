/*
 *  Safe function evaluator.
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: safe.h,v $
 *  Date last modified: $Date: 1997/07/21 16:56:51 $
 *  Last modified by: $Author: kt2g $
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
 *  COPYING.
 *
 */

/*
 *  Contents:     Safe calculation module
 *
 *  Authors:      Karl Westerberg, Joseph Zaher
 *
 *  Dates:        06/90 - original version in solver
 *                08/93 - removed safe_D0, safe_D1, safe_D2, safe_Dn
 *                        and added them as internal functions of the
 *                        exprman module.
 *                04/94 - enhanced error messaging
 *                06/96 - dealiased and moved into compiler. BAA
 *
 *  Description:  Operations are provided for computing unary and binary
 *                functions.  First, second, and nth derivatives may
 *                also be computed for all of the functions featured.  A
 *                global boolean variable safe_ok, which is declared
 *                external so that it may be monitored from other
 *                modules, will be set to FALSE in the event of a
 *                calculation error.  Also, to suppress printing of error
 *                messages in the event of an error, set the global
 *                variable safe_print_errors to FALSE.
 *                The intent here is to provide safer (if slower)
 *                versions of math operations with our own little
 *                version of errno.
 *                At present, not all functions here are completely safe.
 *                It's floating point. Can they ever be?
 */

/*
 *  When #including safe.h, make sure these files are #included first:
 *         #include "compiler.h"
 *         #include "functype.h"
 */


#ifndef safe__already_included
#define safe__already_included


/*extern int safe_print_errors;*/

extern void safe_error_to_stderr(enum safe_err *);

/*
 *  The variable safe_err will be set to the appropriate value whenever
 *  an undefined numerical calculation is attempted.
 *  In this case, if the variable safe_print_errors is set to TRUE,
 *  a description of the error will be printed to stderr.
 *  In order to monitor errors effectively, it is
 *  the responsibility of the user to set the safe_err variable to safe_ok
 *  before calling any of the functions below.
 *  (it's our on silly version of errno...)
 */

#define safe_ERF_COEF    1.1283791670955130   /* = 2 / sqrt(PI) */
#define safe_LOG10_COEF  0.4342944819032518   /* = log10(e) = 1/ln(10) */
#define safe_PI          3.1415926535897930

extern double safe_upower(double, unsigned, enum safe_err *); /* buggy */
extern double safe_factorial(unsigned,enum safe_err *);
extern double safe_rec(double,enum safe_err *);
extern double safe_cube(register double,enum safe_err *);
extern double safe_Dn_rec(double,int,enum safe_err *);
#ifdef HAVE_ERF
extern double safe_erf_inv(double,enum safe_err *);
#endif /* HAVE_ERF */
extern double safe_lnm_inv(double,enum safe_err *);
extern int safe_is_int(double,enum safe_err *);
/*
 *  value = safe_upower(x,n,safe)   Compute x^n. Not Verified Code!
 *  value = safe_factorial(n,safe)  Compute n!.
 *  value = safe_rec(x,safe)        Compute 1/x.
 *  value = safe_cube(x,safe)       Compute x^3 with range check
 *  value = safe_Dn_rec(x,n,safe)   Compute n-th derivative of 1/x.
 *  value = safe_erf_inv(x,safe)    Compute inverse of erf.
 *  isint = safe_is_int(x,safe)     0,1 ==> even/odd int,else ==> not an int.
 *  double value,x;
 *  unsigned n;
 *  enum safe_err *safe
 *  int isint;
 */

/*  All of the following safe_XXXX_Di must resolve to C functions
 *  rather than macros because safe_func_Di takes the address of them.
 */

extern double safe_sin_D0(double,enum safe_err *);
extern double safe_sinh_D0(double,enum safe_err *);
extern double safe_cos_D0(double,enum safe_err *);
extern double safe_cosh_D0(double,enum safe_err *);
extern double safe_tan_D0(double,enum safe_err *);
extern double safe_tanh_D0(double,enum safe_err *);
extern double safe_arctan_D0(double,enum safe_err *);
extern double safe_arcsin_D0(double,enum safe_err *);
extern double safe_arcsinh_D0(double,enum safe_err *);
extern double safe_arccos_D0(double,enum safe_err *);
extern double safe_arccosh_D0(double,enum safe_err *);
extern double safe_arctanh_D0(double,enum safe_err *);
#ifdef HAVE_ERF
extern double safe_erf_D0(double,enum safe_err *);
#endif /* HAVE_ERF */
extern double safe_exp_D0(double,enum safe_err *);
extern double safe_ln_D0(double,enum safe_err *);
extern double safe_lnm_D0(double,enum safe_err *);
extern double safe_log_D0(double,enum safe_err *);
extern double safe_sqr_D0(double,enum safe_err *);
extern double safe_sqrt_D0(double,enum safe_err *);
extern double safe_cbrt_D0(double,enum safe_err *);
extern double safe_fabs_D0(double,enum safe_err *);
extern double safe_hold_D0(double,enum safe_err *);
/*
 *  value = safe_<uop>_D0(x)
 *  double value,x;
 *  enum safe_err *safe
 *
 *  Computes value of unary operator.
 */

extern double safe_sin_D1(double,enum safe_err *);
extern double safe_sinh_D1(double,enum safe_err *);
extern double safe_cos_D1(double,enum safe_err *);
extern double safe_cosh_D1(double,enum safe_err *);
extern double safe_tan_D1(double,enum safe_err *);
extern double safe_tanh_D1(double,enum safe_err *);
extern double safe_arcsin_D1(double,enum safe_err *);
extern double safe_arcsinh_D1(double,enum safe_err *);
extern double safe_arccos_D1(double,enum safe_err *);
extern double safe_arccosh_D1(double,enum safe_err *);
extern double safe_arctan_D1(double,enum safe_err *);
extern double safe_arctanh_D1(double,enum safe_err *);
#ifdef HAVE_ERF
extern double safe_erf_D1(double,enum safe_err *);
#endif /* HAVE_ERF */
extern double safe_exp_D1(double,enum safe_err *);
extern double safe_ln_D1(double,enum safe_err *);
extern double safe_lnm_D1(double,enum safe_err *);
extern double safe_log_D1(double,enum safe_err *);
extern double safe_sqr_D1(double,enum safe_err *);
extern double safe_sqrt_D1(double,enum safe_err *);
extern double safe_cube_D0(double,enum safe_err *);
extern double safe_cube_D1(double,enum safe_err *);
extern double safe_cbrt_D1(double,enum safe_err *);
extern double safe_fabs_D1(double,enum safe_err *);
extern double safe_hold_D1(double,enum safe_err *);
/*
 *  value = safe_<uop>_D1(x)
 *  double value,x;
 *  enum safe_err *safe
 *
 *  Computes first derivative of unary operator.
 */


extern double safe_sin_D2(double,enum safe_err *);
extern double safe_sinh_D2(double,enum safe_err *);
extern double safe_cos_D2(double,enum safe_err *);
extern double safe_cosh_D2(double,enum safe_err *);
extern double safe_tan_D2(double,enum safe_err *);
extern double safe_tanh_D2(double,enum safe_err *);
extern double safe_arcsin_D2(double,enum safe_err *);
extern double safe_arcsinh_D2(double,enum safe_err *);
extern double safe_arccos_D2(double,enum safe_err *);
extern double safe_arccosh_D2(double,enum safe_err *);
extern double safe_arctan_D2(double,enum safe_err *);
extern double safe_arctanh_D2(double,enum safe_err *);
#ifdef HAVE_ERF
extern double safe_erf_D2(double,enum safe_err *);
#endif /* HAVE_ERF */
extern double safe_exp_D2(double,enum safe_err *);
extern double safe_ln_D2(double,enum safe_err *);
extern double safe_lnm_D2(double,enum safe_err *);
extern double safe_log_D2(double,enum safe_err *);
extern double safe_sqr_D2(double,enum safe_err *);
extern double safe_sqrt_D2(double,enum safe_err *);
extern double safe_cube_D2(double,enum safe_err *);
extern double safe_cbrt_D2(double,enum safe_err *);
extern double safe_fabs_D2(double,enum safe_err *);
#define safe_hold_D2 safe_hold_D1
/*
 *  value = safe_<uop>_D2(x)
 *  double value,x;
 *  enum safe_err *safe
 *
 *  Computes second derivative of unary operator.
 */

extern double safe_arcsin_Dn(double,int,enum safe_err *);
extern double safe_arccos_Dn(double,int,enum safe_err *);
extern double safe_arctan_Dn(double,int,enum safe_err *);
extern double safe_cos_Dn(double,int,enum safe_err *);
extern double safe_sin_Dn(double,int,enum safe_err *);
#ifdef HAVE_ERF
extern double safe_erf_Dn(double,int,enum safe_err *);
#endif /* HAVE_ERF */
extern double safe_exp_Dn(double,int,enum safe_err *);
extern double safe_ln_Dn(double,int,enum safe_err *);
extern double safe_log_Dn(double,int,enum safe_err *);
extern double safe_sqr_Dn(double,int,enum safe_err *);
extern double safe_sqrt_Dn(double,int,enum safe_err *);
extern double safe_tan_Dn(double,int,enum safe_err *);
extern double safe_fabs_Dn(double,int,enum safe_err *);
#define safe_hold_Dn safe_hold_D1
/*
 *  value = safe_<uop>_Dn(x,n)
 *  double value,x;
 *  int n;   n >= 0
 *  enum safe_err *safe
 *
 *  Computes n-th derivative of unary operator.
 */

#define safe_add_D0(x,y,safe) ((x)+(y))
#define safe_sub_D0(x,y,safe) ((x)-(y))
#define safe_mul_D0(x,y,safe) ((x)*(y))
#define safe_div_D0(x,y,safe) safe_mul_D0((x),safe_rec(y,safe),(safe))
#define safe_ipow_D0(x,y,safe) asc_ipow((x),ascnint(y))
extern double safe_pow_D0(double,double,enum safe_err *);
/*
 *  value = safe_<binop>_D0(x,y,safe)
 *  double value,x,y;
 *  enum safe_err *safe
 *
 *  Computes x <binop> y and returns the value.
 */

#define safe_add_D1(x,y,wrt,safe) (1.0)
#define safe_sub_D1(x,y,wrt,safe) (1.0 - 2.0*(wrt))
#define safe_mul_D1(x,y,wrt,safe) ((wrt)==0 ? (y) : (x))
extern double safe_div_D1(double,double,int,enum safe_err *);
extern double safe_ipow_D1(double,double,int,enum safe_err *);
extern double safe_pow_D1(double,double,int,enum safe_err *);
/*
 *  diff = safe_<binop>_D1(x,y,wrt)
 *  double diff,x,y;
 *  int wrt;   0 ==> d/dx, 1 ==> d/dy
 *  enum safe_err *safe
 *
 *  Computes derivative of x <binop> y w.r.t. x or y depending on wrt.
 */

#define safe_add_D2(x,y,wrt1,wrt2,safe) (0.0)
#define safe_sub_D2(x,y,wrt1,wrt2,safe) (0.0)
#define safe_mul_D2(x,y,wrt1,wrt2,safe) ((wrt1)!=(wrt2)?1.0:0.0)
extern double safe_div_D2(double,double,int,int,enum safe_err *);
extern double safe_ipow_D2(double, double, int, int,enum safe_err *);
extern double safe_pow_D2(double, double, int, int,enum safe_err *);
/*
 *  diff2 = safe_<binop>_D2(x,y,wrt1,wrt2,safe)
 *  double diff2,x,y;
 *  int wrt1,wrt2;   0 ==> w.r.t 1st arg, 1 ==> w.r.t. 2nd arg
 *  enum safe_err *safe
 *
 *  Computes 2nd derivative of x <binop> y w.r.t. x or y depending on
 *  wrt1 and wrt2. (so you can get d^2(f)/dx^2, d^2(f)/dy^2 or d^2(f)/dy/dx
 *
 *  At least in the CASE of pow/ipow
 *  wrt1 = 0 wrt2 = 0  ==>  d^2f/dx^2
 *  wrt1 = 0 wrt2 = 1  ==>  d^2f/dy/dx
 *  wrt1 = 1 wrt2 = 0  ==>  d^2f/dy/dx
 *  wrt1 = 1 wrt2 = 1  ==>  d^2f/dy^2
 */

extern double safe_add_Dn(double,double,int,int,enum safe_err *);
extern double safe_sub_Dn(double,double,int,int,enum safe_err *);
extern double safe_mul_Dn(double,double,int,int,enum safe_err *);
extern double safe_div_Dn(double,double,int,int,enum safe_err *);
extern double
safe_pow_Dn(double,double,int,int,enum safe_err *); /*this one not verified!*/
/*
 *  diffn = safe_<binop>_Dn(x,y,nwrt0,nwrt1)
 *  double diffn,x,y;
 *  int nwrt0,nwrt1;
 *
 *  Computes nwrt0'th derivative wrt x of the nwrt1'th derivative wrt y
 *  of x <binop> y.
 */

#endif
