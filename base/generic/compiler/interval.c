/*
 *  Interval Routine Implementation
 *  by Tom Epperly
 *  Created: 5/29/1990
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: interval.c,v $
 *  Date last modified: $Date: 1998/02/27 16:29:02 $
 *  Last modified by: $Author: mthomas $
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

#include <math.h>
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascPanic.h"
#include "compiler/rounded.h"
#include "compiler/interval.h"
#define TEST(x) assert(((x).low) <= ((x).high))

#ifndef lint
static CONST char IntervalRoutinesRCSid[] = "$Id: interval.c,v 1.7 1998/02/27 16:29:02 mthomas Exp $";
#endif

struct Interval CreateInterval(double low, double high)
{
  struct Interval result;
  result.low = low;
  result.high = high;
  TEST(result);
  return result;
}

struct Interval IntervalFromIntegers(long int low, long int high)
{
  struct Interval result;
  result.low = Dltod(low);
  result.high = Ultod(high);
  TEST(result);
  return result;
}

struct Interval CreateThin(double v)
{
  struct Interval result;
  result.low = v;
  result.high = v;
  TEST(result);
  return result;
}

struct Interval CreateThinInteger(long int l)
{
  struct Interval result;
  result.low = Dltod(l);
  result.high = Ultod(l);
  TEST(result);
  return result;
}

double Mag(struct Interval i)
{
  TEST(i);
  return ((fabs(i.low) > fabs(i.high)) ? fabs(i.low) : fabs(i.high));
}

double Mig(struct Interval i)
{
  TEST(i);
  if ((i.low > 0.0)||(i.high < 0.0)){
    return ((fabs(i.low)<fabs(i.high)) ? fabs(i.low) : fabs(i.high));
  }
  else return 0.0;
}

double Rad(struct Interval i)
{
  TEST(i);
  return UDivide(UMinus(i.high,i.low),2.0);
}

double Mid(struct Interval i)
{
  TEST(i);
  return DPlus(i.low,DDivide(DMinus(i.high,i.low),2.0));
}

struct Interval AddIntervals(struct Interval i1, struct Interval i2)
{
  struct Interval result;
  TEST(i1);
  TEST(i2);
  result.low = DPlus(i1.low,i2.low);
  result.high = UPlus(i1.high,i2.high);
  TEST(result);
  return result;
}

struct Interval SubIntervals(struct Interval i1, struct Interval i2)
{
  struct Interval result;
  TEST(i1);
  TEST(i2);
  result.low = DMinus(i1.low,i2.high);
  result.high = UMinus(i1.high,i2.low);
  TEST(result);
  return result;
}

struct Interval SubtractScaler(struct Interval i, double s)
{
  struct Interval result;
  TEST(i);
  result.low = DMinus(i.low,s);
  result.high = UMinus(i.high,s);
  TEST(result);
  return result;
}

static
double MyIntMin(double d1, double d2)
{
  return ((d1<d2) ? d1 : d2);
}

static
double MyIntMax(double d1, double d2)
{
  return ((d1>d2) ? d1 : d2);
}

struct Interval MulIntervals(struct Interval i1, struct Interval i2)
{
  struct Interval result;
  TEST(i1);
  TEST(i2);
  if (i1.low >= 0.0) {
    if (i2.low >= 0.0) { /* i1 >= 0 && i2 >= 0 */
      result.low = DTimes(i1.low,i2.low);
      result.high = UTimes(i1.high,i2.high);
    }
    else if (i2.high <= 0.0) { /* i1 >= 0 && i2 <= 0 */
      result.low = DTimes(i2.low,i1.high);
      result.high = UTimes(i2.high,i1.low);
    }
    else { /* i1 >= 0 && 0 in i2 */
      result.low = DTimes(i2.low,i1.high);
      result.high = UTimes(i2.high,i1.high);
    }
  }
  else if (i1.high <= 0.0) {
    if (i2.low >= 0.0) { /* i1 <= 0 && i2 >= 0 */
      result.low = DTimes(i2.high,i1.low);
      result.high = UTimes(i2.low,i1.high);
    }
    else if (i2.high <= 0.0) { /* i1 <= 0 && i2 <= 0 */
      result.low = DTimes(i2.high,i1.high);
      result.high = UTimes(i1.low,i2.low);
    }
    else { /* i1 <= 0 && 0 in i2 */
      result.low = DTimes(i2.high,i1.low);
      result.high = UTimes(i2.low,i1.low);
    }
  }
  else {
    if (i2.low >= 0.0) { /* 0 in i1 && i2 >= 0.0 */
      result.low = DTimes(i2.high,i1.low);
      result.high = UTimes(i2.high,i1.high);
    }
    else if (i2.high <= 0.0) { /* i1 contains 0 && i2 <= 0 */
      result.low = DTimes(i2.low,i1.high);
      result.high = UTimes(i2.low,i1.low);
    }
    else { /* 0 in i1 && 0 in i2 */
      result.low = MyIntMin(DTimes(i2.low,i1.high),DTimes(i2.high,i1.low));
      result.high = MyIntMax(UTimes(i2.low,i1.low),UTimes(i2.high,i1.high));
    }
  }
  TEST(result);
  return result;
}

struct Interval ScaleInterval(struct Interval i, double s)
{
  struct Interval result;
  TEST(i);
  if (s >= 0.0){
    result.low = DTimes(i.low,s);
    result.high = UTimes(i.high,s);
  }
  else{
    result.low = DTimes(i.high,s);
    result.high = UTimes(i.low,s);
  }
  TEST(result);
  return result;
}

struct Interval DivIntervals(struct Interval i1, struct Interval i2)
{
  struct Interval result;
  TEST(i1);
  TEST(i2);
  if ((i2.high<0.0)||(i2.low>0.0)) { /* 0 is not in i2 */
    if (i2.low > 0.0) {
      if (i1.low >= 0.0) {
	result.low = DDivide(i1.low,i2.high);
	result.high = UDivide(i1.high,i2.low);
      }
      else if (i1.high <= 0.0) {
	result.low = DDivide(i1.low,i2.low);
	result.high = UDivide(i1.high,i2.high);
      }
      else {
	result.low = DDivide(i1.low,i2.low);
	result.high = UDivide(i1.high,i2.low);
      }
    }
    else {
      if (i1.low >= 0.0) {
	result.low = DDivide(i1.high,i2.high);
	result.high = UDivide(i1.low,i2.low);
      }
      else if (i1.high <= 0.0) {
	result.low = DDivide(i1.high,i2.low);
	result.high = UDivide(i1.low,i2.high);
      }
      else {
	result.low = DDivide(i1.high,i2.high);
	result.high = UDivide(i1.low,i2.low);
      }
    }
  }
  else {
    Asc_Panic(2, NULL,
              "Division by zero in DivIntervals routine!\n"
              "Giving up and dying! RIP!\n");
    exit(2);
  }
  TEST(result);
  return result;
}

struct Interval SqrInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  if (i.high < 0.0){
    result.low = DSqr(i.high);
    result.high = USqr(i.low);
  }
  else if (i.low > 0.0){
    result.low = DSqr(i.low);
    result.high = USqr(i.high);
  }
  else{
    result.low = 0.0;
    if (i.high > (-i.low)) result.high = USqr(i.high);
    else result.high = USqr(i.low);
  }
  TEST(result);
  return result;
}

struct Interval SqrtInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  if (i.low >= 0.0){
    result.low = DSqrt(i.low);
    result.high = USqrt(i.high);
  }
  else{
    Asc_Panic(2, NULL,
              "Square root of an interval which contains negative values.\n"
              "Giving up and dying! RIP!\n");
    exit(2);  /*NOTREACHED*/
  }
  TEST(result);
  return result;
}

struct Interval LogInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  if (i.low > 0.0){
    result.low = DLog(i.low);
    result.high = ULog(i.high);
  }
  else{
    Asc_Panic(2, NULL,
              "Log on zero or negative interval.\n"
              "Giving up and dying. RIP!\n");
    exit(2);  /*NOTREACHED*/
  }
  TEST(result);
  return result;
}

struct Interval LnInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  if (i.low > 0.0){
    result.low = DLn(i.low);
    result.high = ULn(i.high);
  }
  else{
    Asc_Panic(2, NULL,
              "Ln on zero or negative interval.\n"
              "Giving up and dying. RIP!\n");
    exit(2);  /*NOTREACHED*/
  }
  TEST(result);
  return result;
}

struct Interval ExpInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  result.low = DExp(i.low);
  result.high = UExp(i.high);
  TEST(result);
  return result;
}

#ifdef HAVE_ERF
struct Interval ErfInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  result.low = DErf(i.low);
  result.high = UErf(i.high);
  TEST(result);
  return result;
}
#endif /* HAVE ERF */

struct Interval AbsInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  result.low = Mig(i);
  result.high = Mag(i);
  TEST(result);
  return result;
}

#define IsOdd(i) ((i & 1)!=0)
#define IsEven(i) ((i & 1)==0)

struct Interval PowerInterval(struct Interval i, long int n)
{
  struct Interval result;
  TEST(i);
  if (n==0) {
    result.low = 1.0;
    result.high = 1.0;
  }
  else {
    if ((n>0)||(i.low > 0.0) || (i.high < 0.0)) {
      if IsOdd(n) {
	if (n > 0) {
	  result.low = DIPow(i.low,n);
	  result.high = UIPow(i.high,n);
	}
	else {
	  result.low = DIPow(i.high,n);
	  result.high = UIPow(i.low,n);
	}
      }
      else{ /* even */
	if (IsIn(0.0,i)) {
	  result.high = MyIntMax(UIPow(i.high,n),UIPow(i.low,n));
	  result.low = 0.0;
	}
	else if (i.low > 0.0) {
	  result.low = DIPow(i.low,n);
	  result.high = UIPow(i.high,n);
	}
	else {
	  result.low = DIPow(i.high,n);
	  result.high = UIPow(i.low,n);
	}
      }
    }
    else {
      Asc_Panic(2, NULL,
                "Interval containing 0 raised to a negative power.\n"
                "Giving up and dying. RIP!\n");/*NOTREACHED*/
      exit(2);  /*NOTREACHED*/
    }
  }
  TEST(result);
  return result;
}

struct Interval NegInterval(struct Interval i)
{
  struct Interval result;
  TEST(i);
  result.low = - i.high;
  result.high = - i.low;
  TEST(result);
  return result;
}

struct Interval Intersect(struct Interval i1, struct Interval i2)
{
  struct Interval result;
  TEST(i1);
  TEST(i2);
  result.low = MAX(i1.low,i2.low);
  result.high = MIN(i1.high,i2.high);
  if (result.low > result.high){
    FPRINTF(ASCERR,"Invalid intersection\n");
    FPRINTF(ASCERR,"result = [%.5g,%.5g]\n",result.low,result.high);
    FPRINTF(ASCERR,"Inf(result)-Sup(result) = %g\n",
	    result.low - result.high);
  }
  TEST(result);
  return result;
}

int IsIn(double d, struct Interval i)
{
  return ((d <= i.high)&&(d >= i.low));
}

struct Interval PowInterval(struct Interval x, struct Interval y)
{
  register double a,b,c,d;
  struct Interval result;
  if (x.low < 0.0){
    Asc_Panic(2, NULL, "PowInterval call with a argument less than zero.\n");
    exit(2);  /*NOTREACHED*/
  }
  else{
    if (x.low == 0.0){
      if (y.low <= 0.0){
        Asc_Panic(2, NULL, "PowInterval called with illegal arguments.\n");
        exit(2);  /*NOTREACHED*/
      }
      else{
        result.low = 0.0;
        a = UPow(x.high,y.low);
        b = UPow(x.high,y.high);
        result.high = MAX(a,b);
      }
    }
    else{
      a = UPow(x.high,y.low);
      b = UPow(x.high,y.high);
      c = UPow(x.low,y.low);
      d = UPow(x.low,y.high);
      result.high = MAX(a,MAX(b,MAX(c,d)));
      a = DPow(x.high,y.low);
      b = DPow(x.high,y.high);
      c = DPow(x.low,y.low);
      d = DPow(x.low,y.high);
      result.low = MIN(a,MIN(b,MIN(c,d)));
    }
  }
  assert((!IsIn(0.0,y))||(IsIn(1.0,result)));
  return result;
}
