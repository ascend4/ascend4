/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: calc.h,v $
 *  Date last modified: $Date: 1997/07/21 17:01:04 $
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/***************************************************************************
 ***  Contents:     Calculation module
 ***
 ***  Authors:      Karl Westerberg
 ***                Joseph Zaher
 ***
 ***  Dates:        06/90 - original version
 ***                08/93 - removed calc_D0, calc_D1, calc_D2, calc_Dn
 ***                        and added them as internal functions of the
 ***                        exprman module.
 ***                04/94 - enhanced error messaging
 ***
 ***  Description:  Operations are provided for computing unary and binary
 ***                functions.  First, second, and nth derivatives may
 ***                also be computed for all of the functions featured.  A
 ***                global boolean variable calc_ok, which is declared
 ***                external so that it may be monitored from other
 ***                modules, will be set to FALSE in the event of a
 ***                calculation error.  Also, to suppress printing of error
 ***                messages in the event of an error, set the global
 ***                variable calc_print_errors to FALSE.
 ***************************************************************************/
#ifndef calc__already_included
#define calc__already_included

extern int32 calc_ok;
extern boolean calc_print_errors;
/**
 ***  The variable calc_ok will be set to FALSE whenever an undefined
 ***  numerical calculation is attempted.  In this case, if the variable
 ***  calc_print_errors is set to TRUE, a description of the error will be
 ***  printed to stderr.  In order to monitor errors effectively, it is
 ***  the responsibility of the user to set the calc_ok variable to TRUE
 ***  before calling any of the functions below. 
 **/

#define calc_ERF_COEF    1.1283791670955130   /* = 2 / sqrt(PI) */
#define calc_LOG10_COEF  0.4342944819032518   /* = log10(e) = 1/ln(10) */
#define calc_PI          3.1415926535897930

extern real64 calc_upower(real64, unsigned);
extern real64 calc_factorial(unsigned);
extern real64 calc_rec();
extern real64 calc_cube();
extern real64 calc_Dn_rec();
#ifdef HAVE_ERF
extern real64 calc_erf_inv();
#endif /* HAVE_ERF */
extern real64 calc_lnm_inv();
extern int calc_is_int();
/**
 ***  value = calc_upower(x,n)   Compute x^n.
 ***  value = calc_factorial(n)  Compute n!.
 ***  value = calc_rec(x)        Compute 1/x.
 ***  value = calc_cube(x)       Compute x^3 with range check
 ***  value = calc_Dn_rec(x,n)   Compute n-th derivative of 1/x.
 ***  value = calc_erf_inv(x)    Compute inverse of erf.
 ***  isint = calc_is_int(x)     0,1 ==> even/odd int, else ==> not an int.
 ***  real64 value,x;
 ***  unsigned n;
 ***  int isint;
 **/

/**  All of the following calc_XXXX_Di must resolve to C functions
 ***  rather than macros because calc_func_Di takes the address of them.
 **/

#define calc_sin_D0      sin
#define calc_sinh_D0     sinh
#define calc_cos_D0      cos
#define calc_cosh_D0     cosh
extern real64 calc_tan_D0();
#define calc_tanh_D0     tanh
extern real64 calc_arcsin_D0();
#define calc_arcsinh_D0  arcsinh
extern real64 calc_arccos_D0();
extern real64 calc_arccosh_D0();
#define calc_arctan_D0   atan
extern real64 calc_arctanh_D0();
#ifdef HAVE_ERF
#define calc_erf_D0      erf
#endif /* HAVE_ERF */
extern real64 calc_exp_D0();
extern real64 calc_ln_D0();
#define calc_lnm_D0      lnm
extern real64 calc_log_D0();
#define calc_sqr_D0      sqr
extern real64 calc_sqrt_D0();
#define calc_cbrt_D0     cbrt
#define calc_fabs_D0     fabs
/**
 ***  value = calc_<uop>_D0(x)
 ***  real64 value,x;
 ***
 ***  Computes value of unary operator.
 **/

#define calc_sin_D1      cos
#define calc_sinh_D1     cosh
#define calc_cos_D1      dcos
#define calc_cosh_D1     sinh
extern real64 calc_tan_D1(double);
#define calc_tanh_D1     dtanh
extern real64 calc_arcsin_D1(double);
#define calc_arcsinh_D1  darcsinh
extern real64 calc_arccos_D1(double);
extern real64 calc_arccosh_D1(double);
#define calc_arctan_D1   datan
extern real64 calc_arctanh_D1(double);
#ifdef HAVE_ERF
extern real64 calc_erf_D1(double);
#endif /* HAVE_ERF */
#define calc_exp_D1      calc_exp_D0
#define calc_ln_D1       calc_rec
#define calc_lnm_D1      dlnm
extern real64 calc_log_D1(double);
#define calc_sqr_D1      dsqr
extern real64 calc_sqrt_D1(double);
extern real64 calc_cbrt_D1(double);
extern real64 calc_fabs_D1(double);
/**
 ***  value = calc_<uop>_D1(x)
 ***  real64 value,x;
 ***
 ***  Computes first derivative of unary operator.
 **/


#define calc_sin_D2      dcos
#define calc_sinh_D2     sinh
#define calc_cos_D2      dcos2
#define calc_cosh_D2     cosh
extern real64 calc_tan_D2();
#define calc_tanh_D2     dtanh2
extern real64 calc_arcsin_D2();
#define calc_arcsinh_D2  darcsinh2
extern real64 calc_arccos_D2();
extern real64 calc_arccosh_D2();
#define calc_arctan_D2   datan2
extern real64 calc_arctanh_D2();
#ifdef HAVE_ERF
extern real64 calc_erf_D2();
#endif /* HAVE_ERF */
#define calc_exp_D2      calc_exp_D0
extern real64 calc_ln_D2();
#define calc_lnm_D2      dlnm2
extern real64 calc_log_D2();
#define calc_sqr_D2      dsqr2
extern real64 calc_sqrt_D2();
extern real64 calc_cbrt_D2();
extern real64 calc_fabs_D2();
/**
 ***  value = calc_<uop>_D2(x)
 ***  real64 value,x;
 ***
 ***  Computes second derivative of unary operator.
 **/

extern real64 calc_arcsin_Dn();
extern real64 calc_arccos_Dn();
extern real64 calc_arctan_Dn();
extern real64 calc_cos_Dn();
extern real64 calc_sin_Dn();
#ifdef HAVE_ERF
extern real64 calc_erf_Dn();
#endif /* HAVE_ERF */
extern real64 calc_exp_Dn();
extern real64 calc_ln_Dn();
extern real64 calc_log_Dn();
extern real64 calc_sqr_Dn();
extern real64 calc_sqrt_Dn();
extern real64 calc_tan_Dn();
extern real64 calc_fabs_Dn();
/**
 ***  value = calc_<uop>_Dn(x,n)
 ***  real64 value,x;
 ***  int n;   n >= 0
 ***
 ***  Computes n-th derivative of unary operator.
 **/

#define calc_add_D0(x,y) ((x)+(y))
#define calc_sub_D0(x,y) ((x)-(y))
#define calc_mul_D0(x,y) ((x)*(y))
#define calc_div_D0(x,y) calc_mul_D0(x,calc_rec(y))
#define calc_ipow_D0(x,y) asc_ipow(x,(int)y)
extern real64 calc_pow_D0();
/**
 ***  value = calc_<binop>_D0(x,y)
 ***  real64 value,x,y;
 ***
 ***  Computes x <binop> y and returns the value.
 **/

#define calc_add_D1(x,y,wrt) (1.0)
#define calc_sub_D1(x,y,wrt) (1.0 - 2.0*(wrt))
#define calc_mul_D1(x,y,wrt) ((wrt)==0 ? ((x),(y)) : ((y),(x)))
extern real64 calc_div_D1();
#define calc_ipow_D1(x,y) asc_d1ipow(x,(int)y)     
extern real64 calc_pow_D1();
/**
 ***  diff = calc_<binop>_D1(x,y,wrt)
 ***  real64 diff,x,y;
 ***  int wrt;   0 ==> w.r.t 1st arg, 1 ==> w.r.t. 2nd arg
 ***
 ***  Computes derivative of x <binop> y w.r.t. x or y depending on wrt.
 **/

#define calc_add_D2(x,y,wrt1,wrt2) (0.0)
#define calc_sub_D2(x,y,wrt1,wrt2) (0.0)
#define calc_mul_D2(x,y,wrt1,wrt2) ((wrt1)!=(wrt2)?1.0:0.0)
extern real64 calc_div_D2();
#define calc_ipow_D2(x,y) asc_d2ipow(x,(int)y)          
extern real64 calc_pow_D2();
/**
 ***  diff2 = calc_<binop>_D2(x,y,wrt1,wrt2)
 ***  real64 diff2,x,y;
 ***  int wrt1,wrt2;   0 ==> w.r.t 1st arg, 1 ==> w.r.t. 2nd arg
 ***
 ***  Computes 2nd derivative of x <binop> y w.r.t. x or y depending on
 ***  wrt1 and wrt2.
 **/

extern real64 calc_add_Dn();
extern real64 calc_sub_Dn();
extern real64 calc_mul_Dn();
extern real64 calc_div_Dn();
extern real64 calc_pow_Dn();
/**
 ***  diffn = calc_<binop>_Dn(x,y,nwrt0,nwrt1)
 ***  real64 diffn,x,y;
 ***  int nwrt0,nwrt1;
 ***
 ***  Computes nwrt0'th derivative wrt x of the nwrt1'th derivative wrt y
 ***  of x <binop> y.
 **/

#endif
