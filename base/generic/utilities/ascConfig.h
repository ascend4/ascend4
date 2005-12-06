/*
 *  Basic Definitions for Ascend
 *  by Mark Thomas
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: ascConfig.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:13 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend
 *
 *  Copyright (C) 1997  Carnegie Mellon University
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
 *  This module defines the fundamental constants used by the rest of
 *  Ascend and pulls in system headers.
 *  There is not corresponding compiler.c. The variables
 *  declared in this header are defined in ascParse.y.
 */
 
/** @file
 *  Global configuration parameters.
 *  This header and tcl/tk headers are known to conflict. This header
 *  should be included AFTER tcl.h or tk.h, not before.
 */

#ifndef ASC_ASCCONFIG_H
#define ASC_ASCCONFIG_H

/**
	What kind of C compiler do we have?

    Use compiler flag -DDISABLE_C99 to force older ISO C
	This means no variadic macros, maybe other things too.
*/
#if defined(__STDC__) && defined(__STDC_VERSION__)
#  if __STDC_VERSION__>=199901L
#    ifndef DISABLE_C99
#      define HAVE_C99
#    endif
#  endif
#endif

/*
 * If we are in a tcl-infested file, define
 * CONST84 to be empty for back-compatibility with
 * tcl8.3
 */
#ifdef TCL_VERSION
# ifndef CONST84
#  define CONST84
#  define QUIET(x) x
#  define QUIET2(x) x
# else
/** use this macro to shut up const when const
    from tcl-land would be going into non-tcl C.
 */
#  define QUIET(x) ((char *)x)
#  define QUIET2(v) ((char **)v)
# endif
#endif

/*
 *
 *  Determine the Operating System we are building on
 *
 */
#ifndef __WIN32__
# if defined(_WIN32) || defined(WIN32)

/** Standard Windows define used in ASCEND. */
#  define __WIN32__

# else /* _WIN32 || WIN32 */

/* Some flavor of Unix */

#  ifdef __alpha
/** DEC Alpha running OSF */
#   define __ALPHA_OSF__
#  endif /* __alpha */

#  ifdef __hpux
/** HP running HP-UX */
#   define __HPUX__
#  endif /* __hpux */

#  ifdef _AIX
/** IBM RS6000 or PowerPC running AIX */
#   define __IBM_AIX__
#  endif /* _AIX */

#  ifdef __sgi
/** SGI running IRIX */
#   define __SGI_IRIX__
#  endif /* __sgi */

#  if defined(__sun) || defined(sun)
#   ifdef __SVR4
/** Sparc running Solaris 2.x (SunOS 5.x) */
#    define __SUN_SOLARIS__
#   else /* __SVR4 */
/** Sparc running SunOS 4.x (Solaris 1.x) */
#    define __SUN_SUNOS__
#   endif /* __SVR4 */
#  endif /* __sun || sun */

# endif /* _WIN32 || WIN32 */
#endif /* __WIN32__ */

/*
 *  Make certain we have proper limits defined
 */
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <sys/types.h>
#ifdef _OSF_SOURCE
# include <sys/syslimits.h>
#endif
/* for use practically everywhere */
#include<stdio.h>

#ifdef __WIN32__
# undef Status         /* jds20041229 - #define in tcl include/X11/XLib.h conflicts. */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>  /* jds20041212 - some limits defined in winnt.h (MAXLONG). */
# undef WIN32_LEAN_AND_MEAN
#endif

#ifndef PATH_MAX
/** Normally will come from stdio.h or limits.h 
 * POSIX values of PATH_MAX is 255, traditional is 1023 
 */
# define PATH_MAX 1023
#endif

/* the following numbers should have come through limits.h */
#ifndef SHRT_MAX
# define SHRT_MAX 32767
#endif
#ifndef INT_MAX
# define INT_MAX 2147483647
/* 32 bit machines */
#endif
#ifndef LONG_MAX
# ifdef __alpha
#  define LONG_MAX 9223372036854775807 /* 64 bit machines */
# else /* __alpha */
#  define LONG_MAX 2147483647 /* 32 bit machines */
# endif /* __alpha */
#endif /* LONG_MAX */

#ifndef MAXDOUBLE
# define MAXDOUBLE  DBL_MAX
#endif
#ifndef MAXINT
# define MAXINT     INT_MAX
#endif
#ifndef MAXUINT
# define MAXUINT    UINT_MAX
#endif
#ifndef MAXLONG
# define MAXLONG    LONG_MAX
#endif
#ifndef MAXULONG
# define MAXULONG   UILONG_MAX
#endif

/**
 * The largest number of digits we'd ever expect to see in a single
 * numeric value, * 3 or so for good measure.
 */
#define MAXIMUM_NUMERIC_LENGTH 80

/*
 *  Useful Headers
 */

/* for use by ascmalloc, which is also practically everywhere */
#include <assert.h>
#include <string.h>
/*
 * #include <malloc.h>
 *
 *  malloc() is defined in <stdlib.h> in ANSI-C
 */

#define MAXTOKENLENGTH 1024	/**< Maximum token size.  
                                 Most significant for identifiers and strings */
#ifndef FALSE
# define FALSE 0
# define TRUE  1
#endif

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef ABS
# define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef CONST
# define CONST const
#endif

#define VOIDPTR void *

/* Updated the following to use new Wiki URLS */
#define ASC_MILD_BUGMAIL "https://pse.cheme.cmu.edu/wiki/view/Ascend/BugReport"
#define ASC_BIG_BUGMAIL "https://pse.cheme.cmu.edu/wiki/view/Ascend/BugReport"

extern FILE *g_ascend_errors;         /**< File stream to receive error messages. */
extern FILE *g_ascend_warnings;       /**< File stream to receive warning messages. */
extern FILE *g_ascend_information;    /**< File stream to receive general messages. */

/* NB For error messages to be correctly captured, all output needs to go to stderr in error.h */
#ifndef ASCERR
# define ASCERR g_ascend_errors
# define ASCWAR g_ascend_warnings
# define ASCINF g_ascend_information
#endif
/*
 *  file pointers for whine. they default to stderr. if you change
 *  them, be sure to change them to valid files.
 */

#define	BYTESIZE   8
#define	WORDSIZE   (sizeof(unsigned)*BYTESIZE)
#ifndef TRUE
# define	TRUE       1
#endif
#ifndef FALSE
# define	FALSE      0
#endif
typedef	char       *POINTER;       /**< Generic pointer. */
typedef	char       boolean;        /**< Boolean type. */
typedef	int        ALIGN;          /**< Data alignment. */
/**
 *  real64:
 *  change this typedef and all the solver code should change
 *  precision automatically. The code is predicated on 64bit ieee math.
 *  change the typedef as needed to get 64bit numbers. In particular
 *  crays may change this to float.
 *  int32:
 *  uint32;
 *  a 32bit int. change this typedef and all the solver code changes
 *  with it. may be of use for compiler which default int to 16bits
 *  equivalent to -i4 on most f77 compilers.
 */
typedef double     real64;         /* a value */
typedef	int        int32;          /**< a row/col/var/rel index number */
typedef	unsigned   uint32;


/**
 * MAXREFCOUNT should always be defined to be the maximum value
 * that the reference count attribute of reference count objects
 * can take. It is unreasonable that a refcount be 64 bits, so
 * it is 32.
 */
#define MAXREFCOUNT UINT_MAX
#define REFCOUNT_T unsigned int

/*
 *  A bit of header magic to address the question of NULL being implementation
 *  defined, though virtually all systems of interest define it as 0 or 0L,
 *  if we are to claim ANSI conformance.
 *
 *  IEEE 754 arithmetic implies all 0 bits <--> 0.0, so there isn't
 *  a problem where this is concerned. We have assumed ieee math.
 *
 *  Notes to myself:
 *  Put this in base.h and suggest its insertion in
 *  compiler.h and perhaps interface1.h.
 *  Any place that calloc is being used to allocate arrays of NULL pointers,
 *  put a proper #if to replace it with a call to malloc and an appropriate
 *  NULLing function.
 *
 *  Ben Allan, Jan 6, 1994.
 */

/*
 *  NOTE: The below if works for 0 or 0L, which is to say I assume it
 *  works for 0 and I have tested that it works for 0L. Next we define
 *  the cheap versions of null testing for machine with 0 == NULL
 *
 *  These are the proper ANSI C tests for NULL on 0!=NULL machines, should
 *  we ever run into them.
 */
#ifdef NULL
/* ok, so the machine has a NULL defined. */
# ifndef ISNULL
/* and we've not got an ISNULL function */
#  define ISNULL(a) ((a) == NULL)
#  define NOTNULL(a) ((a) != NULL)
#  endif /* ISNULL */
# endif /* NULL */

#ifndef NULL
# error "Null not defined by the time ascConfig.h seen"
#endif

#ifndef UNUSED_PARAMETER
/** Standardize the treatment of unreferenced function parameters. */
# define UNUSED_PARAMETER(p) (void)(p)
#endif

/* ASCEND assertion defines */
/** Exit status code for failed assertion. */
#define ASCERR_ASSERTION_FAILED 100
/* 
 *  For now, asc_assert tied to NDEBUG just like regular assert.
 *  Having a separate assertion deactivator (ASC_NO_ASSERTIONS)
 *  gives us the ability to decouple from NDEBUG and leave 
 *  assertions active in release code if desired.
 */
#ifdef NDEBUG
/** If defined, asc_assert() will be removed from code. */
# define ASC_NO_ASSERTIONS
#endif

/*
 *
 *  Platform specific fixes
 *
 */

/*
 * The following definitions set up the proper options for Windows
 * compilers.  We use this method because there is no autoconf equivalent.
 */
#ifdef __WIN32__

/*
 *  use the ASCEND printf substitutes
 */
# ifndef USE_ASC_PRINTF
#  define USE_ASC_PRINTF
# endif /* USE_ASC_PRINTF */

/*
 *  build the Tk Console
 */
# ifndef ASC_USE_TK_CONSOLE
#  define ASC_USE_TK_CONSOLE                                                              
# endif /* ASC_USE_TK_CONSOLE */

/*
 * make macros so that DLLs can see nominated internal C functions.
 * Unix programmers might also use the presence of these macros to figure
 * out which APIs are to be regarded as more stable.
 */
# if defined(ASC_BUILD_DLL)
#  define DLEXPORT __declspec(dllexport)
# elif defined(ASC_BUILD_LIB)
#  define DLEXPORT
# else
#  define DLEXPORT __declspec(dllimport)
# endif
#else /* not __WIN32__ isms */
# ifdef HAVE_GCCVISIBILITYPATCH
#  define DLEXPORT __attribute__ ((visibility("default")))
# else
#  define DLEXPORT
# endif
#endif /* __WIN32__ */

/* use signals by default, but disable with configure. */
#ifdef ASC_NO_TRAPS
/** Don't use signals. */
# define NO_SIGNAL_TRAPS 1
#else
/** Use signals. */
# undef NO_SIGNAL_TRAPS
#endif

/*------------------------------------*
	OUTPUT FILE* MAPPINGS

	On Windows, we have to use these special Asc_Printf (etc)
	routines in order that output can be captured correctly by the GUI.

	On non-Windows platforms, allow error.h to 
*/
#ifdef USE_ASC_PRINTF
#  define ASC_PRINTF  Asc_Printf
#  define ASC_FPRINTF Asc_FPrintf
#  define ASC_VFPRINTF Asc_VFPrintf
#  define ASC_FFLUSH  Asc_FFlush
#  define ASC_PUTC    Asc_FPutc
#  define ASC_FPUTC   Asc_FPutc
#  define ASC_PUTCHAR Asc_Putchar
#  include <stdarg.h>
#  include "utilities/ascPrint.h"
#else
#  define ASC_PRINTF  printf
#  define ASC_FPRINTF fprintf
#  define ASC_VFPRINTF vfprintf
#  define ASC_FFLUSH  fflush
#  define ASC_PUTC    putc
#  define ASC_FPUTC   fputc
#  define ASC_PUTCHAR putchar
#endif

/* error.h will define handlers for
		FPRINTF
		FPUTC
		FFLUSH
*/
#include "utilities/error.h"

/*	These are the remaining output macros that need to be mapped
*/
#define PRINTF ASC_PRINTF
#define PUTCHAR ASC_PUTCHAR
/*------------------------------------*/


#ifndef __SUN_SUNOS__
/* do this for non-sun machines and for sun-solaris machines */
# ifndef roundup
#  define	rounddown(num,mod)	((num)-(num)%(mod))
#  define	roundup(num,mod)	rounddown((num)+(mod)-1,(mod))
#  define	roundoff(num,mod)	rounddown((num)+((mod)/2),(mod))
# endif /* roundup */
#endif /* __SUN_SUNOS__ */


/* These define integer divide and modulus mathematically correctly */
#define	imod(num,den)	((num)%(den) + ((num)%(den)<0 ? (den) : 0))
#define	idiv(num,den)	((num)/(den) + ((num)%(den)<0))
#define	array_length(a)	(sizeof(a)/sizeof((a)[0]))

/*
 * the following patch up IEEE754isms that some systems can't seem to
 * get right.
 */
#define FPRESET (void)0
#ifdef __WIN32__
/* renamed in some __WIN32__ compiler systems */
#  ifndef NO_RENAME_IEEE_FUNCTIONS
#    undef finite
#    define finite(x) _finite(x)
#    undef isnan
#    define isnan(x)  _isnan(x)
#    undef isinf
#    define isinf(x)  _isinf(x)
#  endif
#  undef FPRESET
#  define FPRESET _fpreset()
#endif  /* __WIN32__ */

#ifdef __SUN_SUNOS__
 /** not properly headered in math.h or ieee*.h */
 extern int finite(double);
#endif

#endif /* ASC_ASCCONFIG_H */

