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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Ascend Dimensions Data Structure.
*//*
	by Tom Epperly
	8/18/89
	Version: $Revision: 1.7 $
	Version control file: $RCSfile: dimen.h,v $
	Date last modified: $Date: 1997/07/18 12:28:58 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_DIMEN_H
#define ASC_DIMEN_H

/**	@addtogroup compiler_dimen Compiler Dimensions
	@{
*/

#include <ascend/general/platform.h>
#include "compiler.h"
#include "fractions.h"


/* Keep these defines current with DimNames in dimen.h. */
#define NUM_DIMENS            10  /**< Number of dimension types. */
#define D_MASS                0   /**< Index for mass dimension. */
#define D_QUANTITY            1   /**< Index for quantity dimension. */
#define D_LENGTH              2   /**< Index for length dimension. */
#define D_TIME                3   /**< Index for time dimension. */
#define D_TEMPERATURE         4   /**< Index for temperature dimension. */
#define D_CURRENCY            5   /**< Index for currency dimension. */
#define D_ELECTRIC_CURRENT    6   /**< Index for electric current dimension. */
#define D_LUMINOUS_INTENSITY  7   /**< Index for luminous intensity dimension. */
#define D_PLANE_ANGLE         8   /**< Index for plane angle dimension. */
#define D_SOLID_ANGLE         9   /**< Index for solid angle dimension. */

/** Dimension data structure. */
struct DimStruct {
  struct fraction f[NUM_DIMENS];  /**< Array of fractions, one for each dimension type. */
  unsigned int wild;              /**< Wild flag.  Only valid values are 0 and DIM_WILD */
};
#define DIM_WILD 0x1
/**< Code for a wild dimension. */

/** Dimension typedef for general use */
typedef struct DimStruct dim_type;

ASC_DLLSPEC struct gl_list_t *g_dimen_list;
/**<
 *  Global list of dimension data structures. All persistent dim pointers
 *  should resolve to something pointed at in this list to minimize dim
 *  overhead.
 */

ASC_DLLSPEC void InitDimenList(void);
/**<
 *  Initialize the dimension list.
 *  Must be called once and only once before any other dimension calls.
 */

ASC_DLLSPEC void DestroyDimenList(void);
/**<
 *  This can be called to deallocate all of the allocated dimensions.
 */

#define GetDimFraction(d,i) ((d).f[i])
/**<
 *  Returns a fraction struct from a dim_type.
 *  i should be one of
 *  - D_MASS
 *  - D_QUANTITY
 *  - D_LENGTH
 *  - D_TIME
 *  - D_TEMPERATURE
 *  - D_CURRENCY
 *  - D_ELECTRIC_CURRENT
 *  - D_LUMINOUS_INTENSITY
 *  - D_PLANE_ANGLE
 *  - D_SOLID_ANGLE
 */

#define GetDimPower(d,i)  \
 (int)(Denominator((d).f[i])==1 ? Numerator((d).f[i]) : 0)
/**<
 *  Returns an int value of the numerator, or 0 if the
 *  denominator != 1.
 */

#define SetDimFraction(d,i,frac) ( (d).f[(i)] = (frac) )
/**<
 *  Set fraction i in dim_type d to frac.
 */

ASC_DLLSPEC void ClearDimensions(dim_type *d);
/**<
 *  Initialize all the dimension fractions to zero.
 */

ASC_DLLSPEC const dim_type*Dimensionless(void);
/**<
 *  Return a pointer to the dimensionless structure.
 */

ASC_DLLSPEC const dim_type *TrigDimension(void);
/**<
 *  Return a pointer to the dimension structure for plane angle.
 */

ASC_DLLSPEC const dim_type *WildDimension(void);
/**<
 *  Return a pointer to a wild dimension structure.  You don't need to
 *  call FindOrAddDimen with this dimension.
 */

extern const dim_type *HalfDimension(const dim_type *d, int b);
/**<
 *  Return a pointer to a dimension structure with sqrt dimensionality.
 *  Returns null if sqrt dimensionality is fractional when tested (b true).
 *  If not b, result may point to noninteger dim. Dim will be in global list.
 */

ASC_DLLSPEC const dim_type *ThirdDimension(const dim_type *d, int b);
/**<
 *  Return a pointer to a dimension structure with cbrt dimensionality.
 *  Returns null if cbrt dimensionality is fractional when tested (b true).
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

ASC_DLLSPEC const dim_type *SquareDimension(const dim_type *d, int b);
/**<
 *  Return a pointer to a dimension structure with square dimensionality.
 *  Returns null if square dimensionality is fractional when tested (b true).
 *  If not b, result may point to noninteger dim. Dim will be in global list.
 */

ASC_DLLSPEC const dim_type *CubeDimension(const dim_type *d, int b);
/**<
 *  Return a pointer to a dimension structure with cube dimensionality.
 *  Returns null if cube dimensionality is fractional when tested (b true).
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

ASC_DLLSPEC const dim_type *PowDimension(long mult, const dim_type *d, int b);
/**<
 *  Return a pointer to a dimension structure with d*mult dimensionality.
 *  Returns null if cube dimensionality is fractional when tested (b true)
 *  or if mult*d yields integer overflow of the dimensionality.
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

ASC_DLLSPEC void SetWild(dim_type *dim);
/**<
 *  Set the wild flag of dimensions dim.
 */

ASC_DLLSPEC int IsWild(const dim_type *d);
/**<
 *  Return a true value if d is wild, and otherwise return a false value.
 */

ASC_DLLSPEC int OddDimension(const dim_type *d);
/**<
 *  Return a true value if d has an odd, wild, or non-integer dimension.
 */

ASC_DLLSPEC int NonCubicDimension(const dim_type *d);
/**<
 *  Return a true value if d has an noncubic, wild, or non-integer dimension.
 */

extern int SameDimen(const dim_type *d1, const dim_type *d2);
/**<
 *  Return 1 if d1 and d2 have the same dimensional value, or 0
 *  otherwise. Two wild dimensions are the same, regardless of any
 *  other data they may contain.
 *  Wild and any non-wild are NOT the same.
 */

ASC_DLLSPEC int CmpDimen(const dim_type *d1, const dim_type *d2);
/**<
 *  Return 1,0,-1 if d1 is >,=, or < d2 respectively.
 */

ASC_DLLSPEC const dim_type *FindOrAddDimen(const dim_type *d);
/**<
 *  This function is run to make sure only one copy of each dimensions
 *  is stored.  It is designed to be called as follows:
 *  <pre>
 *  Example:
 *    dim_type d,*p;
 *    ClearDimensions(&d);
 *    SetDimFraction(d,D_MASS,CreateFraction(1,2));
 *    ...etc...
 *    p = FindOrAddDimen(&d);
 *    p will never point to d.  p != &d.
 *  </pre>
 */

ASC_DLLSPEC void CopyDimensions(const dim_type *src, dim_type *dest);
/**<
 *  Copy from src to dest.
 */

ASC_DLLSPEC dim_type AddDimensions(const dim_type *d1, const dim_type *d2);
/**<
 *  Add 2 dimensions.
 *  Wild+anything equals wild.
 *  return d1+d2;
 *  NOTE: This returns a dim by value, not by pointer. There are places
 *  where this is desirable. Where you want a pointer from the dim
 *  table instead, use SumDimensions.
 */

ASC_DLLSPEC const dim_type *SumDimensions(const dim_type *d1, const dim_type *d2, int check);
/**<
 *  Add 2 dimensions with checking.
 *  Wild+anything equals wild.
 *  return d1+d2;
 *  If check != 0, verifies that d1 and d2 are not fractional, returning
 *  NULL if fractional found.
 *  Result will be in global list.
 */

ASC_DLLSPEC dim_type SubDimensions(const dim_type *d1, const dim_type *d2);
/**<
 *  Subtract 2 dimensions.
 *  Wild-anything equals wild.
 *  return d1-d2;
 *  NOTE: This returns a dim by value, not by pointer. There are places
 *  where this is desirable. Where you want a pointer from the dim
 *  table instead, use DiffDimensions.
 */

extern const dim_type *DiffDimensions(const dim_type *d1, const dim_type *d2, int check);
/**<
 *  Subtract 2 dimensions with checking.
 *  Wild-anything equals wild.
 *  return d1-d2;
 *  If check != 0, verifies that d1 and d2 are not fractional, returning
 *  NULL if fractional found.
 *  Result will be in global list.
 */

ASC_DLLSPEC dim_type ScaleDimensions(const dim_type *dim, struct fraction frac);
/**<
 *  Scale the dimensions by frac.  A wild scaled always remains wild.
 */

ASC_DLLSPEC void ParseDim(dim_type *dim, const char *c);
/**<
 *  Initialize dim appropriately according to the string c.  If c doesn't
 *  match any of the dimension strings, dim will be dimensionless and
 *  an error message will be printed.
 *
 *  Note that this function only matches simple single-dimensional strings,
 *  equal to just one of the strings below, no dimensional expressions are
 *  matched.
 *  
 *  <pre>
 *         String  Dimension Index
 *         "M"     D_MASS
 *         "Q"     D_QUANTITY
 *         "T"     D_TIME
 *         "L"     D_LENGTH
 *         "TMP"   D_TEMPERATURE
 *         "C"     D_CURRENCY
 *         "E"     D_ELECTRIC_CURRENT
 *         "LUM"   D_LUMINOUS_INTENSITY
 *         "P"     D_PLANE_ANGLE
 *         "S"     D_SOLID_ANGLE
 *  </pre>
 */

ASC_DLLSPEC char *DimName(const int nIndex);
/**<
 *  Return the internal copy of the name of the dimension corresponding
 *  to index if index is within [0..NUM_DIMENS-1], otherwise return NULL.
 */

ASC_DLLSPEC const dim_type *CheckDimensionsMatch(const dim_type *d1, const dim_type *d2);
/**<
 *  Compare 2 dimensions.
 *  - Return d1 if d2 is wild
 *  - Return d2 if d1 is wild or d1 == d2
 *  - Return d1 if *d1 == *d2
 *  - Otherwise return NULL
 */

ASC_DLLSPEC void PrintDimen(FILE *f ,const dim_type *d);
/**< Print a dimension to a file.  Used in interface */


ASC_DLLSPEC void PrintDimenMessage(const char *message
		, const char *label1, const dim_type *d1
		, const char *label2, const dim_type *d2
);

/**< Print a message like "LABEL1='dim1', LABEL2='dim2'" */

ASC_DLLSPEC void DumpDimens(FILE *f);
/**< Dump all dimensions to a file.  Used in interface */

/* @} */

#endif /* ASC_DIMEN_H */
