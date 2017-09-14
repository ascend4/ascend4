/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	This file provides error reporting to a callback function via
	ASCEND's FPRINTF(ASCERR,...) syntax. It is anticipated that
	this would gradually be expanded to including richer reporting
	of errors with severity and source file name and line numbers.

	Usage:
		error_reporter_start(<error-severity>,<filepath>,<linenumber>);
		FPRINTF(ASCERR,"half of ");
		FPRINTF(ASCERR,"your message");
		error_reporter_end_flush();

	or:
		error_reporter_start(<error-severity>,<filepath>,<linenumber>
				,"format string %s %d etc",<printf-args>,...");

	The first form allows you to use multiple FPRINTF statements to
	generate your error message. The second form assumes that your
	entire message will be contained in a single statement.

	Error severities are
		ASC_(USER|PROG)_(NOTE|WARNING|ERROR)
	and ASC_USER_SUCCESS
*//*
	by John Pye
	2005
*/
#ifndef ASC_ERROR_H
#define ASC_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
	ascConfig defines ASC_FPRINTF etc, the routines to provide
	default 'real' printf behaviour on this platform. (As
	opposed to the sneaky stuff that FPRINTF does in this header)
*/
#include <ascend/general/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascPrint.h>

/**	@addtogroup utilities_error Utilities Error Message Handling
	@{
*/

/**
	FPRINTF(ASCERR,...) messages will by default be treated
	by the error_reporter as ASC_PROG_NOTE messages. These will
    gradually need to be replaced with error_severity_t values
	that accurately reflect the nature of the error.
*/
#define FPRINTF fprintf_error_reporter
#define FPUTC fputc_error_reporter
#define PUTC fputc_error_reporter
#define FFLUSH fflush_error_reporter

/*
	By default, don't use coloured output on any terminals. We will reintroduce
	this later, hopefully. It should be done using CURSES, instead of directly
	using xterm codes. But that brings its own problems on MinGW and Windows...
*/

/**
	Error severity codes. This will be used to visually
	the seriousness of errors. ASC_PROG_ERRORs for example
	might be red, or be highlighted with a (!) icon, etc.
*/
typedef enum error_severity_enum{
    ASC_USER_SUCCESS=0
   ,ASC_USER_NOTE=1    /**< a note to the user */
   ,ASC_USER_WARNING=2 /**< the user has done something bad but tolerable */
   ,ASC_USER_ERROR=4   /**< the user has done something wrong */
   ,ASC_PROG_NOTE=8    /**< a note for the programmer */
   ,ASC_PROG_WARNING=16/**< the program encounters an unexpected state */
   ,ASC_PROG_ERROR=32  /**< the program has failed but can ignore and continue (maybe) */
   ,ASC_PROG_FATAL=64  /**< fatal error, program will exit */
} error_severity_t;

#define ASC_ERR_ERR (ASC_PROG_ERROR | ASC_USER_ERROR | ASC_PROG_FATAL)

/**
	Variadic macros to allow nice succint logging and error reporting
	calls from C dialects that support them (GCC, C99 and others)

	If you don't support variadic macros, you will still get the messages
	but without the file/function/line number.
*/
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define ERROR_REPORTER_HERE(SEV,args...) error_reporter(SEV,__FILE__, __LINE__, __func__, ##args)
# define ERROR_REPORTER_NOLINE(SEV,args...) error_reporter(SEV, NULL, 0, NULL, ##args)
# define CONSOLE_DEBUG(args...) ((void)(color_on(stderr,ASC_FG_BRIGHTBLUE) + \
		fprintf(stderr, "%s:%d ",__FILE__,__LINE__) + \
		color_on(stderr,ASC_FG_BRIGHTRED) + \
		fprintf(stderr, "(%s)", __func__) + \
		color_on(stderr,ASC_FG_BRIGHTBLUE) + \
		fprintf(stderr, ": ") + \
		fprintf(stderr, ##args) + \
        fprintf(stderr, "\n") + color_off(stderr)))

# define ERROR_REPORTER_START_HERE(SEV) error_reporter_start(SEV,__FILE__,__LINE__,__func__);

#elif defined(HAVE_C99)
# define ERROR_REPORTER_HERE(SEV,...) error_reporter(SEV,__FILE__,__LINE__,__func__, ## __VA_ARGS__)
# define ERROR_REPORTER_NOLINE(SEV,...) error_reporter(SEV,NULL,0,NULL, ## __VA_ARGS__)
# define CONSOLE_DEBUG(...) (color_on(stderr,BRIGHTBLUE) + fprintf(stderr, "%s:%d (%s): ", __FILE__,__LINE__,__func__) + \
                             fprintf(stderr, ##__VA_ARGS__) + \
                             fprintf(stderr, "\n") + color_off(stderr))
# define ERROR_REPORTER_START_HERE(SEV) error_reporter_start(SEV,__FILE__,__LINE__,__func__);

#elif defined(_MSC_VER) && _MSC_VER >= 1400 /* Microsoft Visual C++ 2005 or newer */
#  define ERROR_REPORTER_START_HERE(SEV) error_reporter_start(SEV,__FILE__,__LINE__,__FUNCTION__);
#  define ERROR_REPORTER_HERE(SEV,...)  error_reporter(SEV,__FILE__,__LINE__,__FUNCTION__, __VA_ARGS__)
#  define ERROR_REPORTER_NOLINE(SEV,...) error_reporter(SEV,NULL,0,NULL, __VA_ARGS__)
#  define CONSOLE_DEBUG(...)   (fprintf(stderr, "%s:%d (%s): ", __FILE__,__LINE__,__FUNCTION__) + \
                                fprintf(stderr, __VA_ARGS__) + \
                                fprintf(stderr, "\n"))
#else /* workaround for compilers without variadic macros: last resort */
# define NO_VARIADIC_MACROS
# define ERROR_REPORTER_HERE error_reporter_here
# define ERROR_REPORTER_NOLINE error_reporter_noline
# define CONSOLE_DEBUG console_debug
# define ERROR_REPORTER_START_HERE(SEV) error_reporter_start(SEV,__FILE__,__LINE__,"[function?]");
ASC_DLLSPEC int error_reporter_note_no_line(const char *fmt,...);
ASC_DLLSPEC int error_reporter_here(const error_severity_t sev, const char *fmt,...);
ASC_DLLSPEC int error_reporter_noline(const error_severity_t sev, const char *fmt,...);
ASC_DLLSPEC int console_debug(const char *fmt,...);
#endif

#define ERROR_REPORTER_START_NOLINE(SEV) error_reporter_start(SEV,NULL,0,NULL);

/** An alias for ASC_PROG_ERROR */
#define ASC_PROG_ERR ASC_PROG_ERROR

#define ERROR_REPORTER_MAX_MSG 4096 /* no particular reason */

typedef struct{
	unsigned char iscaching; /** set to true for fprintf_error_reporter to do its work */
	error_severity_t sev;
	const char *filename;
	int line;
	const char *func;
	char msg[ERROR_REPORTER_MAX_MSG];
} error_reporter_meta_t;

/**
	This structure provides a means for observing when errors were reported in 
	nested code, so that we can be sure that high-level routines can report
	error status correctly. This is a bit of a hack because ASCEND doesn't
	always report errors through return values in a straightforward way.
	
	This structure also provides the ability to suppress error_reporter output
	(but not CONSOLE_DEBUG though -- at this stage), which can be useful when
	running batch tasks etc. Potentially (with extra coding) allows just the 
	first few of a long series of errors to be reported.

	Usage will be

		bool has_error = 0;
		bool is_caching = 0;
		error_reporter_tree_t *tree = error_reporter_tree_start(is_caching);
		do_subordinate_tasks();
		has_error = error_reporter_tree_has_error(tree);
		if(has_error){
			error_reporter_tree_end(tree);
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"task failed");
		}else{
			error_reporter_tree_end_clear(tree);
			ERROR_REPORTER_NOLINE(ASC_USER_SUCCESS,"task succeeded");
		}

	The error_reporter_tree_end() call will output any nested errors in the case
	where it is a caching tree without any caching parent(s). If there are
	parents, the errors will be kept for high-up scrutiny. If there are no
	parents, the errors will the cleared away once they are output.

	The error_reporter_tree_end_clear() call will NOT output any nested errors
	and will clear away any nested errors making them invisible to higher-up
	nodes in the error tree. This allow errors which have occurred, eg in 
	failed attempts to solve equations, to be suppressed in the case that
	alternative actions are taken (eg re-solving with different parameters).

	Both of these commands, error_reporter_tree_end() and
	error_reporter_tree_end_clear() close the current error tree and move the
	current-tree-pointer up to the parent level if it exists.

	If an 'error_reporter' call is found *inside* an an 'error_reporter_tree_start'
	and 'error_reporter_tree_end', the error message is recorded in the
	error_reporter_tree_t.
*/
typedef struct ErrorReporterTree{
	error_reporter_meta_t *err;
	int iscaching;
	struct ErrorReporterTree *head; /**< first on the list of child errors */
	struct ErrorReporterTree *tail; /**< last on the list of child errors */
	struct ErrorReporterTree *next; /**< next error in the present list */
	struct ErrorReporterTree *prev; /**< prev error in the present list */
	struct ErrorReporterTree *parent; /**< parent error (or NULL) */
} error_reporter_tree_t;

ASC_DLLSPEC error_reporter_tree_t *error_reporter_tree_start(int iscaching);
ASC_DLLSPEC int error_reporter_tree_end(error_reporter_tree_t *tree);
ASC_DLLSPEC void error_reporter_tree_end_clear(error_reporter_tree_t *tree);
ASC_DLLSPEC int error_reporter_tree_has_error(error_reporter_tree_t *tree);
ASC_DLLSPEC ASC_DEPREC error_reporter_tree_t *error_reporter_get_tree_current();

/**
	This is the drop-in replacement for Asc_FPrintf. Anythin you attempt
	to print to stderr will be captured and passed to the error_reporter_callback
	function for handling.
*/
ASC_DLLSPEC int fprintf_error_reporter(FILE *file, const char *fmt, ...);

/**
	For use when implementing higher-level error handling routines
*/
ASC_DLLSPEC int vfprintf_error_reporter(FILE *file, const char *fmt, va_list *args);

/**
	If file!=stderr, this will do the usual thing. If file==stderr, it will output
	the character via fprintf_error_reporter.
*/
ASC_DLLSPEC int fputc_error_reporter(int c, FILE *file); /* just calls fprintf_error_reporter */

/**
	This replaces the standard 'fflush' of Asc_FFlush. If file!=stderr, it will
	call the standard fflush. If file==stderr, it will call error_reporter_end_flush.
*/
ASC_DLLSPEC int fflush_error_reporter(FILE *file);

/**
	Start a cached error report. This means that multiple frprintf_error_reporter calls will
	be stored in a global string until an error_reporter_end_flush is encountered.
*/
ASC_DLLSPEC int error_reporter_start(const error_severity_t sev, const char *filename, const int line, const char *func);

/**
	Output the contents of the checked global string as an error report
*/
ASC_DLLSPEC int error_reporter_end_flush();

/**
	This #define saves you typing the list of arguments in your
	callback function declarations.
*/
#define ERROR_REPORTER_CALLBACK_ARGS \
    const error_severity_t sev \
  , const char *filename \
  , const int line \
  , const char *funcname \
  , const char *fmt \
  , va_list *args

/*
	In you have functions which pass-through callback parameters,
	this #define ensures that if their ordering/naming changes,
	you won't have to go hunting and change stuff.
*/
#define ERROR_REPORTER_CALLBACK_VARS \
	sev, filename, line, funcname, fmt, args

/*
	Define the type of the function pointer to be used for all
	error reporting functions. The final argument is a va_list.
	You should use 'vsnprintf' of 'vfprintf' to output your
	message to the desired file or string, see <stdio.h> for these.
*/
typedef int (*error_reporter_callback_t)(
	ERROR_REPORTER_CALLBACK_ARGS
);

typedef int (*ErrorReporter_fptr_t)(
      const error_severity_t sev
    , const char *errfile
    , const int errline
    , const char *errfunc
    , const char *fmt
    , ...
);

/**
	Use this function directly for 'richer' reporting of
	of error messages.

	@return follows the style of fprintf
*/
ASC_DLLSPEC int error_reporter(
      const error_severity_t sev
    , const char *errfile
    , const int errline
    , const char *errfunc
    , const char *fmt
    , ...
);

/**
	This format of the error reporter is useful if you must call it
	from another variable-argument-list function.
*/
ASC_DLLSPEC int va_error_reporter(ERROR_REPORTER_CALLBACK_ARGS);

/**
	Set error reporting callback function using this
	function. If left unset, errors will be printed
	to standard error, which is effectively what the
	hitherto FPRINTF has done.
*/
ASC_DLLSPEC void error_reporter_set_callback(
		const error_reporter_callback_t new_callback
);

/* @} */

#endif /* ASC_ERROR_H */
