/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Fraction module for ASCEND.

	Requires:
	#include <utilities/ascConfig.h>
*//*
	by Tom Epperly
	8/18/89
	Version: $Revision: 1.5 $
	Version control file: $RCSfile: fractions.h,v $
	Date last modified: $Date: 1997/07/18 12:29:47 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_FRACTIONS_H
#define ASC_FRACTIONS_H

/** The type of a fraction numerator or denominator. */
#define FRACPART short
/** The maximum FRACPART value. */
#define FRACMAX SHRT_MAX

/** Fraction data structure. */
struct fraction {
  FRACPART numerator;
  FRACPART denominator;
};

ASC_DLLSPEC struct fraction CreateFraction(short n, short d);
/**<
 *  Create a fraction (n/d).
 *  Specify the numerator (n) and denominator (d).
 */

extern struct fraction Simplify(struct fraction f);
/**<
 *  Simplify the fraction.  This forces the denominator to be >= 0; so
 *  if the fraction is negative, the numerator will be negative.
 */

#define Numerator(f) ((f).numerator)
/**<
 *  Return the numerator of f as a FRACPART.
 */

#define Denominator(f) ((f).denominator)
/**<
 *  Return the denominator of f as a FRACPART.
 */

extern struct fraction AddF(struct fraction f1, struct fraction f2);
/**<
 *  Return f1+f2 simplified.
 */

extern struct fraction SubF(struct fraction f1, struct fraction f2);
/**<
 *  Return f1-f2 simplified.
 */

extern struct fraction MultF(struct fraction f1, struct fraction f2);
/**<
 *  Return f1*f2 simplified.
 */

extern struct fraction DivF(struct fraction f1, struct fraction f2);
/**<
 *  Return f1/f2 simplified.
 */

extern int CmpF(struct fraction f1, struct fraction f2);
/**<
 *  Return -1,0,1 if f1 is <,=, or > than f2 respectively.
 */

extern struct fraction NegateF(struct fraction f);
/**<
 *  Returned fraction equal -f.
 */

#endif /* ASC_FRACTIONS_H */

