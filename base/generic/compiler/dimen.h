/*
 *  Ascend Dimensions Data Structure
 *  by Tom Epperly
 *  8/18/89
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: dimen.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:58 $
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

#ifndef __DIMEN_H_SEEN__
#define __DIMEN_H_SEEN__


/*
 *  When #including dimen.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 */

#define NUM_DIMENS            10
#define D_MASS                0  /* index for mass dimension */
#define D_QUANTITY            1  /* index for quantity dimension */
#define D_LENGTH              2  /* index for length dimension */
#define D_TIME                3  /* index for time dimension */
#define D_TEMPERATURE         4  /* index for temperature dimension */
#define D_CURRENCY            5  /* index for currency dimension */
#define D_ELECTRIC_CURRENT    6  /* index for electric current dimension */
#define D_LUMINOUS_INTENSITY  7  /* index for luminous intensity dimension */
#define D_PLANE_ANGLE         8  /* index for plane angle dimension */
#define D_SOLID_ANGLE         9  /* index for solid angle dimension */

struct DimStruct {
  struct fraction f[NUM_DIMENS];
  unsigned int wild;	/* only valid values 0 and DIM_WILD */
#define DIM_WILD 0x1
};

typedef struct DimStruct dim_type;

extern struct gl_list_t * g_dimen_list;
/*
 *  Global list of dimension data structures. All persistent dim pointers
 *  should resolve to something pointed at in this list to minimize dim
 *  overhead.
 */

extern void InitDimenList(void);
/*
 *  Must be called once and only once before any other dimension calls.
 */

extern void DestroyDimenList(void);
/*
 *  This can be called to deallocate all of the allocated dimensions.
 */

#define GetDimFraction(d,i) ((d).f[i])
/*
 *  Returns a fraction struct from a dim_type.
 *  i should be one of D_MASS D_QUANTITY D_LENGTH D_TIME D_TEMPERATURE
 *  D_CURRENCY D_ELECTRIC_CURRENT D_LUMINOUS_INTENSITY D_PLANE_ANGLE
 *  D_SOLID_ANGLE
 */

#define GetDimPower(d,i)  \
 (int)(Denominator((d).f[i])==1 ? Numerator((d).f[i]) : 0)
/*
 *  Returns an int value of the numerator, or 0 if the
 *  denominator != 1.
 */

#define SetDimFraction(d,i,frac) ( (d).f[(i)] = (frac) )

extern void ClearDimensions(dim_type *);
/*
 *  ClearDimensions(d)
 *  dim_type *d;
 *  Initialize all the dimension fractions to zero.
 */

extern CONST dim_type *Dimensionless(void);
/*
 *  dim_type *Dimensionless();
 *  Return a pointer to the dimensionless structure.
 */

extern CONST dim_type *TrigDimension(void);
/*
 *  dim_type *TrigDimension();
 *  Return a pointer to the dimension structure for plane angle.
 */

extern CONST dim_type *WildDimension(void);
/*
 *  dim_type *WildDimension()
 *  Return a pointer to a wild dimension structure.  You don't need to
 *  call FindOrAddDimen with this dimension.
 */

extern  CONST dim_type *HalfDimension(CONST dim_type *,int);
/*
 *  dim_type *HalfDimension(d,b)
 *  Return a pointer to a dimension structure with sqrt dimensionality.
 *  Returns null if sqrt dimensionality is fractional when tested (b true).
 *  If not b, result may point to noninteger dim. Dim will be in global list.
 */

extern  CONST dim_type *ThirdDimension(CONST dim_type *,int);
/*
 *  dim_type *ThirdDimension(d,b)
 *  Return a pointer to a dimension structure with cbrt dimensionality.
 *  Returns null if cbrt dimensionality is fractional when tested (b true).
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

extern  CONST dim_type *SquareDimension(CONST dim_type *,int);
/*
 *  dim_type *SquareDimension(d,b)
 *  Return a pointer to a dimension structure with square dimensionality.
 *  Returns null if square dimensionality is fractional when tested (b true).
 *  If not b, result may point to noninteger dim. Dim will be in global list.
 */

extern  CONST dim_type *CubeDimension(CONST dim_type *,int);
/*
 *  dim_type *CubeDimension(d,b)
 *  Return a pointer to a dimension structure with cube dimensionality.
 *  Returns null if cube dimensionality is fractional when tested (b true).
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

extern  CONST dim_type *PowDimension(long,CONST dim_type *,int);
/*
 *  dim_type *IntDimension(mult,d,b)
 *  Return a pointer to a dimension structure with d*mult dimensionality.
 *  Returns null if cube dimensionality is fractional when tested (b true)
 *  or if mult*d yields integer overflow of the dimensionality.
 *  If !b, result may point to noninteger dim. Dim will be in global list.
 */

extern void SetWild(dim_type *);
/*
 *  void SetWild(dim)
 *  dim_type *dim;
 *  Set the wild flag of dimensions dim.
 */

extern int IsWild(CONST dim_type *);
/*
 *  int IsWild(d)
 *  const dim_type *d;
 *  Return a true value if d is wild, and otherwise return a false value.
 */

extern int OddDimension(CONST dim_type *);
/*
 *  int OddDimension(d)
 *  const dim_type *d;
 *  Return a true value if d has an odd, wild, or non-integer dimension.
 */

extern int NonCubicDimension(CONST dim_type *);
/*
 *  int CubicDimension(d)
 *  const dim_type *d;
 *  Return a true value if d has an noncubic, wild, or non-integer dimension.
 */

extern int SameDimen(CONST dim_type *,CONST dim_type *);
/*
 *  SameDimen(d1,d2)
 *  const dim_type *d1,*d2;
 *  Return 1 if d1 and d2 have the same dimensional value, or 0
 *  otherwise. Two wild dimensions are the same, regardless of any
 *  other data they may contain.
 *  Wild and any non-wild are NOT the same.
 */

extern int CmpDimen(CONST dim_type *,CONST dim_type *);
/*
 *  CmpDimen(d1,d2)
 *  const dim_type *d1,*d2;
 *  Return 1,0,-1 if d1 is >,=, or < d2 respectively.
 */

extern CONST dim_type *FindOrAddDimen(CONST dim_type *);
/*
 *  const dim_type *FindOrAddDimen(d)
 *  const dim_type *d;
 *  This function is run to make sure only one copy of each dimensions
 *  is stored.  It is designed to be called as follows:
 *  Example:
 *  dim_type d,*p;
 *  ClearDimensions(&d);
 *  SetDimFraction(d,D_MASS,CreateFraction(1,2));
 *  ...etc...
 *  p = FindOrAddDimen(&d);
 *  p will never point to d.  p != &d.
 */

extern void CopyDimensions(CONST dim_type *,dim_type *);
/*
 *  void CopyDimensions(src,dest)
 *  const dim_type *src;
 *  dim_type *dest;
 *  Copy from src to dest.
 */

extern dim_type AddDimensions(CONST dim_type *,CONST dim_type *);
/*
 *  dim_type AddDimensions(d1,d2)
 *  const dim_type *d1,*d2;
 *  Wild+anything equals wild.
 *  return d1+d2;
 *  NOTE: This returns a dim by value, not by pointer. There are places
 *  where this is desirable. Where you want a pointer from the dim
 *  table instead, use SumDimensions.
 */

extern CONST dim_type *SumDimensions(CONST dim_type *,CONST dim_type *,int);
/*
 *  CONST dim_type *SumDimensions(d1,d2,check)
 *  const dim_type *d1,*d2;
 *  int check;
 *  Wild+anything equals wild.
 *  return d1+d2;
 *  If check != 0, verifies that d1 and d2 are not fractional, returning
 *  NULL if fractional found.
 *  Result will be in global list.
 */

extern dim_type SubDimensions(CONST dim_type *,CONST dim_type *);
/*
 *  dim_type SubDimensions(d1,d2)
 *  const dim_type *d1,*d2;
 *  Wild-anything equals wild.
 *  return d1-d2;
 *  NOTE: This returns a dim by value, not by pointer. There are places
 *  where this is desirable. Where you want a pointer from the dim
 *  table instead, use DiffDimensions.
 */

extern CONST dim_type *DiffDimensions(CONST dim_type *,CONST dim_type *,int);
/*
 *  CONST dim_type *SumDimensions(d1,d2,check)
 *  const dim_type *d1,*d2;
 *  int check;
 *  Wild-anything equals wild.
 *  return d1-d2;
 *  If check != 0, verifies that d1 and d2 are not fractional, returning
 *  NULL if fractional found.
 *  Result will be in global list.
 */

extern dim_type ScaleDimensions(CONST dim_type *,struct fraction);
/*
 *  dim_type ScaleDimensions(dim,frac)
 *  dim_type *dim;
 *  struct fraction frac;
 *  Scale the dimensions by frac.  A wild scaled always remains wild.
 */

extern void ParseDim(dim_type *,CONST char *);
/*
 *  void ParseDim(dim,c)
 *  dim_type *dim;
 *  char *c;
 *  Initialize dim appropriately according to the string c.  If c doesn't
 *  match any of the dimension strings, dim will be dimensionless and
 *  an error message will be printed.
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
 */

extern char *DimName(CONST int);
/*
 *  char *DimName(index)
 *  int index;
 *  Return the internal copy of the name of the dimension corresponding
 *  to index if index is within [0..NUM_DIMENS-1], otherwise return NULL.
 */

extern CONST dim_type *CheckDimensionsMatch(CONST dim_type *,CONST dim_type *);
/*
 *  dim_t *CheckDimensionsMatch(d1,d2)
 *  const dim_t *d1,*d2;
 *  Return d1 if d2 is wild
 *  Return d2 if d1 is wild or d1 == d2
 *  Return d1 if *d1 == *d2
 *  Otherwise return NULL
 */

extern void PrintDimen(FILE *,CONST dim_type *);
extern void DumpDimens(FILE *);
/* used in interface */

#endif /* __DIMEN_H_SEEN__ */
