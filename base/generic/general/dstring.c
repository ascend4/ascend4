/*
 *  Dynamic String Utilities
 *
 *  Taken from the tcl collection. - September 21, 1995
 *  by Kirk Abbott, and lightly modified to suit the neeeds of
 *  ASCEND.
 *
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: dstring.c,v $
 *  Date last modified: $Date: 1997/07/18 11:41:34 $
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

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/dstring.h"


void Asc_DStringInit(register Asc_DString *dsPtr)
{
  asc_assert(NULL != dsPtr);
  dsPtr->string = dsPtr->staticSpace;
  dsPtr->length = 0;
  dsPtr->spaceAvl = ASC_DSTRING_STATIC_SIZE;
  dsPtr->staticSpace[0] = '\0';
}


char *Asc_DStringSet(Asc_DString *dsPtr, CONST char *string)
{
  int length;
  char *newString;

  asc_assert(NULL != dsPtr);
  asc_assert(NULL != string);
  asc_assert((int)(MAXINT - strlen(string)) >= 0);

  length = (int)strlen(string);

  /*
   * Allocate a larger buffer for the string if the current one isn't
   * large enough.  Allocate extra space in the new buffer so that there
   * will be room to grow before we have to allocate again.
   */
  if (length >= dsPtr->spaceAvl) {
    dsPtr->spaceAvl = length*2;
    newString = (char *) ascmalloc((size_t) dsPtr->spaceAvl);
    if (dsPtr->string != dsPtr->staticSpace) {
      ascfree(dsPtr->string);
    }
    dsPtr->string = newString;
  }

  /*
   * Copy the new string into the buffer
   */
  strncpy(dsPtr->string, string, (size_t)length);
  dsPtr->length = length;
  dsPtr->string[dsPtr->length] = '\0';
  return dsPtr->string;
}



char *Asc_DStringAppend(register Asc_DString *dsPtr,
                        CONST char *string,
                        int length)
{
  int str_length;
  int newSize;
  char *newString;

  asc_assert(NULL != dsPtr);
  asc_assert(NULL != string);
  asc_assert((int)(MAXINT - strlen(string)) >= 0);

  str_length = (int)strlen(string);

  if (length < 0) {
    length = str_length;
  }
  else {
    length = MIN(length, str_length);
  }

  newSize = length + dsPtr->length;

  /*
   * Allocate a larger buffer for the string if the current one isn't
   * large enough.  Allocate extra space in the new buffer so that there
   * will be room to grow before we have to allocate again.
   */
  if (newSize >= dsPtr->spaceAvl) {
    dsPtr->spaceAvl = newSize*2;
    newString = (char *) ascmalloc((size_t) dsPtr->spaceAvl);
    strcpy(newString, dsPtr->string);
    if (dsPtr->string != dsPtr->staticSpace) {
      ascfree(dsPtr->string);
    }
    dsPtr->string = newString;
  }

  /*
   * Copy the new string into the buffer at the end of the old one.
   */
  strncpy(dsPtr->string + dsPtr->length, string, (size_t)length);
  dsPtr->length += length;
  dsPtr->string[dsPtr->length] = '\0';
  return dsPtr->string;
}



void Asc_DStringTrunc(register Asc_DString *dsPtr, int length)
{
  asc_assert(NULL != dsPtr);
  
  if (length < 0) {
    length = 0;
  }
  if (length < dsPtr->length) {
    dsPtr->length = length;
    dsPtr->string[length] = '\0';
  }
}



void Asc_DStringFree(register Asc_DString *dsPtr)
{
  asc_assert(NULL != dsPtr);
  if (dsPtr->string != dsPtr->staticSpace) {
    ascfree(dsPtr->string);
  }
  dsPtr->string = dsPtr->staticSpace;
  dsPtr->length = 0;
  dsPtr->spaceAvl = ASC_DSTRING_STATIC_SIZE;
  dsPtr->staticSpace[0] = '\0';
}



char *Asc_DStringResult(Asc_DString *dsPtr)
{
  register char *result;

  asc_assert (NULL != dsPtr);
  result = (char *)ascmalloc(strlen(dsPtr->string)+1);
  strcpy(result,dsPtr->string);
  Asc_DStringFree(dsPtr);
  return result;
}
