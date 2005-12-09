/*
 *  Fraction implementation for ASCEND
 *  by Tom Epperly
 *  8/18/89
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: fractions.c,v $
 *  Date last modified: $Date: 1997/07/18 12:29:44 $
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"

#ifndef lint
static CONST char FractionsID[] = "$Id: fractions.c,v 1.5 1997/07/18 12:29:44 mthomas Exp $";
#endif


static
short GCD(register int s1, register int s2)
/*
 *  Return the greatist common divisor of s1 and s2.  This is guarenteed
 *  to always work.  It can be proven to be correct.
 */
{
  if (s1<0) {
    s1 = -s1;
  }
  if (s2<0) {
    s2 = -s2;
  }
  if ((s1==0)||(s2==0)) {
    return (short)(s1+s2);
  }
  while (s1!=s2) {
    if (s1>s2) {
      s1 -= s2;
    } else {
      s2 -= s1;
    }
  }
  return (short)s1;
}

struct fraction Simplify(struct fraction f)
{
  register short s;
  if (f.denominator<0) {
    f.denominator = -f.denominator;
    f.numerator = -f.numerator;
  }
  s = GCD(f.numerator,f.denominator);
  if ((s!=1)&&(s!=0)) {
    f.numerator /= s;
    f.denominator /= s;
  }
  return f;
}

struct fraction CreateFraction(short n,short d)
/*     short n,d;*/
{
  struct fraction result;
  result.numerator = n;
  result.denominator = d;
  return Simplify(result);
}

struct fraction AddF(struct fraction f1, struct fraction f2)
{
  struct fraction result;
  if (f1.denominator==f2.denominator) {
    result.numerator = f1.numerator+f2.numerator;
    result.denominator = f1.denominator;
    return result;
  }
  else {
    result.numerator = f1.numerator*f2.denominator+f2.numerator*f1.denominator;
    result.denominator = f1.denominator*f2.denominator;
    return Simplify(result);
  }
}

struct fraction SubF(struct fraction f1, struct fraction f2)
{
  struct fraction result;
  if (f1.denominator==f2.denominator) {
    result.denominator = f1.denominator;
    result.numerator = f1.numerator-f2.numerator;
    return result;
  }
  else {
    result.denominator = f1.denominator*f2.denominator;
    result.numerator = f1.numerator*f2.denominator-f2.numerator*f1.denominator;
    return Simplify(result);
  }
}

struct fraction MultF(struct fraction f1, struct fraction f2)
{
  struct fraction result;
  result.numerator = f1.numerator*f2.numerator;
  result.denominator = f1.denominator*f2.denominator;
  return Simplify(result);
}

struct fraction DivF(struct fraction f1, struct fraction f2)
{
  struct fraction result;
  result.numerator = f1.numerator*f2.denominator;
  result.denominator = f1.denominator*f2.numerator;
  return Simplify(result);
}

int CmpF(struct fraction f1, struct fraction f2)
{
  register FRACPART
    num1,
    num2;
  num1 = f1.numerator*f2.denominator;
  num2 = f2.numerator*f1.denominator;
  if (num1 < num2) return -1;
  else if (num1 == num2) return 0;
  else return 1;
}

struct fraction NegateF(struct fraction f)
{
  struct fraction result;
  result.numerator = -f.numerator;
  result.denominator = f.denominator;
  return result;
}
