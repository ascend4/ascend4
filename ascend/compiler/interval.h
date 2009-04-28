/*
 *  Interval Arithmetic Routines
 *  by Tom Epperly
 *  Created: 5/29/1990
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: interval.h,v $
 *  Date last modified: $Date: 1997/07/18 12:31:02 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  Interval Arithmetic Routines.
 *  <pre>
 *  When #including interval.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASC_INTERVAL_H
#define ASC_INTERVAL_H

/**	@addtogroup compiler Compiler
	@{
*/

/** Interval data structure. */
struct Interval {
  double low;
  double high;
};

extern struct Interval CreateInterval(double low, double high);
/**< 
 *  <!--  struct Interval CreateInterval(low,high)                     -->
 *  <!--  double low,high;                                             -->
 *  Create an interval from low to high.
 */

extern struct Interval IntervalFromIntegers(long low, long high);
/**< 
 *  <!--  struct Interval IntervalFromIntegers(low,high)               -->
 *  <!--  long low,high;                                               -->
 *  Create an interval from low to high where the endpoints are integers.
 */

extern struct Interval CreateThin(double v);
/**< 
 *  <!--  struct Interval CreateThin(v)                                -->
 *  <!--  double v;                                                    -->
 *  Create an interval with lower and upper bound equal to v.
 */

extern struct Interval CreateThinInteger(long l);
/**< 
 *  <!--  struct Interval CreateThingInteger(l)                        -->
 *  <!--  long l;                                                      -->
 *  Create an interval with lower and upper bound equal to the integer l.
 */

extern double Mag(struct Interval i);
/**< 
 *  Returns the magnitude of interval i.
 */

extern double Mig(struct Interval i);
/**< 
 *  Returns the mignitude of interval i.
 */

extern double Rad(struct Interval i);
/**< 
 *  Returns the radius of interval i.
 */

extern double Mid(struct Interval i);
/**< 
 *  returns the midpoint of interval i.
 */

#define Inf(i) ((i).low)
/**< 
 *  Returns the lower bound of the interval.
 */

#define Sup(i) ((i).high)
/**< 
 *  Returns the upper bound of the interval.
 */

extern struct Interval AddIntervals(struct Interval i1, struct Interval i2);
/**< 
 *  Return the sum of the two arguments.
 */

extern struct Interval SubIntervals(struct Interval i1, struct Interval i2);
/**< 
 *  Return the first interval minus the second interval.
 */

extern struct Interval SubtractScaler(struct Interval i, double scalar);
/**< 
 *  Return the interval minus the scaler.
 */

extern struct Interval MulIntervals(struct Interval i1, struct Interval i2);
/**< 
 *  Return the product of the two intervals.
 */

extern struct Interval ScaleInterval(struct Interval i, double scalar);
/**< 
 *  Return the interval times the scalar.
 */

extern struct Interval DivIntervals(struct Interval i1, struct Interval i2);
/**< 
 *  Return the first interval divided by the second.
 */

extern struct Interval SqrInterval(struct Interval i);
/**< 
 *  Return the interval squared.
 */

extern struct Interval SqrtInterval(struct Interval i);
/**< 
 *  Return the square root of the interval.
 */

extern struct Interval LnInterval(struct Interval i);
/**< 
 *  Return the natural log of the interval.
 */

extern struct Interval LogInterval(struct Interval i);
/**< 
 *  Return the log base 10 of the interval.
 */

extern struct Interval ExpInterval(struct Interval i);
/**< 
 *  Return e raised to the power of the interval.
 */

extern struct Interval ErfInterval(struct Interval i);
/**< 
 *  Return the erf of the interval.
 */

extern struct Interval AbsInterval(struct Interval i);
/**< 
 *  Return the absolute value of the interval.
 */

extern struct Interval PowerInterval(struct Interval i, long power);
/**< 
 *  Return the interval raised to the integer power.
 */

extern struct Interval PowInterval(struct Interval i1, struct Interval i2);
/**< 
 *  <!--  struct Interval PowInterval(x,y)                             -->
 *  <!--  struct Interval x,y;                                         -->
 *  Return x^y  or x**y.
 */

extern struct Interval NegInterval(struct Interval i);
/**< 
 *  Return the negative of the interval.
 */

extern struct Interval Intersect(struct Interval i1, struct Interval i2);
/**< 
 *  Return the intersection of the two intervals.
 */

/* BOOLEAN OPERATIONS */

extern int IsIn(double scaler, struct Interval i);
/**< 
 *  Return true if the scaler is in the interval.
 */

/* @} */

#endif /* ASC_INTERVAL_H */

