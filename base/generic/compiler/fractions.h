/**< 
 *  Fraction module for ASCEND
 *  by Tom Epperly
 *  8/18/89
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: fractions.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:47 $
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
 */

#ifndef __FRACTIONS_H_SEEN__
#define __FRACTIONS_H_SEEN__


/**< 
 *  When #including fractions.h, make sure these files are #included first:
 *         NO INCLUDES NEEDED
 */


#define FRACPART short
#define FRACMAX SHRT_MAX
struct fraction {
  FRACPART numerator,denominator;
};

extern struct fraction CreateFraction(short,short);
/**< 
 *  struct fraction CreateFraction(n,d)
 *  short n,d;
 *  n - numerator
 *  d - denominator
 */

extern struct fraction Simplify(struct fraction);
/**< 
 *  struct fraction Simplify(f)
 *  struct fraction f;
 *  Simplify the fraction.  This forces the denominator to be >= 0; so
 *  if the fraction is negative, the numerator will be negative.
 */

#define Numerator(f) ((f).numerator)
/**< 
 *  macro Numerator(f)
 *  struct fraction f;
 *  return the numerator of f
 */

#define Denominator(f) ((f).denominator)
/**< 
 *  macro Denominator(f)
 *  struct fraction f;
 *  return the denominator of f.
 */

extern struct fraction AddF(struct fraction,struct fraction);
/**< 
 *  struct fraction AddF(f1,f2)
 *  struct fraction f1,f2;
 *  Return f1+f2 simplified.
 */

extern struct fraction SubF(struct fraction,struct fraction);
/**< 
 *  struct fraction SubF(f1,f2)
 *  struct fraction f1,f2;
 *  Return f1-f2 simplified.
 */

extern struct fraction MultF(struct fraction,struct fraction);
/**< 
 *  struct fraction MultF(f1,f2)
 *  struct fraction f1,f2;
 *  Return f1*f2 simplified.
 */

extern struct fraction DivF(struct fraction,struct fraction);
/**< 
 *  struct fraction DivF(f1,f2)
 *  struct fraction f1,f2;
 *  Return f1/f2 simplified.
 */

extern int CmpF(struct fraction,struct fraction);
/**< 
 *  int CmpF(f1,f2)
 *  struct fraction f1,f2;
 *  Return -1,0,1 if f1 is <,=, or > than f2 respectively.
 */

extern struct fraction NegateF(struct fraction);
/**< 
 *  struct fraction NegateF(f)
 *  struct fraction f;
 *  Returned fraction equal -f.
 */
#endif /**< __FRACTIONS_H_SEEN__ */
