/*
	ASCEND Language Interpreter
	AscPanic by Mark Thomas, created 15 May 1997
	Copyright (C) 2005 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
	This file is part of the SLV solver.
*/

/** @file
	Ascend Panic - fatal error handling.

	Requires
	#include <stdarg.h>
	#include "utilities/ascConfig.h"
*/

/* removed changelog -- see ViewCVS for full history */

#ifndef ASC_ASCPANIC_H
#define ASC_ASCPANIC_H

#ifndef __GNUC__
# ifndef __FUNCTION__
#  define __FUNCTION__ NULL
# endif
#endif

NORETURN ASC_DLLSPEC(void) asc_panic_line(
		const int status, const char *file, const int line, const char *function,
		const char *format, ...
);

/* for 'Asc_Panic', use a var-args macro to get local line numbers if possible */

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define Asc_Panic(STAT,FUNC,ARGS...) asc_panic_line(STAT,__FILE__,__LINE__,__func__, ##ARGS)
#else
# define Asc_Panic asc_panic

NORETURN ASC_DLLSPEC(void) asc_panic(
		CONST int status, CONST char *function,
		CONST char *format, ...
);

#endif

/**
	Our assertion macros. Use asc_panic_line to report & handle assertion failure. Disabled if ASC_NO_ASSERTIONS is defined.
*/
#ifdef ASC_NO_ASSERTIONS
# define asc_assert(x) ((void)0)
# define ASC_ASSERT_LT(A,B) ((void)0)
# define ASC_ASSERT_EQ(A,B) ((void)0)
# define ASC_ASSERT_RANGE(A,B,C) ((void)0)

#else
# define asc_assert(cond) \
	((cond) ? (void)0 : asc_panic_line(ASCERR_ASSERTION_FAILED\
		, __FILE__, __LINE__, __FUNCTION__\
		,"Assertion failed: %s", #cond))

#define ASC_ASSERT_LT(A,B) \
	(((A)<(B)) ? (void)0 : asc_panic_line(ASCERR_ASSERTION_FAILED\
		, __FILE__, __LINE__, __FUNCTION__\
		,"Assertion failed: %s < %s (lhs = %f, rhs = %f)" \
		, #A, #B \
		, (float)A, (float)B))

#define ASC_ASSERT_EQ(A,B) \
	(((A)==(B)) ? (void)0 : asc_panic_line(ASCERR_ASSERTION_FAILED\
		, __FILE__, __LINE__, __FUNCTION__\
		,"Assertion failed: %s < %s (lhs = %f, rhs = %f)" \
		, #A, #B \
		, (float)A, (float)B))

#define ASC_ASSERT_RANGE(A,B,C) \
	((A) >= (B) && (A) < (C) ? (void)0 : (void)asc_panic_line(ASCERR_ASSERTION_FAILED\
		, __FILE__, __LINE__, __FUNCTION__\
		,"Assertion failed: %s < %s < %s (val = %f, low = %f, up = %f)" \
		, #B, #A, #C \
		, (float)A, (float)B), (float)C)

#endif

/**< Print fatal error message, run callback function & (usually) exit the program.

	@param status   Status code passed by the calling function.
	@param function Pointer to the name of the calling function.
	@param format   printf-style format string for VAR_ARGS to follow.

	This is the fatal error handler for the ASCEND system.  It prints
	the printf-style variable arguments using the specified format to
	ASCERR file handle.  The message is formatted with a header
	(e.g. 'ASCEND PANIC!!) and the name of the function (if non-NULL),
	followed by the variables & format passed as arguments.  ASCERR
	should have been initialized to a valid file stream or else the
	message will not be printed (checked by assertion). @par

	If a valid file name has been previously set using
	Asc_PanicSetOutfile(), the message is printed to this file also.
	Under Windows, a MessageBox will also be displayed with the
	message. @par

	If a callback has been set using Asc_PanicSetCallback(), the
	registered function will be called with the specified status.
	If the callback returns non-NULL, then exit() is called to end
	the program.  This is the default behavior.  If the callback
	is able to resolve the problem, then it should return zero and
	Asc_Panic() will just return.  This will be useful mostly for
	testing purposes, and should be used with caution.
*/

ASC_DLLSPEC(void ) Asc_PanicSetOutfile(CONST char *filename);
/**< Sets a file name for reporting of Asc_Panic() messages.

	@param filename Pointer to the name of the file to print messages to.

	Calling this function with a non-NULL "filename" will cause
	Asc_Panic() to write panic messages to "filename" in addition to the
	ASCERR file handle.  Passing in a "filename" of NULL causes panic
	messages not to be written to disk---this undoes the effect of
	previous calls to Asc_PanicSetOutfile().
 */

typedef int (*PanicCallbackFunc)(int);
/**< Signature of the callback function called by Asc_Panic().

	@param the status code passed to Asc_Panic() by the original caller.
	@return nonzero if ASCEND should exit, 0 if Asc_Panic should just return.

	This functionality is provided primarily for internal testing
	purposes.  It should be used with extreme caution in release
	code.  Asc_Panic() is called from all over ASCEND for many
	error conditions, and current calls assume no return.
*/

ASC_DLLSPEC(PanicCallbackFunc ) Asc_PanicSetCallback(PanicCallbackFunc func);
/**< Registers a callback function to be called by Asc_Panic().

	@param func Pointer to the new function to call during an Asc_Panic().
	@return A pointer to the previously-registered callback, or NULL
		if none was registered.

	This allows the user to specify a cleanup function to be
	called during a fatal error.

	@see PanicCallbackFunc for the form this callback function takes.
*/

ASC_DLLSPEC(void ) Asc_PanicDisplayMessageBox(int is_displayed);
/**<
	Controls whether a MessageBox is displayed by Asc_Panic() on Windows.

	@param is_displayed if non-zero, messagebox should be displayed, else not.

	Has no effect on non-Windows platforms.
*/

#endif /* ASC_ASCPANIC_H */
