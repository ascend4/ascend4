/*
 *  Function module
 *  by Tom Epperly
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: functype.h,v $
 *  Date last modified: $Date: 2001/01/31 22:23:58 $
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
 *  COPYING.  Revision notes:
 *
 *  If CHRIS_FUNC defined, auxillary quantities to func structure.
 *  2/96 baa Probably somebody should properly set these evaluation
 *  defined below based on math.h when possible.
 *  10/96 moved safe_err definition into here.
 */

/** @file
 *  Function module types and data structures.
 *  <pre>
 *  When #including functype.h, make sure these files are #included first:
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_FUNCTYPE_H
#define ASC_FUNCTYPE_H

/** 
 *  Safe math error codes.
 *  If you add to this enum be sure to add to safe_error_to_stderr 
 */
enum safe_err {
  safe_problem = -1,
  safe_ok = 0,
  safe_div_by_zero = 1,
  safe_complex_result = 2,
  safe_overflow = 3,
  safe_underflow = 4,
  safe_range_error = 5
};

/** Function enumeration. */
enum Func_enum {
   F_LOG10, F_LN, F_EXP,
   F_SIN, F_COS, F_TAN,
   F_ARCSIN, F_ARCCOS, F_ARCTAN,
   F_SQR, F_SQRT,
 #ifdef HAVE_ERF
   F_ERF,
 #endif
   F_LNM, F_SINH, F_COSH, F_TANH,
   F_ARCSINH, F_ARCCOSH, F_ARCTANH,
   F_CUBE, F_CBRT, F_ABS, F_HOLD
};

/** Function data structure. */
struct Func {
  CONST char *name;         /**< ASCEND name of function. not symchar */
  CONST char *cname;        /**< C name of function. not symchar */
  CONST char *deriv1cname;  /**< C name of first derivative. not symchar */
  CONST char *deriv2cname;  /**< C name of second derivative. not symchar */
  enum Func_enum id;        /**< identification of function */
  double (*value)(double);  /**< pointer to function evaluation */
  double (*deriv)(double);  /**< pointer to derivative evaluation */
  double (*deriv2)(double); /**< pointer to a second derivative evaluation */
  double (*safevalue)(double,enum safe_err *); /**< pointer to function evaluation */
  double (*safederiv)(double,enum safe_err *); /**< pointer to derivative evaluation */
  double (*safederiv2)(double,enum safe_err *); /**< pointer to a second derivative evaluation */
#ifdef CHRIS_FUNC
  struct Interval (*ivalue)();  /**< interval evaluation of function */
  void (*slope)(unsigned long,struct Interval *,struct Interval *,
  struct Interval *);           /**< pointer to slope evaluation routine */
  struct Interval (*ideriv)(struct Interval);
    /**< interval derivative evaluation routine */
  double (*tmin)(double,double); /**< return the point where the func is a min */
  double (*tmax)(double,double); /**< return the point where the func is a max */
  double (*e)(double,double,double,double (*)(double));   /**< convex envelope */
  double (*ed)(double,double,double,double (*)(double));  /**< convex envelope derivative */
  double (*E)(double,double,double,double (*)(double));   /**< concave envelope */
  double (*Ed)(double,double,double,double (*)(double));  /**< concave envelope derivative */
#endif
};

#endif   /* ASC_FUNCTYPE_H */

