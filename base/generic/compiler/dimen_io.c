/*
 *  Dimensions Output Routine
 *  by Tom Epperly
 *  Created: 2/14/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: dimen_io.c,v $
 *  Date last modified: $Date: 1998/04/11 01:31:06 $
 *  Last modified by: $Author: ballan $
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
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/dstring.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "dimen_io.h"

#ifndef lint
static CONST char DimenIORCSid[] = "$Id: dimen_io.c,v 1.8 1998/04/11 01:31:06 ballan Exp $";
#endif

static
void WriteFrac(FILE *f, struct fraction frac, CONST char *str, int *CONST p)
{
  if (Numerator(frac)){
    if (*p) PUTC('*',f);
    (*p) = 1;
    if (Denominator(frac)==1) {
      FPRINTF(f,"%s^%d",str,Numerator(frac));
    } else {
      FPRINTF(f,"%s^(%d/%d)",str,Numerator(frac),Denominator(frac));
    }
  }
}

void WriteDimensions(FILE *f, CONST dim_type *dimp)
{
  struct fraction frac;
  int printed=0;
  if (IsWild(dimp)) {
    FPRINTF(f,"*");
  } else {
    int i;
    for( i=0; i<NUM_DIMENS; i++ ) {
       frac = GetDimFraction(*dimp,i);
       WriteFrac(f,frac,DimName(i),&printed);
    }
    if (!printed) FPRINTF(f,"dimensionless");
  }
}

char *WriteDimensionString(CONST dim_type *p)
{
  Asc_DString ds, *dsPtr;
  char *result;
  int numseen = 0, i, k;
  char expo[40];
  if (p==NULL) {
    return NULL;
  }
  if (IsWild(p)) {
    result = ASC_NEW_ARRAY(char,2);
    sprintf(result,"*");
    return result;
  }
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  for (i=0; i < NUM_DIMENS; i++) {
    k = GetDimPower(*(p),i);
    if (k > 0) {
      if (numseen) {
        Asc_DStringAppend(dsPtr,"*",1);
      }
      Asc_DStringAppend(dsPtr,DimName(i),-1);
      if (k > 1) {
        sprintf(expo,"^%d",k);
        Asc_DStringAppend(dsPtr,expo,-1);
      }
      numseen = 1;
    }
  }
  if (!numseen) {
    Asc_DStringAppend(dsPtr,"1",1);
  }
  for (i=0; i < NUM_DIMENS; i++) {
    k = GetDimPower(*(p),i);
    if (k < 0) {
      Asc_DStringAppend(dsPtr,"/",1);
      Asc_DStringAppend(dsPtr,DimName(i),-1);
      if (k < -1) {
        sprintf(expo,"^%d",-k);
        Asc_DStringAppend(dsPtr,expo,-1);
      }
    }
  }
  result = Asc_DStringResult(dsPtr);
  return result;
}
