/*
 *  Rounded Arithmetic Routines
 *  by Tom Epperly
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: rounded.h,v $
 *  Date last modified: $Date: 1997/07/30 12:16:59 $
 *  Last modified by: $Author: mthomas $
 *  6/28/89
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#ifndef __ROUNDED_H_SEEN__
#define __ROUNDED_H_SEEN__

extern double DIPow(register double d, long int n);
/*
 *  double DIPow(d,n)
 *  double d;
 *  long n;
 *  Return d^n rounded downward.
 */

extern double UIPow(register double d, long int n);
/*
 *  double UIPow(d,n)
 *  double d;
 *  long n;
 *  Return d^n rounded upward.
 */

#ifndef SLOPPY
extern double DPow(register double d1, register double d2);
/*
 *  double DPow(d1,d2)
 *  double d1,d2;
 *  Return the downward rounded result of d1^d2(d1 raised to the power d2).
 */

extern double UPow(register double d1, register double d2);
/*
 *  double UPow(d1,d2)
 *  double d1,d2;
 *  Return the downward rounded result of d1^d2(d1 raised to the power d2).
 */

extern double DPlus(register double d1, register double d2);
/*
 *  double DPlus(d1,d2)
 *  double d1,d2;
 *  Return the downward rounded addition of d1 and d2.
 */

extern double UPlus(register double d1, register double d2);
/*
 *  double UPlus(d1,d2)
 *  double d1,d2;
 *  Return the upward rounded addition of d1 and d2.
 */

extern double DMinus(register double d1, register double d2);
/*
 *  double DMinus(d1,d2)
 *  double d1,d2;
 *  Return d1-d2 with downward rounding.
 */

extern double UMinus(register double d1, register double d2);
/*
 *  double UMinus(d1,d2)
 *  double d1,d2;
 *  Return d1-d2 with upward rounding.
 */

extern double DTimes(register double d1, register double d2);
/*
 *  double DTimes(d1,d2)
 *  double d1,d2;
 *  return d1*d2 with downward rounding.
 */

extern double UTimes(register double d1, register double d2);
/*
 *  double UTimes(d1,d2)
 *  double d1,d2;
 *  return d1*d2 with upward rounding
 */

extern double DDivide(register double d1, register double d2);
/*
 *  double DDivide(d1,d2);
 *  double d1,d2;
 *  return d1/d2 with downward rounding.
 */

extern double UDivide(register double d1, register double d2);
/*
 *  double UDivide(d1,d2);
 *  double d1,d2;
 *  return d1/d2 with upward rounding.
 */

extern double DSqr(register double d);
/*
 *  double DSqr(d)
 *  double d;
 *  return sqr(d) with downward rounding.
 */

extern double USqr(register double d);
/*
 *  double USqr(d)
 *  double d;
 *  return sqr(d) with upward rounding.
 */

extern double DSqrt(register double d);
/*
 *  double DSqrt(d);
 *  double d;
 *  return sqrt(d) with downward rounding.
 */

extern double USqrt(register double d);
/*
 *  double USqrt(d)
 *  double d;
 *  return sqrt(d) with upward rounding.
 */

extern double DExp(register double d);
/*
 *  double DExp(d)
 *  double d;
 *  return exp(d) with downward rounding
 */

extern double UExp(register double d);
/*
 *  double UExp(d)
 *  double d;
 *  return exp(d) with upward rounding.
 */

extern double DLog(register double d);
/*
 *  double DLog(d)
 *  double d;
 *  return log base 10 of d rounded down
 */

extern double ULog(register double d);
/*
 *  double ULog(d)
 *  double d;
 *  return the log base 10 of d.
 */

extern double DLn(register double d);
/*
 *  double DLn(d)
 *  double d;
 *  return ln(d) (log base e) with downward rounding.
 */

extern double ULn(register double d);
/*
 *  double ULn(d)
 *  double d;
 *  return ln(d) with upward rounding.
 */

#ifdef HAVE_ERF
extern double DErf(register double d);
/*
 *  double DErf(d)
 *  double d;
 *  return erf(d) with downward rounding.
 */

extern double UErf(register double d);
/*
 *  double UErf(d)
 *  double d;
 *  return erf(d) with upward rounding.
 */
#endif /* HAVE_ERF */

extern double Dltod(long int l);
/*
 *  double Dltod(l)
 *  long l;
 */

extern double Ultod(long int l);
/*
 *  double Ultod(l);
 *  long l
 */
#else /* SLOPPY */
#define Dltod(l) ((double)l)
#define Ultod(l) ((double)l)
#define DPlus(d1,d2) ((d1)+(d2))
#define UPlus(d1,d2) ((d1)+(d2))
#define DMinus(d1,d2) ((d1)-(d2))
#define UMinus(d1,d2) ((d1)-(d2))
#define DTimes(d1,d2) ((d1)*(d2))
#define UTimes(d1,d2) ((d1)*(d2))
#define DDivide(d1,d2) ((d1)/(d2))
#define UDivide(d1,d2) ((d1)/(d2))
#define DSqr(d) ((d)*(d))
#define USqr(d) ((d)*(d))
#define DSqrt(d) (sqrt(d))
#define USqrt(d) (sqrt(d))
#define DExp(d) (exp(d))
#define UExp(d) (exp(d))
#define DLn(d) (log(d))
#define ULn(d) (log(d))
#define DLog(d) (log10(d))
#define ULog(d) (log10(d))
#ifdef HAVE_ERF
#define DErf(d) (erf(d))
#define UErf(d) (erf(d))
#endif /* HAVE_ERF */
#define DPow(d1,d2) (pow(d1,d2))
#define UPow(d1,d2) (pow(d1,d2))
#endif /* SLOPPY */

#endif /* __ROUNDED_H_SEEN__ */
