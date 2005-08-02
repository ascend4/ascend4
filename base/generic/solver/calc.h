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

/** @file
 *  SLV: Ascend Numeric Solver - Calculation module.
 *  <pre>
 *  Contents:     Calculation module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                08/93 - removed calc_D0, calc_D1, calc_D2, calc_Dn
 *                        and added them as internal functions of the
 *                        exprman module.
 *                04/94 - enhanced error messaging
 *
 *  Description:  Operations are provided for computing unary and binary
 *                functions.  First, second, and nth derivatives may
 *                also be computed for all of the functions featured.  A
 *                global boolean variable calc_ok, which is declared
 *                external so that it may be monitored from other
 *                modules, will be set to FALSE in the event of a
 *                calculation error.  Also, to suppress printing of error
 *                messages in the event of an error, set the global
 *                variable calc_print_errors to FALSE.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *  </pre>
 *  @todo Complete documentation of solver/calc.h.
 */

#ifndef calc__already_included
#define calc__already_included

extern int32 calc_ok;
/**<
 ***  The variable calc_ok will be set to FALSE whenever an undefined
 ***  numerical calculation is attempted.
 */

extern boolean calc_print_errors;
/**<  When calc_ok false, if the variable
 ***  calc_print_errors is set to TRUE, a description of the error will be
 ***  printed to stderr.  In order to monitor errors effectively, it is
 ***  the responsibility of the user to set the calc_ok variable to TRUE
 ***  before calling any of the functions below. 
 **/

#define calc_ERF_COEF    1.1283791670955130   /**<  = 2 / sqrt(PI) */
#define calc_LOG10_COEF  0.4342944819032518   /**< = log10(e) = 1/ln(10) */
#define calc_PI          3.1415926535897930   /**< really should be using M_PI */

extern real64 calc_upower(real64 x, unsigned n); /**< Compute x^n. */
extern real64 calc_factorial(unsigned n);        /**< Compute n!. */
extern real64 calc_rec();                        /**< Compute 1/x. */
extern real64 calc_cube();                       /**< Compute x^3 with range check. */
extern real64 calc_Dn_rec();                     /**< Compute n-th derivative of 1/x. */
#ifdef HAVE_ERF
extern real64 calc_erf_inv();                    /**< Compute inverse of erf. */
#endif /* HAVE_ERF */
extern real64 calc_lnm_inv();                    /**< Compute inverse of lnm */
extern int calc_is_int();                        /**< 0,1 ==> even/odd int, else ==> not an int. */

/*  All of the following calc_XXXX_Di must resolve to C functions
 *  rather than macros because calc_func_Di takes the address of them.
 */

#define calc_sin_D0      sin              /**< Compute sin. */
#define calc_sinh_D0     sinh             /**< Compute hyperbolic sin. */
#define calc_cos_D0      cos              /**< Compute cos. */
#define calc_cosh_D0     cosh             /**< Compute hyperbolic cos. */
extern real64 calc_tan_D0(real64 x);      /**< Compute tan(x). */
#define calc_tanh_D0     tanh             /**< Compute hyperbolic tan. */
extern real64 calc_arcsin_D0(real64 x);   /**< Compute arcsin(x). */
#define calc_arcsinh_D0  arcsinh          /**< Compute hyperbolic arcsin. */
extern real64 calc_arccos_D0(real64 x);   /**< Compute arccos(x). */
extern real64 calc_arccosh_D0(real64 x);  /**< Compute hyperbolic arccos(x). */
#define calc_arctan_D0   atan             /**< Compute arctan. */
extern real64 calc_arctanh_D0(real64 x);  /**< Compute hyperbolic arctan(x). */
#ifdef HAVE_ERF
#define calc_erf_D0      erf              /**< Compute erf. */
#endif /* HAVE_ERF */
extern real64 calc_exp_D0(real64 x);      /**< Compute exp(x). */
extern real64 calc_ln_D0(real64 x);       /**< Compute ln(x). */
#define calc_lnm_D0      lnm              /**< Compute lnm. */
extern real64 calc_log_D0(real64 x);      /**< Compute log(x). */
#define calc_sqr_D0      sqr              /**< Compute square. */
extern real64 calc_sqrt_D0(real64 x);     /**< Compute sqrt(x). */
#define calc_cbrt_D0     cbrt             /**< Compute cube root. */
#define calc_fabs_D0     fabs             /**< Compute abs value. */
/*
 ***  value = calc_<uop>_D0(x)
 ***  real64 value,x;
 ***
 ***  Computes value of unary operator.
 **/

#define calc_sin_D1      cos              /**< Computes 1st derivative of sin. */
#define calc_sinh_D1     cosh             /**< Computes 1st derivative of hyperbolic sin. */
#define calc_cos_D1      dcos             /**< Computes 1st derivative of cos. */
#define calc_cosh_D1     sinh             /**< Computes 1st derivative of hyperbolic cos. */
extern real64 calc_tan_D1(real64 x);      /**< Computes 1st derivative of tan(x). */
#define calc_tanh_D1     dtanh            /**< Computes 1st derivative of hyperbolic tan. */
extern real64 calc_arcsin_D1(real64 x);   /**< Computes 1st derivative of arcsin(x). */
#define calc_arcsinh_D1  darcsinh         /**< Computes 1st derivative of hyperbolic arcsin. */
extern real64 calc_arccos_D1(real64 x);   /**< Computes 1st derivative of arccos(x). */
extern real64 calc_arccosh_D1(real64 x);  /**< Computes 1st derivative of hyperbolic arccos(x). */
#define calc_arctan_D1   datan            /**< Computes 1st derivative of arctan. */
extern real64 calc_arctanh_D1(real64 x);  /**< Computes 1st derivative of hyperbolic arctan(x). */
#ifdef HAVE_ERF
extern real64 calc_erf_D1(real64 x);      /**< Computes 1st derivative of erf. */
#endif /* HAVE_ERF */
#define calc_exp_D1      calc_exp_D0      /**< Computes 1st derivative of exp(x). */
#define calc_ln_D1       calc_rec         /**< Computes 1st derivative of ln(x). */
#define calc_lnm_D1      dlnm             /**< Computes 1st derivative of lnm. */
extern real64 calc_log_D1(real64 x);      /**< Computes 1st derivative of log(x). */
#define calc_sqr_D1      dsqr             /**< Computes 1st derivative of square. */
extern real64 calc_sqrt_D1(real64 x);     /**< Computes 1st derivative of sqrt(x). */
extern real64 calc_cbrt_D1(real64 x);     /**< Computes 1st derivative of cube root. */
extern real64 calc_fabs_D1(real64 x);     /**< Computes 1st derivative of abs value. */
/*
 ***  value = calc_<uop>_D1(x)
 ***  real64 value,x;
 ***
 ***  Computes first derivative of unary operator.
 **/

#define calc_sin_D2      dcos             /**< Computes 2nd derivative of sin. */
#define calc_sinh_D2     sinh             /**< Computes 2nd derivative of hyperbolic sin. */
#define calc_cos_D2      dcos2            /**< Computes 2nd derivative of cos. */
#define calc_cosh_D2     cosh             /**< Computes 2nd derivative of hyperbolic cos. */
extern real64 calc_tan_D2(real64 x);      /**< Computes 2nd derivative of tan(x). */
#define calc_tanh_D2     dtanh2           /**< Computes 2nd derivative of hyperbolic tan. */
extern real64 calc_arcsin_D2(real64 x);   /**< Computes 2nd derivative of arcsin(x). */
#define calc_arcsinh_D2  darcsinh2        /**< Computes 2nd derivative of hyperbolic arcsin. */
extern real64 calc_arccos_D2(real64 x);   /**< Computes 2nd derivative of arccos(x). */
extern real64 calc_arccosh_D2(real64 x);  /**< Computes 2nd derivative of hyperbolic arccos(x). */
#define calc_arctan_D2   datan2           /**< Computes 2nd derivative of arctan. */
extern real64 calc_arctanh_D2(real64 x);  /**< Computes 2nd derivative of hyperbolic arctan(x). */
#ifdef HAVE_ERF
extern real64 calc_erf_D2(real64 x);      /**< Computes 2nd derivative of erf. */
#endif /* HAVE_ERF */
#define calc_exp_D2      calc_exp_D0      /**< Computes 2nd derivative of exp(x). */
extern real64 calc_ln_D2(real64 x);       /**< Computes 2nd derivative of ln(x). */
#define calc_lnm_D2      dlnm2            /**< Computes 2nd derivative of lnm. */
extern real64 calc_log_D2(real64 x);      /**< Computes 2nd derivative of log(x). */
#define calc_sqr_D2      dsqr2            /**< Computes 2nd derivative of square. */
extern real64 calc_sqrt_D2(real64 x);     /**< Computes 2nd derivative of sqrt(x). */
extern real64 calc_cbrt_D2(real64 x);     /**< Computes 2nd derivative of cube root. */
extern real64 calc_fabs_D2(real64 x);     /**< Computes 2nd derivative of abs value. */
/*
 ***  value = calc_<uop>_D2(x)
 ***  real64 value,x;
 ***
 ***  Computes second derivative of unary operator.
 **/

extern real64 calc_arcsin_Dn(real64 x, int n);  /**< Computes nth derivative of arcsin(x). */
extern real64 calc_arccos_Dn(real64 x, int n);  /**< Computes nth derivative of arccos(x). */
extern real64 calc_arctan_Dn(real64 x, int n);  /**< Computes nth derivative of arctan(x). */
extern real64 calc_cos_Dn(real64 x, int n);     /**< Computes nth derivative of cos(x). */
extern real64 calc_sin_Dn(real64 x, int n);     /**< Computes nth derivative of sin(x). */
#ifdef HAVE_ERF
extern real64 calc_erf_Dn(real64 x, int n);     /**< Computes nth derivative of erf(x). */
#endif /* HAVE_ERF */
extern real64 calc_exp_Dn(real64 x, int n);     /**< Computes nth derivative of exp(x). */
extern real64 calc_ln_Dn(real64 x, int n);      /**< Computes nth derivative of ln(x). */
extern real64 calc_log_Dn(real64 x, int n);     /**< Computes nth derivative of log(x). */
extern real64 calc_sqr_Dn(real64 x, int n);     /**< Computes nth derivative of square(x). */
extern real64 calc_sqrt_Dn(real64 x, int n);    /**< Computes nth derivative of sqrt(x). */
extern real64 calc_tan_Dn(real64 x, int n);     /**< Computes nth derivative of tan(x). */
extern real64 calc_fabs_Dn(real64 x, int n);    /**< Computes nth derivative of abs value(x). */
/*
 ***  value = calc_<uop>_Dn(x,n)
 ***  real64 value,x;
 ***  int n;   n >= 0
 ***
 ***  Computes n-th derivative of unary operator.
 **/

#define calc_add_D0(x,y) ((x)+(y))                  /**< Computes (x + y). */
#define calc_sub_D0(x,y) ((x)-(y))                  /**< Computes (x - y). */
#define calc_mul_D0(x,y) ((x)*(y))                  /**< Computes (x * y). */
#define calc_div_D0(x,y) calc_mul_D0(x,calc_rec(y)) /**< Computes (x/y). */
#define calc_ipow_D0(x,y) asc_ipow(x,(int)y)        /**< Computes (x^(int)y). */
extern real64 calc_pow_D0(real64 x, real64 y);      /**< Computes (x^y). */
/*
 ***  value = calc_<binop>_D0(x,y)
 ***  real64 value,x,y;
 ***
 ***  Computes x <binop> y and returns the value.
 **/

#define calc_add_D1(x,y,wrt) (1.0)                              /**< Computes 1st derivatve of (x + y). */
#define calc_sub_D1(x,y,wrt) (1.0 - 2.0*(wrt))                  /**< Computes 1st derivatve of (x - y). */
#define calc_mul_D1(x,y,wrt) ((wrt)==0 ? ((x),(y)) : ((y),(x))) /**< Computes 1st derivatve of (x * y). */
extern real64 calc_div_D1(real64 x, real64 y, int wrt);         /**< Computes 1st derivatve of (x/y). */
#define calc_ipow_D1(x,y) asc_d1ipow(x,(int)y)                  /**< Computes 1st derivatve of (x^(int)y). */
extern real64 calc_pow_D1(real64 x, real64 y, int wrt);         /**< Computes 1st derivatve of (x^y). */
/*
 ***  diff = calc_<binop>_D1(x,y,wrt)
 ***  real64 diff,x,y;
 ***  int wrt;   0 ==> w.r.t 1st arg, 1 ==> w.r.t. 2nd arg
 ***
 ***  Computes derivative of x <binop> y w.r.t. x or y depending on wrt.
 **/

#define calc_add_D2(x,y,wrt1,wrt2) (0.0)                           /**< Computes 2nd derivatve of (x + y). */
#define calc_sub_D2(x,y,wrt1,wrt2) (0.0)                           /**< Computes 2nd derivatve of (x - y). */
#define calc_mul_D2(x,y,wrt1,wrt2) ((wrt1)!=(wrt2)?1.0:0.0)        /**< Computes 2nd derivatve of (x * y). */
extern real64 calc_div_D2(real64 x, real64 y, int wrt1, int wrt2); /**< Computes 2nd derivatve of (x/y). */
#define calc_ipow_D2(x,y) asc_d2ipow(x,(int)y)                     /**< Computes 2nd derivatve of (x^(int)y). */
extern real64 calc_pow_D2(real64 x, real64 y, int wrt1, int wrt2); /**< Computes 2nd derivatve of (x^y). */
/*
 ***  diff2 = calc_<binop>_D2(x,y,wrt1,wrt2)
 ***  real64 diff2,x,y;
 ***  int wrt1,wrt2;   0 ==> w.r.t 1st arg, 1 ==> w.r.t. 2nd arg
 ***
 ***  Computes 2nd derivative of x <binop> y w.r.t. x or y depending on
 ***  wrt1 and wrt2.
 **/

extern real64 calc_add_Dn(real64 x, real64 y, int nwrt0, int nwrt1); /**< Computes nth derivatve of (x + y). */
extern real64 calc_sub_Dn(real64 x, real64 y, int nwrt0, int nwrt1); /**< Computes nth derivatve of (x - y). */
extern real64 calc_mul_Dn(real64 x, real64 y, int nwrt0, int nwrt1); /**< Computes nth derivatve of (x * y). */
extern real64 calc_div_Dn(real64 x, real64 y, int nwrt0, int nwrt1); /**< Computes nth derivatve of (x/y). */
extern real64 calc_pow_Dn(real64 x, real64 y, int nwrt0, int nwrt1); /**< Computes nth derivatve of (x^y). */
/*
 ***  diffn = calc_<binop>_Dn(x,y,nwrt0,nwrt1)
 ***  real64 diffn,x,y;
 ***  int nwrt0,nwrt1;
 ***
 ***  Computes nwrt0'th derivative wrt x of the nwrt1'th derivative wrt y
 ***  of x <binop> y.
 **/

#endif  /* calc__already_included */

