/*
 *  Dynamic String Utilities
 *
 *  Ripped off from the tcl collection. - September 21, 1995
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
#include "general/dstring.h"


void Asc_DStringInit(dsPtr)
     register Asc_DString *dsPtr;	/* Pointer to structure for
	        			 * dynamic string. */
{
  dsPtr->string = dsPtr->staticSpace;
  dsPtr->length = 0;
  dsPtr->spaceAvl = ASC_DSTRING_STATIC_SIZE;
  dsPtr->staticSpace[0] = 0;
}



extern char *Asc_DStringAppend(dsPtr, string, length)
     register Asc_DString *dsPtr;	/* Structure describing dynamic
	        			 * string. */
     CONST char *string;		/* String to append.  If length is
	        			 * -1 then this must be
        				 * null-terminated. */
     int length;			/* Number of characters from string
	        			 * to append.  If < 0, then append all
        				 * of string, up to null at end. */
{
  int newSize;
  char *newString;

  if (length < 0) {
    length = strlen(string);
  }
  newSize = length + dsPtr->length;

  /*
   * Allocate a larger buffer for the string if the current one isn't
   * large enough.  Allocate extra space in the new buffer so that there
   * will be room to grow before we have to allocate again.
   */

  if (newSize >= dsPtr->spaceAvl) {
    dsPtr->spaceAvl = newSize*2;
    newString = (char *) malloc((unsigned) dsPtr->spaceAvl);
    strcpy(newString, dsPtr->string);
    if (dsPtr->string != dsPtr->staticSpace) {
      ascfree(dsPtr->string);
    }
    dsPtr->string = newString;
  }

  /*
   * Copy the new string into the buffer at the end of the old
   * one.
   */

  strncpy(dsPtr->string + dsPtr->length, string, length);
  dsPtr->length += length;
  dsPtr->string[dsPtr->length] = 0;
  return dsPtr->string;
}



extern void Asc_DStringTrunc(dsPtr, length)
     register Asc_DString *dsPtr;	/* Structure describing dynamic
                                         * string. */
     int length;			/* New length for dynamic string. */
{
  if (length < 0) {
    length = 0;
  }
  if (length < dsPtr->length) {
    dsPtr->length = length;
    dsPtr->string[length] = 0;
  }
}



extern void Asc_DStringFree(dsPtr)
     register Asc_DString *dsPtr;	/* Structure describing dynamic
	        			 * string. */
{
  if (dsPtr->string != dsPtr->staticSpace) {
    ascfree(dsPtr->string);
  }
  dsPtr->string = dsPtr->staticSpace;
  dsPtr->length = 0;
  dsPtr->spaceAvl = ASC_DSTRING_STATIC_SIZE;
  dsPtr->staticSpace[0] = 0;
}



extern char *Asc_DStringResult(dsPtr)
     Asc_DString *dsPtr;		/* Dynamic string that is to become
	        			 * the returned result. */
{
  register char *result;

  result = (char *)ascmalloc(strlen(dsPtr->string)+1);
  strcpy(result,dsPtr->string);
  Asc_DStringFree(dsPtr);
  return result;
}
