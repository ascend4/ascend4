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
#include <utilities/ascConfig.h>
#include <utilities/ascPrint.h>

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

#ifdef USE_XTERM_COLOR_CODES
/** XTERM colour codes used to distinguish between errors of different types.

	@TODO some runtime testing to determine if these should be used or not
	depending on TERM env var.
*/
#  define ERR_RED "\033[31;1m"
#  define ERR_GRN "\033[32;2m"
#  define ERR_BLU "\033[34;1m"
#  define ERR_BRN "\033[33;1m"
#  define ERR_NORM "\033[0m"
#  define ERR_BOLD "\033[1m"
#else
#  define ERR_RED ""
#  define ERR_GRN ""
#  define ERR_BLU ""
#  define ERR_BRN ""
#  define ERR_NORM ""
#  define ERR_BOLD ""
#endif
/**
	Variadic macros to allow nice succint logging and error reporting
	calls from C dialects that support them (GCC, C99 and others)

	If you don't support variadic macros, you will still get the messages
	but without the file/function/line number.
*/
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define ERROR_REPORTER_DEBUG(args...) error_reporter(ASC_PROG_NOTE, __FILE__, __LINE__, __func__, ##args)
# define ERROR_REPORTER_HERE(SEV,args...) error_reporter(SEV,__FILE__, __LINE__, __func__, ##args)
# define ERROR_REPORTER_NOLINE(SEV,args...) error_reporter(SEV, NULL, 0, NULL, ##args)
# define CONSOLE_DEBUG(args...) (fprintf(stderr, ERR_BOLD "%s:%d (%s): ", __FILE__,__LINE__,__func__) + \
                                 fprintf(stderr, ##args) + \
                                 fprintf(stderr, ERR_NORM "\n"))

#elif defined(HAVE_C99)
# define ERROR_REPORTER_DEBUG(...) error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,__func__,## __VA_ARGS__)
# define ERROR_REPORTER_HERE(SEV,...) error_reporter(SEV,__FILE__,__LINE__,__func__, ## __VA_ARGS__)
# define ERROR_REPORTER_NOLINE(SEV,...) error_reporter(SEV,NULL,0,NULL, ## __VA_ARGS__)
# define CONSOLE_DEBUG(...) (fprintf(stderr, ERR_BOLD "%s:%d (%s): ", __FILE__,__LINE__,__func__) + \
                             fprintf(stderr, ##__VA_ARGS__) + \
                             fprintf(stderr, ERR_NORM "\n"))

#else
# define ERROR_REPORTER_DEBUG error_reporter_note_no_line
# define ERROR_REPORTER_HERE error_reporter_here
# define ERROR_REPORTER_NOLINE error_reporter_noline
# define CONSOLE_DEBUG console_debug
int error_reporter_note_no_line(const char *fmt,...);
int error_reporter_here(const error_severity_t sev, const char *fmt,...);
int error_reporter_noline(const error_severity_t sev, const char *fmt,...);
int console_debug(const char *fmt,...);
#endif

#define ERROR_REPORTER_START_NOLINE(SEV) error_reporter_start(SEV,NULL,0,NULL);
#define ERROR_REPORTER_START_HERE(SEV) error_reporter_start(SEV,__FILE__,__LINE__,__func__);

#define ERROR_REPORTER_STAT(sev,stat,msg) \
	error_reporter(sev,Asc_ModuleFileName(stat->mod),stat->linenum,NULL,msg)

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
	const char *func;
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
int error_reporter_start(const error_severity_t sev, const char *filename, const int line, const char *func);

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
  , const char *filename \
  , const int line \
  , const char *funcname \
  , const char *fmt \
  , const va_list args

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

#ifdef ASC_USE_IMPORTED_ERROR_REPORTER
/*
	If we're using a 'imported error reporter' then this means that we're in
	a different DLL to the main ASCEND code. In that case, we will have a
	pointer to our error reporter function, which will be back in the main
	DLL in fact. We need this header file to refer to that global-variable 
	function pointer instead of assuming that the function is here locally.
*/
static ErrorReporter_fptr_t g_ErrorReporter_fptr;
# define error_reporter (*g_ErrorReporter_fptr)
#else

/**
	Use this function directly for 'richer' reporting of
	of error messages.

	@return follows the style of fprintf
*/
DLEXPORT int error_reporter(
      const error_severity_t sev
    , const char *errfile
    , const int errline
    , const char *errfunc
    , const char *fmt
    , ...
);

#endif

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
