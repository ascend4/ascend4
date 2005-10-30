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

#ifndef __dstring_h_seen__
#define __dstring_h_seen__

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
 *	Returns the current length of the string.
 *	The return value is the number of non-NULL characters in
 *  the string, an integer.  dsPtr may not be NULL (not
 *  checked - crash probable).
 *
 *	@param dsPtr Asc_DString *, pointer to a structure
 *               describing the dynamic string to query.
 */
#define Asc_DStringLength(dsPtr) ((dsPtr)->length)


/*----------------------------------------------------------------------*/
/**
 *	Returns the current value of the string.
 *	The return value is a pointer to the first character in the
 *  string, a char*.  The client should not free or modify this
 *  value.  dsPtr may not be NULL (not checked - crash probable).
 *
 *	@param dsPtr Asc_DString *, pointer to a structure
 *               describing the dynamic string to query.
 */
#define Asc_DStringValue(dsPtr) ((dsPtr)->string)

/*----------------------------------------------------------------------*/
/**
 *	Appends more characters to the specified dynamic string.
 *	Length bytes from string (or all of string if length is less
 *	than zero) are added to the current value of the string.  If
 *  string is shorter than length, only the length of string 
 *  characters are appended.  The resulting value of the
 *  dynamic string is always null-terminated.
 *  <p />
 *  Memory	gets reallocated if needed to accomodate the string's
 *  new size.  Neither dpPtr nor string may be NULL (checked by
 *  assertion).
 *
 *  @param dsPtr  Structure describing dynamic string (non-NULL).
 *  @param string String to append (non-NULL).  If length is -1
 *                then this must be null-terminated.
 *  @param length Number of characters from string to append. If
 *                < 0, then append all of string, up to null at end.
 *  @return Returns the new value of the dynamic string.
 */
extern char *Asc_DStringAppend(register Asc_DString *dsPtr,
                               CONST char *string,
                               int length);

/*----------------------------------------------------------------------*/
/**
 *	Frees up any memory allocated for the dynamic string and
 *	reinitializes the string to an empty state.  The previous
 *  contents of the dynamic string are lost, and the new value
 *  is an empty string.  Note that dsPtr may not be NULL
 *  (checked by assertion).
 *
 *  @param dsPtr Structure describing dynamic string (non-NULL).
*/
extern void Asc_DStringFree(Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 *	Initializes a dynamic string, discarding any previous contents
 *	of the string.  Asc_DStringFree() should have been called already
 *	if the dynamic string was previously in use.  The dynamic string
 *  is initialized to be empty.  The passed pointer dsPtr may not
 *  be NULL (checked by assertion).
 *
 *  @param dsPtr Pointer to structure for dynamic string (non-NULL).
 */
extern void Asc_DStringInit(register Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 *	Returns a *copy* of the string held within the dynamic string.
 *  The returned string is owned by the caller, who is responsible
 *  for free'ing it when done with it.  The dynamic string itself
 *  is reinitialized to an empty string.  dsPtr may not be NULL
 *  (checked by assertion).
 *
 *  @param dsPtr dsPtr Dynamic string holding the returned result.
 *  @return Returns a copy of the original value of the dynamic string.
*/
extern char *Asc_DStringResult(Asc_DString *dsPtr);

/*----------------------------------------------------------------------*/
/**
 *	Truncates a dynamic string to a given length without freeing
 *	up its storage. 	The length of dsPtr is reduced to length
 *  unless it was already shorter than that.  Passing a length
 *  < 0 sets the new length to zero.  dsPtr may not be NULL
 *  (checked by assertion).
 *
 *  @param dsPtr  Structure describing dynamic string (non-NULL).
 *  @param length New maximum length for the dynamic string.
 */
extern void Asc_DStringTrunc(Asc_DString *dsPtr, int length);

/*----------------------------------------------------------------------*/
/**
 *	Sets the value of the dynamic string to the specified string.
 *  String must be null-terminated.
 *  Memory gets reallocated if needed to accomodate the string's new
 *  size.  Neither dsPtr nor string may be NULL (checked by assertion).
 *
 *  @param dsPtr   Structure describing dynamic string (non-NULL).
 *  @param string  String to append (non-NULL, null-terminated).
 *  @return Returns the new value of the dynamic string.
 */
extern char *Asc_DStringSet(Asc_DString *dsPtr, CONST char *string);

#endif /* __dstring_h_seen__  */

