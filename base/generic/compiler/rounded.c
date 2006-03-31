/*
 *  Rounded arithmetic routines
 *  by Tom Epperly
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: rounded.c,v $
 *  Date last modified: $Date: 1997/07/21 16:56:47 $
 *  Last modified by: $Author: kt2g $
 *  2/28/89
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

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include "rounded.h"
#define IsOdd(i) ((i & 1)!=0)

#ifndef lint
static CONST char RoundedRCSid[] = "$Id: rounded.c,v 1.6 1997/07/21 16:56:47 kt2g Exp $";
#endif

#ifndef SLOPPY

#ifdef mips
#include <mips/fpu.h>

union fpc_csr global_fpc_csr , global_temp_fpc_csr;

#define CHANGEROUNDINGMODE(x) global_fpc_csr.fc_word = get_fpc_csr(); \
  global_temp_fpc_csr.fc_word = global_fpc_csr.fc_word;\
  global_temp_fpc_csr.fc_struct.rounding_mode = x;\
  set_fpc_csr(global_temp_fpc_csr.fc_word)
#define RESETROUNDINGMODE set_fpc_csr(global_fpc_csr)

#else

#include <ieeefp.h>

fp_rnd global_rnd_mode;

#define CHANGEROUNDINGMODE(x) global_rnd_mode = fpgetround();\
  fpsetround(x)
#define RESETROUNDINGMODE fpsetround(global_rnd_mode)
#define ROUND_TO_MINUS_INFINITY FP_RM
#define ROUND_TO_PLUS_INFINITY FP_RP

#endif

double DPlus(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = d1+d2;
  RESETROUNDINGMODE;
  return result;
}

double UPlus(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = d1+d2;
  RESETROUNDINGMODE;
  return result;
}

double DMinus(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = d1-d2;
  RESETROUNDINGMODE;
  return result;
}

double UMinus(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = d1-d2;
  RESETROUNDINGMODE;
  return result;
}

double DPow(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = pow(d1,d2);
  RESETROUNDINGMODE;
  return result;
}

double UPow(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = pow(d1,d2);
  RESETROUNDINGMODE;
  return result;
}

double DTimes(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = d1*d2;
  RESETROUNDINGMODE;
  return result;
}

double UTimes(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = d1*d2;
  RESETROUNDINGMODE;
  return result;
}

double DDivide(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = d1/d2;
  RESETROUNDINGMODE;
  return result;
}

double UDivide(register double d1, register double d2)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = d1/d2;
  RESETROUNDINGMODE;
  return result;
}

double DSqr(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d *= d;
  RESETROUNDINGMODE;
  return d;
}

double USqr(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d *= d;
  RESETROUNDINGMODE;
  return d;
}

double DSqrt(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d = sqrt(d);
  RESETROUNDINGMODE;
  return d;
}

double USqrt(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d = sqrt(d);
  RESETROUNDINGMODE;
  return d;
}

double DExp(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d = exp(d);
  RESETROUNDINGMODE;
  return d;
}

double UExp(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d = exp(d);
  RESETROUNDINGMODE;
  return d;
}

double DLog(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d = log10(d);
  RESETROUNDINGMODE;
  return d;
}

double ULog(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d = log10(d);
  RESETROUNDINGMODE;
  return d;
}

#ifdef HAVE_ERF
double DErf(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d = erf(d);
  RESETROUNDINGMODE;
  return d;
}

double UErf(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d = erf(d);
  RESETROUNDINGMODE;
  return d;
}
#endif /* HAVE_ERF */

double DLn(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  d = log(d);
  RESETROUNDINGMODE;
  return d;
}

double ULn(register double d)
{
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  d = log(d);
  RESETROUNDINGMODE;
  return d;
}

double DIPow(register double d, long int n)
{
  register double result;
  int negative;
  if (n==0) return 1.0;
  if ((n<0)&&(d==0.0)) {
    Asc_Panic(2, NULL, "Zero raised to a zero power.\n");
  }
  if (d==0.0) return 0.0;
  negative = (d<0.0) &&(IsOdd(n));
  d = fabs(d);
  result = 1.0;
  if (negative) { CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY); }
  else { CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY); }
  if (n < 0)
    while(n++<0) result /= d;
  else
    while(n-->0) result *= d;
  RESETROUNDINGMODE;
  assert(result==result);
  return (negative) ? -result : result ;
}

double UIPow(register double d, long int n)
{
  register double result;
  int negative;
  if (n==0) return 1.0;
  if ((n<0)&&(d==0.0)) {
    Asc_Panic(2, NULL,
              "Zero raised to a zero power.\n");
  }
  if (d==0.0) return 0.0;
  negative = (d<0.0) && (IsOdd(n));
  d = fabs(d);
  result = 1.0;
  if (negative) { CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY); }
  else { CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY); }
  if (n < 0)
    while(n++<0) result /= d;
  else
    while(n-->0) result *= d;
  RESETROUNDINGMODE;
  assert(result==result);
  return (negative) ? -result : result;
}

double Dltod(long int l)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_MINUS_INFINITY);
  result = (double)l;
  RESETROUNDINGMODE;
  return result;
}

double Ultod(long int l)
{
  register double result;
  CHANGEROUNDINGMODE(ROUND_TO_PLUS_INFINITY);
  result = (double)l;
  RESETROUNDINGMODE;
  return result;
}

#else
double DIPow(register double d, long n)
{
  register double result;
  if (n==0) return 1.0;
  if ((n<0)&&(d==0.0)) {
    Asc_Panic(2, NULL, "Zero raised to a zero power.\n");
  }
  if (d==0.0) return 0.0;
  result = 1.0;
  if (n < 0)
    while(n++<0) result /= d;
  else
    while(n-->0) result *= d;
  assert(result==result);
  return result;
}

double UIPow(register double d, long n)
{
  register double result;
  if (n==0) return 1.0;
  if ((n<0)&&(d==0.0)) {
    Asc_Panic(2, NULL, "Zero raised to a zero power.\n");
  }
  if (d==0.0) {
    return 0.0;
  }
  result = 1.0;
  if (n < 0) {
    while(n++<0) result /= d;
  } else {
    while(n-->0) result *= d;
  }
  assert(result==result);
  return result;
}

#endif /* SLOPPY */
