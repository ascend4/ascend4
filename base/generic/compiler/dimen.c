/*
 *  Dimension implementation routines
 *  by Tom Epperly
 *  Part of Ascend
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: dimen.c,v $
 *  Date last modified: $Date: 1997/10/28 19:20:32 $
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

#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include <general/mathmacros.h>

struct gl_list_t *g_dimen_list;
dim_type *g_wild_dimen,*g_trig_dimen,*g_dimensionless;
#ifndef lint
static CONST char DimenID[] = "$Id: dimen.c,v 1.9 1997/10/28 19:20:32 mthomas Exp $";
#endif

#define WILD(d) ((d)->wild & DIM_WILD)
/* test a DIMENSION pointer for being wild or not */

char *DimNames[NUM_DIMENS]={
/* keep this structure current with defines in dimen.h */
  "M",
  "Q",
  "L",
  "T",
  "TMP",
  "C",
  "E",
  "LUM",
  "P",
  "S"
};


void InitDimenList(void)
{
  /* FPRINTF(ASCERR,"INITIALISING DIMENSION LIST\n"); */

  g_dimen_list = gl_create(200L);
  AssertMemory(g_dimen_list);
  g_wild_dimen = ASC_NEW(dim_type);
  AssertAllocatedMemory(g_wild_dimen,sizeof(dim_type));
  g_trig_dimen = ASC_NEW(dim_type);
  AssertAllocatedMemory(g_trig_dimen,sizeof(dim_type));
  g_dimensionless = ASC_NEW(dim_type);
  AssertAllocatedMemory(g_dimensionless,sizeof(dim_type));
  assert((g_wild_dimen!=NULL)&&(g_dimensionless!=NULL)&&(g_trig_dimen!=NULL));
  ClearDimensions(g_wild_dimen);
  ClearDimensions(g_trig_dimen);
  ClearDimensions(g_dimensionless);
  g_dimensionless->wild=0;
  g_wild_dimen->wild = DIM_WILD;
  g_trig_dimen->wild = 0;
  SetDimFraction(*g_trig_dimen,D_PLANE_ANGLE,
                 CreateFraction((FRACPART)1,(FRACPART)1));
  gl_insert_sorted(g_dimen_list,g_dimensionless,(CmpFunc)CmpDimen);
  gl_insert_sorted(g_dimen_list,g_wild_dimen,(CmpFunc)CmpDimen);
  gl_insert_sorted(g_dimen_list,g_trig_dimen,(CmpFunc)CmpDimen);
}

void DestroyDimenList(void)
{
  gl_free_and_destroy(g_dimen_list);
  g_wild_dimen = g_dimensionless = NULL;
}

int IsWild(CONST dim_type *d)
{
  if (d != NULL) {
    return (WILD(d));
  } else {
    FPRINTF(ASCERR,"IsWild called on NULL dimension pointer\n");
    return 1;
  }
}

int OddDimension(CONST dim_type *dimp)
{
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1 ||
        (Numerator(GetDimFraction(*dimp,i))%2) )
      return 1;
  return 0;
}

/* return 1 if fractional, null or wild, 0 otherwise */
static int FractionalDimension(CONST dim_type *dimp)
{
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1) return 1;
  return 0;
}

int NonCubicDimension(CONST dim_type *dimp)
{
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1 ||
        (Numerator(GetDimFraction(*dimp,i))%3) )
      return 1;
  return 0;
}

void CopyDimensions(CONST dim_type *src, dim_type *dest)
{
  *dest = *src;
}

CONST dim_type *SquareDimension(CONST dim_type *dim, int check)
{
  if (!dim) return NULL;
  if (IsWild(dim)) return WildDimension();
  if (check && FractionalDimension(dim)) return NULL;
  else {
    dim_type d;
    struct fraction sqr=CreateFraction((FRACPART)2,(FRACPART)1);
    d=ScaleDimensions(dim,sqr);
    return (FindOrAddDimen(&d));
  }
}

CONST dim_type *HalfDimension(CONST dim_type *dim, int check)
{
  if (!dim) return NULL;
  if (IsWild(dim)) return WildDimension();
  if (check && OddDimension(dim)) return NULL;
  else {
    dim_type d;
    struct fraction half=CreateFraction((FRACPART)1,(FRACPART)2);
    d=ScaleDimensions(dim,half);
    return (FindOrAddDimen(&d));
  }
}

CONST dim_type *CubeDimension(CONST dim_type *dim, int check)
{
  if (!dim) return NULL;
  if (IsWild(dim)) return WildDimension();
  if (check && FractionalDimension(dim)) return NULL;
  else {
    dim_type d;
    struct fraction cub=CreateFraction((FRACPART)3,(FRACPART)1);
    d=ScaleDimensions(dim,cub);
    return (FindOrAddDimen(&d));
  }
}

static FRACPART topmax(CONST dim_type *dim) {
  int i;
  FRACPART biggest = 0;
  for (i=0;i<NUM_DIMENS;i++) {
    biggest = MAX(ABS(Numerator(GetDimFraction(*dim,i))),biggest);
  }
  return biggest;
}

CONST dim_type *PowDimension(long mult, CONST dim_type *dim, int check)
{
  if (!dim) return NULL;
  if (IsWild(dim)) return dim;
  if (dim==Dimensionless()) return dim;
  if (check && FractionalDimension(dim)) return NULL;
  if ((long)FRACMAX < mult*topmax(dim)) return NULL;
  else {
    dim_type d;
    struct fraction new;
    new = CreateFraction((FRACPART)mult,(FRACPART)1);
    d = ScaleDimensions(dim,new);
    return (FindOrAddDimen(&d));
  }
}

CONST dim_type *ThirdDimension(CONST dim_type *dim, int check)
{
  if (!dim) return NULL;
  if (IsWild(dim)) return WildDimension();
  if (check && NonCubicDimension(dim)) return NULL;
  else {
    dim_type d;
    struct fraction third=CreateFraction((FRACPART)1,(FRACPART)3);
    d=ScaleDimensions(dim,third);
    return (FindOrAddDimen(&d));
  }
}

void SetWild(dim_type *dim)
{
  assert(dim!=NULL);
  dim->wild=DIM_WILD;
}

int SameDimen(CONST dim_type *d1, CONST dim_type *d2)
{
  assert(d1!=NULL);
  assert(d2!=NULL);

  if (d1==d2 || (WILD(d1) && WILD(d2)) ) return 1;
  /* same pointer or both wild return now */

  if ( WILD(d1) || WILD(d2) ) return 0;
  /* one is wild, other not, so punt */

  return ( memcmp((char *)d1->f,(char *)d2->f,
           (sizeof(struct fraction)*NUM_DIMENS)) ==0 );
}

int CmpDimen(CONST dim_type *d1, CONST dim_type *d2)
{
	register unsigned c;
	register int i;
	assert(d1!=NULL);
	assert(d2!=NULL);
	if (WILD(d1)) {
		if (WILD(d2)) {
			return 0;
		} else {
			return -1;
		}
	}
	if (WILD(d2)) { return 1; }
	for(c=0;c<NUM_DIMENS;c++) {
		i = CmpF( GetDimFraction(*d1,c), GetDimFraction(*d2,c) );
		if (i<0) {
			return -1;
		} else {
			if (i>0) {
				return 1;
			}
			/* else continue to next dimen */
		}
  }
  return 0;
}

void ClearDimensions(dim_type *d)
{
  register unsigned c;
  struct fraction f;
  assert(d!=NULL);
  f = CreateFraction((FRACPART)0,(FRACPART)1);
  d->wild = 0;
  for(c=0;c<NUM_DIMENS;c++)
    SetDimFraction(*d,c,f);
}

CONST dim_type *Dimensionless(void)
{
  AssertAllocatedMemory(g_dimensionless,sizeof(dim_type));
  return g_dimensionless;
}

CONST dim_type *TrigDimension(void)
{
  AssertAllocatedMemory(g_trig_dimen,sizeof(dim_type));
  return g_trig_dimen;
}

CONST dim_type *WildDimension(void)
{
  AssertAllocatedMemory(g_wild_dimen,sizeof(dim_type));
  return g_wild_dimen;
}

static
dim_type *CopyDimen(register CONST dim_type *d)
{
  register dim_type *result;
  assert(d!=NULL);
  result = ASC_NEW(dim_type);
  ascbcopy((char *)d,(char *)result,sizeof(dim_type));
  AssertAllocatedMemory(result,sizeof(dim_type));
  return result;
}

CONST dim_type *FindOrAddDimen(CONST dim_type *d)
{
  register unsigned long place;
  register dim_type *result;
  if ((place=gl_search(g_dimen_list,d,(CmpFunc)CmpDimen))!=0){
    result = gl_fetch(g_dimen_list,place);
  }
  else {
    result = CopyDimen(d);
    gl_insert_sorted(g_dimen_list,result,(CmpFunc)CmpDimen);
  }
  AssertAllocatedMemory(result,sizeof(dim_type));
  return result;
}

dim_type AddDimensions(CONST dim_type *d1, CONST dim_type *d2)
{
  register unsigned c;
  dim_type result;
  ClearDimensions(&result);
  if (WILD(d1)||WILD(d2)) {
    result.wild = DIM_WILD;
  } else {
    for(c=0;c<NUM_DIMENS;c++) {
      SetDimFraction(result,c,
		     AddF(GetDimFraction(*d1,c),GetDimFraction(*d2,c)));
    }
  }
  return result;
}

CONST dim_type *SumDimensions(CONST dim_type *d1, CONST dim_type *d2,int check)
{
  register unsigned c;
  dim_type result;
  if (check && (FractionalDimension(d1) || FractionalDimension(d2)) ) {
    return NULL;
  }
  ClearDimensions(&result);
  if (WILD(d1)||WILD(d2)) {
    return WildDimension();
  } else {
    for(c=0;c<NUM_DIMENS;c++) {
      SetDimFraction(result,c,
		     AddF(GetDimFraction(*d1,c),GetDimFraction(*d2,c)));
    }
  }
  return (FindOrAddDimen(&result));
}

dim_type SubDimensions(CONST dim_type *d1, CONST dim_type *d2)
{
  register unsigned c;
  dim_type result;
  ClearDimensions(&result);
  if (WILD(d1)||WILD(d2)) {
    result.wild = DIM_WILD;
  } else {
    for(c=0;c<NUM_DIMENS;c++) {
      SetDimFraction(result,c,
		     SubF(GetDimFraction(*d1,c),GetDimFraction(*d2,c)));
    }
  }
  return result;
}

CONST dim_type *DiffDimensions(CONST dim_type *d1,
                               CONST dim_type *d2,
                               int check)
{
  register unsigned c;
  dim_type result;
  if (check && (FractionalDimension(d1) || FractionalDimension(d2)) ){
    return NULL;
  }
  ClearDimensions(&result);
  if (WILD(d1)||WILD(d2)) {
    return WildDimension();
  } else {
    for(c=0;c<NUM_DIMENS;c++) {
      SetDimFraction(result,c,
		     SubF(GetDimFraction(*d1,c),GetDimFraction(*d2,c)));
    }
  }
  return (FindOrAddDimen(&result));
}

dim_type ScaleDimensions(CONST dim_type *dim, struct fraction frac)
{
  dim_type result;
  register unsigned c;
  result = *dim;
  if (result.wild & DIM_WILD) return result;
  for(c=0;c<NUM_DIMENS;c++) {
    SetDimFraction(result,c,MultF(frac,GetDimFraction(*dim,c)));
  }
  return result;
}

void PrintDimen(FILE *file, CONST dim_type *dim)
{
  int printed;
  if (WILD(dim)) {
    FPRINTF(file,"wild");
  } else {
    int i;
    printed = 0;
    for (i=0;i<NUM_DIMENS; i++)  {
      if (Numerator(dim->f[i])) {
        FPRINTF(file,"%d/%d%s ",Numerator(dim->f[i]),Denominator(dim->f[i]),
          DimName(i));
        printed = 1;
      }
    }
    if (printed == 0) FPRINTF(file,"dimensionless");
  }
}

ASC_DLLSPEC(void) PrintDimenMessage(CONST char *message
		, CONST char *label1, CONST dim_type *d1
		, CONST char *label2, CONST dim_type *d2
){
		/*
		error_reporter_start(ASC_USER_ERROR,NULL,0,NULL);
		FPRINTF(ASCERR,"%s: %s='", message, label1);
		PrintDimen(ASCERR,d1);
		FPRINTF(ASCERR,"', %s='",label2);
		PrintDimen(ASCERR,d2);
		FPRINTF(ASCERR,"'");
		error_reporter_end_flush();
		*/
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid dimensions");
}


void DumpDimens(FILE *file)
{
  register unsigned long c,len;
  len = gl_length(g_dimen_list);
  FPRINTF(file,"Dimensions dump\n");
  for(c=1;c<=len;c++) {
    PrintDimen(file,(dim_type *)gl_fetch(g_dimen_list,c));
    PUTC('\n',file);
  }
}

CONST dim_type *CheckDimensionsMatch(CONST dim_type *d1, CONST dim_type *d2)
{
  if (WILD(d1)) return d2;
  if (WILD(d2)||d1 == d2) return d1;
  if (CmpDimen(d1,d2)==0) return d1;
  return NULL;
}

void ParseDim(dim_type *dim, CONST char *c)
{
  int i;
  assert((dim!=NULL)&&(c!=NULL));
  ClearDimensions(dim);
  for( i=0; i<NUM_DIMENS && strcmp(c,DimNames[i]); i++ ) ;
  if( i>=NUM_DIMENS ) FPRINTF(ASCERR,"Dimension %s unknown.\n",c);
  else SetDimFraction(*dim,i,CreateFraction((FRACPART)1,(FRACPART)1));
}

char *DimName(CONST int ndx)
{
   if( ndx >= 0 && ndx < NUM_DIMENS )
      return( DimNames[ndx] );
   else
      return NULL;
}
