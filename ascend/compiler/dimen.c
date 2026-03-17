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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dimen.h"

#include <ascend/general/platform.h>

#include <ascend/general/panic.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/dstring.h>
#include <ascend/utilities/error.h>
#include <ascend/general/list.h>

#include <ascend/general/mathmacros.h>

struct gl_list_t *g_dimen_list;
dim_type *g_wild_dimen,*g_trig_dimen,*g_dimensionless;

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


void InitDimenList(void){
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


void DestroyDimenList(void){
  gl_free_and_destroy(g_dimen_list);
  g_wild_dimen = g_dimensionless = NULL;
}


int IsWild(const dim_type *d){
  if(d != NULL){
    return(WILD(d));
  }else{
    FPRINTF(ASCERR,"IsWild called on NULL dimension pointer\n");
    return 1;
  }
}


int OddDimension(const dim_type *dimp){
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1 ||
        (Numerator(GetDimFraction(*dimp,i))%2) )
      return 1;
  return 0;
}


/* return 1 if fractional, null or wild, 0 otherwise */
static int FractionalDimension(const dim_type *dimp){
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1) return 1;
  return 0;
}


int NonCubicDimension(const dim_type *dimp){
  int i;
  if (!dimp || IsWild(dimp)) return 1;
  for (i=0;i<NUM_DIMENS;i++)
    if (Denominator(GetDimFraction(*dimp,i))!=1 ||
        (Numerator(GetDimFraction(*dimp,i))%3) )
      return 1;
  return 0;
}


void CopyDimensions(const dim_type *src, dim_type *dest){
  *dest = *src;
}


const dim_type *SquareDimension(const dim_type *dim, int check){
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


const dim_type *HalfDimension(const dim_type *dim, int check){
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


const dim_type *CubeDimension(const dim_type *dim, int check){
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


static FRACPART topmax(const dim_type *dim){
  int i;
  FRACPART biggest = 0;
  for (i=0;i<NUM_DIMENS;i++) {
    biggest = MAX(ABS(Numerator(GetDimFraction(*dim,i))),biggest);
  }
  return biggest;
}


const dim_type *PowDimension(long mult, const dim_type *dim, int check){
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

const dim_type *ThirdDimension(const dim_type *dim, int check)
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


void SetWild(dim_type *dim){
  assert(dim!=NULL);
  dim->wild=DIM_WILD;
}


int SameDimen(const dim_type *d1, const dim_type *d2){
  assert(d1!=NULL);
  assert(d2!=NULL);

  if (d1==d2 || (WILD(d1) && WILD(d2)) ) return 1;
  /* same pointer or both wild return now */

  if ( WILD(d1) || WILD(d2) ) return 0;
  /* one is wild, other not, so punt */

  return ( memcmp((char *)d1->f,(char *)d2->f,
           (sizeof(struct fraction)*NUM_DIMENS)) ==0 );
}


int CmpDimen(const dim_type *d1, const dim_type *d2){
	unsigned c;
	int i;
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


void ClearDimensions(dim_type *d){
  unsigned c;
  struct fraction f;
  assert(d!=NULL);
  f = CreateFraction((FRACPART)0,(FRACPART)1);
  d->wild = 0;
  for(c=0;c<NUM_DIMENS;c++)
    SetDimFraction(*d,c,f);
}


const dim_type *Dimensionless(void){
  AssertAllocatedMemory(g_dimensionless,sizeof(dim_type));
  return g_dimensionless;
}


const dim_type *TrigDimension(void){
  AssertAllocatedMemory(g_trig_dimen,sizeof(dim_type));
  return g_trig_dimen;
}


const dim_type *WildDimension(void){
  AssertAllocatedMemory(g_wild_dimen,sizeof(dim_type));
  return g_wild_dimen;
}


static
dim_type *CopyDimen(const dim_type *d){
  dim_type *result;
  assert(d!=NULL);
  result = ASC_NEW(dim_type);
  ascbcopy((char *)d,(char *)result,sizeof(dim_type));
  AssertAllocatedMemory(result,sizeof(dim_type));
  return result;
}


const dim_type *FindOrAddDimen(const dim_type *d){
  unsigned long place;
  dim_type *result;
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


dim_type AddDimensions(const dim_type *d1, const dim_type *d2){
  unsigned c;
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


const dim_type *SumDimensions(const dim_type *d1, const dim_type *d2,int check){
  unsigned c;
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


dim_type SubDimensions(const dim_type *d1, const dim_type *d2){
  unsigned c;
  dim_type result;
  ClearDimensions(&result);
  if (WILD(d1)||WILD(d2)) {
    result.wild = DIM_WILD;
  }else{
    for(c=0;c<NUM_DIMENS;c++) {
      SetDimFraction(result,c,
		     SubF(GetDimFraction(*d1,c),GetDimFraction(*d2,c)));
    }
  }
  return result;
}

const dim_type *DiffDimensions(const dim_type *d1,
                               const dim_type *d2,
                               int check)
{
  unsigned c;
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


dim_type ScaleDimensions(const dim_type *dim, struct fraction frac){
  dim_type result;
  unsigned c;
  result = *dim;
  if (result.wild & DIM_WILD) return result;
  for(c=0;c<NUM_DIMENS;c++) {
    SetDimFraction(result,c,MultF(frac,GetDimFraction(*dim,c)));
  }
  return result;
}


void PrintDimen(FILE *file, const dim_type *dim){
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

static int DimenHasTerms(const dim_type *dim){
  int i;
  for(i=0; i<NUM_DIMENS; ++i){
    if (Numerator(GetDimFraction(*dim,i))) {
      return 1;
    }
  }
  return 0;
}

static void PrintDimenFactor(FILE *file, const char *name, struct fraction power){
  FRACPART num = Numerator(power);
  FRACPART den = Denominator(power);
  if (num == 1 && den == 1) {
    FPRINTF(file,"%s",name);
  } else if (den == 1) {
    FPRINTF(file,"%s^%d",name,(int)num);
  } else {
    FPRINTF(file,"%s^(%d/%d)",name,(int)num,(int)den);
  }
}

static void PrintDimenProduct(FILE *file, const dim_type *dim, int negative){
  int i, printed;
  printed = 0;
  for(i=0; i<NUM_DIMENS; ++i){
    struct fraction power = GetDimFraction(*dim,i);
    FRACPART num = Numerator(power);
    if (num == 0) {
      continue;
    }
    if ((negative && num > 0) || (!negative && num < 0)) {
      continue;
    }
    if (printed) {
      FPRINTF(file,"*");
    }
    if (num < 0) {
      num = -num;
    }
    PrintDimenFactor(file,DimName(i),CreateFraction(num,Denominator(power)));
    printed = 1;
  }
  if (!printed) {
    FPRINTF(file,"1");
  }
}

static void PrintDimenReadable(FILE *file, const dim_type *dim){
  int i, has_negative;
  if (WILD(dim)) {
    FPRINTF(file,"[wild]");
    return;
  }
  if (!DimenHasTerms(dim)) {
    FPRINTF(file,"[dimensionless]");
    return;
  }
  has_negative = 0;
  for(i=0; i<NUM_DIMENS; ++i){
    if (Numerator(GetDimFraction(*dim,i)) < 0) {
      has_negative = 1;
      break;
    }
  }
  FPRINTF(file,"[");
  PrintDimenProduct(file,dim,0);
  if (has_negative) {
    FPRINTF(file,"/");
    PrintDimenProduct(file,dim,1);
  }
  FPRINTF(file,"]");
}

static void WriteDimenFactor2Str(Asc_DString *dsPtr, const char *name, struct fraction power){
  char buf[64];
  FRACPART num = Numerator(power);
  FRACPART den = Denominator(power);
  if (num == 1 && den == 1) {
    Asc_DStringAppend(dsPtr,name,-1);
  } else if (den == 1) {
    snprintf(buf,sizeof(buf),"%s^%d",name,(int)num);
    Asc_DStringAppend(dsPtr,buf,-1);
  } else {
    snprintf(buf,sizeof(buf),"%s^(%d/%d)",name,(int)num,(int)den);
    Asc_DStringAppend(dsPtr,buf,-1);
  }
}

static void WriteDimenProduct2Str(Asc_DString *dsPtr, const dim_type *dim, int negative){
  int i, printed;
  printed = 0;
  for(i=0; i<NUM_DIMENS; ++i){
    struct fraction power = GetDimFraction(*dim,i);
    FRACPART num = Numerator(power);
    if (num == 0) {
      continue;
    }
    if ((negative && num > 0) || (!negative && num < 0)) {
      continue;
    }
    if (printed) {
      Asc_DStringAppend(dsPtr,"*",1);
    }
    if (num < 0) {
      num = -num;
    }
    WriteDimenFactor2Str(dsPtr,DimName(i),CreateFraction(num,Denominator(power)));
    printed = 1;
  }
  if (!printed) {
    Asc_DStringAppend(dsPtr,"1",1);
  }
}

ASC_DLLSPEC char *WriteDimensionBracketsString(CONST dim_type *dim){
  Asc_DString ds;
  char *result;
  int i, has_negative;

  Asc_DStringInit(&ds);
  if (WILD(dim)) {
    Asc_DStringAppend(&ds,"[wild]",-1);
  } else if (!DimenHasTerms(dim)) {
    Asc_DStringAppend(&ds,"[dimensionless]",-1);
  } else {
    has_negative = 0;
    for(i=0; i<NUM_DIMENS; ++i){
      if (Numerator(GetDimFraction(*dim,i)) < 0) {
        has_negative = 1;
        break;
      }
    }
    Asc_DStringAppend(&ds,"[",1);
    WriteDimenProduct2Str(&ds,dim,0);
    if (has_negative) {
      Asc_DStringAppend(&ds,"/",1);
      WriteDimenProduct2Str(&ds,dim,1);
    }
    Asc_DStringAppend(&ds,"]",1);
  }
  result = Asc_DStringResult(&ds);
  Asc_DStringFree(&ds);
  return result;
}


ASC_DLLSPEC void PrintDimenMessage(const char *message
		, const char *label1, const dim_type *d1
		, const char *label2, const dim_type *d2
){
	error_reporter_start(ASC_USER_ERROR,NULL,0,NULL);
	FPRINTF(ASCERR,"%s",message ? message : "Dimension mismatch");
	if(label1 != NULL && d1 != NULL){
		FPRINTF(ASCERR,": %s has dimensions ",label1);
		PrintDimenReadable(ASCERR,d1);
	}
	if(label2 != NULL && d2 != NULL){
		FPRINTF(ASCERR,", but %s has dimensions ",label2);
		PrintDimenReadable(ASCERR,d2);
	}
	error_reporter_end_flush();
}


void DumpDimens(FILE *file){
  unsigned long c,len;
  len = gl_length(g_dimen_list);
  FPRINTF(file,"Dimensions dump\n");
  for(c=1;c<=len;c++) {
    PrintDimen(file,(dim_type *)gl_fetch(g_dimen_list,c));
    PUTC('\n',file);
  }
}


const dim_type *CheckDimensionsMatch(const dim_type *d1, const dim_type *d2){
  if (WILD(d1)) return d2;
  if (WILD(d2)||d1 == d2) return d1;
  if (CmpDimen(d1,d2)==0) return d1;
  return NULL;
}


void ParseDim(dim_type *dim, const char *c){
  int i;
  assert((dim!=NULL)&&(c!=NULL));
  ClearDimensions(dim);
  for( i=0; i<NUM_DIMENS && strcmp(c,DimNames[i]); i++ ) ;
  if( i>=NUM_DIMENS ) FPRINTF(ASCERR,"Dimension %s unknown.\n",c);
  else SetDimFraction(*dim,i,CreateFraction((FRACPART)1,(FRACPART)1));
}


char *DimName(const int ndx){
   if( ndx >= 0 && ndx < NUM_DIMENS )
      return( DimNames[ndx] );
   else
      return NULL;
}
