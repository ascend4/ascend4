#ifndef ASC_ERROR_H
#define ASC_ERROR_H
/**
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
	ascConfig defines ASC_FPRINTF etc, the routines to provide
	default 'real' printf behaviour on this platform. (As
	opposed to the sneaky stuff that FPRINTF does in this header)
*/
#include "utilities/ascConfig.h"

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

/**
	This nice macro on GNU C allows quick-and-dirty debug error 
	messages using the ERROR_REPORTER_DEBUG macro. On non GNU C
	systems, you will still get the error messages, but the location
	of the error won't be reported, because you don't support
	variadic macros.
*/
#ifdef HAVE_C99
# define ERROR_REPORTER_DEBUG(MSG,...) error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"%s: " MSG, __func__, ## __VA_ARGS__)
# define CONSOLE_DEBUG(MSG,...) fprintf(stderr,"%s:%d (%s): " MSG "\n", __FILE__,__LINE__,__func__, ## __VA_ARGS__)
#else
# define ERROR_REPORTER_DEBUG error_reporter_note_no_line
# define CONSOLE_DEBUG console_debug
int error_reporter_note_no_line(const char *fmt,...);
int console_debug(const char *fmt,...);
#endif

/**
	Error severity codes. This will be used to visually 
	the seriousness of errors. ASC_PROG_ERRORs for example
	might be red, or be highlighted with a (!) icon, etc.
*/
typedef enum error_severity_enum{
    ASC_USER_SUCCESS=0
   ,ASC_USER_NOTE=1    /**< a note to the user */
   ,ASC_USER_WARNING   /**< the user has done something bad but tolerable */
   ,ASC_USER_ERROR     /**< the user has done something wrong */
   ,ASC_PROG_NOTE      /**< a note for the programmer */
   ,ASC_PROG_WARNING   /**< the program encounters an unexpected state */
   ,ASC_PROG_ERROR     /**< the program has failed but can ignore and continue (maybe) */
   ,ASC_PROG_FATAL	   /**< fatal error, program will exit */
} error_severity_t;

/** An alias for ASC_PROG_ERROR */
#define ASC_PROG_ERR ASC_PROG_ERROR

#define ERROR_REPORTER_MAX_MSG 4096 /* no particular reason */

typedef struct{
	unsigned char iscaching; /** set to true for fprintf_error_reporter to do its work */
	error_severity_t sev;
	const char *filename;
	int line;
	char msg[ERROR_REPORTER_MAX_MSG];
} error_reporter_meta_t;

/**
	This is the drop-in replacement for Asc_FPrintf. Anythin you attempt
	to print to stderr will be captured and passed to the error_reporter_callback
	function for handling.
*/
int fprintf_error_reporter(FILE *file, const char *fmt, ...);

/**
	If file!=stderr, this will do the usual thing. If file==stderr, it will output
	the character via fprintf_error_reporter.
*/
int fputc_error_reporter(int c, FILE *file); /* just calls fprintf_error_reporter */

/**
	This replaces the standard 'fflush' of Asc_FFlush. If file!=stderr, it will
	call the standard fflush. If file==stderr, it will call error_reporter_end_flush.
*/
int fflush_error_reporter(FILE *file);

/**
	Start a cached error report. This means that multiple frprintf_error_reporter calls will
	be stored in a global string until an error_reporter_end_flush is encountered.
*/
int error_reporter_start(const error_severity_t sev, const char *filename, const int line);

/**
	Output the contents of the checked global string as an error report
*/
int error_reporter_end_flush();

/**
	This #define saves you typing the list of arguments in your
	callback function declarations.
*/
#define ERROR_REPORTER_CALLBACK_ARGS \
   const error_severity_t sev \
	, const char *filename, const int line \
	, const char *fmt, const va_list args

/*
	In you have functions which pass-through callback parameters,
	this #define ensures that if their ordering/naming changes,
	you won't have to go hunting and change stuff.
*/
#define ERROR_REPORTER_CALLBACK_VARS \
	sev, filename, line, fmt, args

/*
	Define the type of the function pointer to be used for all 
	error reporting functions. The final argument is a va_list.
	You should use 'vsnprintf' of 'vfprintf' to output your
	message to the desired file or string, see <stdio.h> for these.
*/
typedef int (*error_reporter_callback_t)(
	ERROR_REPORTER_CALLBACK_ARGS
);

/**
	Use this function directly for 'richer' reporting of 
	of error messages.

	@return follows the style of fprintf
*/
int error_reporter(
		const error_severity_t sev
		, const char *errfile, const int errline
		, const char *fmt, ...
);

/**
	This format of the error reporter is useful if you must call it
	from another variable-argument-list function.
*/
int va_error_reporter(ERROR_REPORTER_CALLBACK_ARGS);

/**
	Set error reporting callback function using this
	function. If left unset, errors will be printed
	to standard error, which is effectively what the
	hitherto FPRINTF has done.
*/
void error_reporter_set_callback(
		const error_reporter_callback_t new_callback
);

#endif /* ASC_ERROR_H */
