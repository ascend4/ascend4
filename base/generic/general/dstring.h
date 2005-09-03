/*
 *  Dynamic String Utilities
 *
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: dstring.h,v $
 *  Date last modified: $Date: 1997/07/18 11:41:38 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Dynamic String Utilities.
 *  These routines are modifications of TCL code by John Osterhout at 
 *  Berkeley, as allowed by the TCL distribution license.  See dstring.c 
 *  for the details. There are minor differences internally.
 *  <pre>
 *  Requires:
 *        #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef _DSTRING_H
#define _DSTRING_H

#define ASC_RESULT_SIZE 200
#define ASC_DSTRING_STATIC_SIZE 200
#define ASC_ALL_STRING -1

/** Dynamic string data structure. */
typedef struct Asc_DString {
  char *string;   /**< Points to beginning of string:  either
                       staticSpace below or a malloc'ed array. */
  int length;     /**< Number of non-NULL characters in the string. */
  int spaceAvl;   /**< Total number of bytes available for the
                       string and its terminating NULL char. */
  char staticSpace[ASC_DSTRING_STATIC_SIZE];
                   /**< Space to use in common case where string is small. */
} Asc_DString;

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringLength --                                          -->
 *
 *	Return the current length of the string.
 *
 * Results:
 *	The return value is the number of non-NULL characters in
 *      the string, an integer.<br><br>
 *
 * Side effects:
 *	None.
 */
#define Asc_DStringLength(dsPtr) ((dsPtr)->length)


/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringValue --                                           -->
 *
 *	Return the curent value of the string.
 *
 * Results:
 *	The return value is a pointer to the first character in the string,
 *      a char*.  The client should not free or modify this value.<br><br>
 *
 * Side effects:
 *	None.
 */
#define Asc_DStringValue(dsPtr) ((dsPtr)->string)

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringAppend --                                          -->
 *
 *	Append more characters to the current value of a dynamic string.
 *
 * Results:
 *	The return value is a pointer to the dynamic string's new value.<br><br>
 *
 * Side effects:
 *	Length bytes from string (or all of string if length is less
 *	than zero) are added to the current value of the string.  Memory
 *	gets reallocated if needed to accomodate the string's new size.
 */
extern char *Asc_DStringAppend(Asc_DString *dsPtr,
                               CONST char *string,
                               int length);

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringFree --                                            -->
 *
 *	Frees up any memory allocated for the dynamic string and
 *	reinitializes the string to an empty state.
 *
 * Results:
 *	None.<br><br>
 *
 * Side effects:
 *	The previous contents of the dynamic string are lost, and
 *	the new value is an empty string.
 */
extern void Asc_DStringFree(Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringInit --                                            -->
 *
 *	Initializes a dynamic string, discarding any previous contents
 *	of the string (Asc_DStringFree should have been called already               
 *	if the dynamic string was previously in use).
 *
 * Results:
 *	None.<br><br>
 *
 * Side effects:
 *	The dynamic string is initialized to be empty.
 */
extern void Asc_DStringInit(Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringResult --                                          -->
 *
 *	This procedure returns a *copy* of the string held within the
 *	dynamic string. The string itself is reinitialized
 *	to an empty string.<br><br>
 *
 * Results:
 *	The string held.<br><br>
 *
 * Side effects:
 *	DsPtr is reinitialized to an empty string.
 */
extern char *Asc_DStringResult(Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 * <!--  Asc_DStringTrunc --                                           -->
 *
 *	Truncate a dynamic string to a given length without freeing
 *	up its storage.
 *
 * Results:
 *	None.<br><br>
 *
 * Side effects:
 *	The length of dsPtr is reduced to length unless it was already
 *	shorter than that.
 */
extern void Asc_DStringTrunc(Asc_DString *dsPtr, int length);

#endif /* _DSTRING_H  */

