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

/* removed changelog -- please consult commit comments in ViewCVS */

#include <stdarg.h>
#include "ascConfig.h"
#include "ascPanic.h"

#define PANIC_MSG_MAXLEN 2047
/**< The maximum length of the panic message.
	Used to create a buffer to hold the message.
*/

static PanicCallbackFunc f_panic_callback_func = NULL;
/**< Holds a pointer to a callback function if registered using
	Asc_PanicSetCallback().  If NULL (the default), nothing is called.
*/

/*
	disable the windows message box...
#if defined(__WIN32__)
# define USE_WIN32_FATAL_MSGBOX
#endif
*/

#ifdef USE_WIN32_FATAL_MSGBOX
static int f_display_MessageBox = TRUE;
/**< On Windows only, flag to enable/disable display of the MessageBox
	in Asc_Panic().
*/
#endif

static char g_panic_outfile[PATH_MAX];
/**< Holds the name of the file in which to write panic messages.
	Use the Asc_PanicSetOutfile(filename) function to set it.
*/


/**
	This static function does the actual output of a PANIC message. It aims
	to do a few things to ensure that the user always gets the message.

	@TODO We could can improve even further by integrating with some crash
	reporting facility in GNOME, etc?
*/
static void asc_va_panic(const int status, const char *filename, const int line
		, const char *function, const char *fmt, const va_list args
){
	FILE *outfile;               /* The file to save the message into */
	int cancel = FALSE;          /* If non-zero, do not call exit().  Default is to exit() */
	char msg[PANIC_MSG_MAXLEN];
	size_t p;

	/* Fail loudly if ASCERR isn't set to a file pointer -- can't use asc_assert here! */
	assert(NULL != ASCERR);

	p = snprintf(msg,PANIC_MSG_MAXLEN-2,"%s:%d (%s): ",filename,line,function);

	/* Add the variable args to the panic message using the format "format" */
	vsnprintf(msg+p, PANIC_MSG_MAXLEN-p-2, fmt, args );

	p = strlen(msg);
	msg[p++] = '\n';
	msg[p++] = '\0';

	/*
		Always write the message to g_panic_outfile if it is not empty
		and we can actually write to that location.  Print a
		message on ASCERR (if valid) saying that we did that.
	*/
	if(( *g_panic_outfile != '\0' )
			&& ( (outfile=fopen(g_panic_outfile,"w")) != NULL )
	){
		fprintf(outfile, msg);
		CONSOLE_DEBUG("Error message written to %s\n", g_panic_outfile);

		fclose(outfile);
	}

	if (NULL == f_panic_callback_func) {
		/* No panic-callback, so we reset the error handler and output to console */
		error_reporter_set_callback(NULL);

		/* Print the message to the default error reporter (ASCERR) */
		fprintf(stderr,"\n\n");
		va_error_reporter(ASC_PROG_FATAL,filename,line,function,fmt,args);
		fprintf(stderr,"\n");

	}else{
    	/* just use the callback, don't make any output */
    	cancel = (*f_panic_callback_func)(status);
		if(cancel){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,
				"AscPanic 'cancel' facility has been disabled, program will exit"
			);
		}
	}

  /* Display msg in a MessageBox under Windows unless turned off */
#ifdef USE_WIN32_FATAL_MSGBOX
	if(FALSE != f_display_MessageBox) {
		(void)MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(NULL, msg, "Fatal Error in ASCEND"
			,(UINT)(MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND)
		);
	}
#endif
}

/**
	This is a new replacement for Asc_Panic that handles error source
	details (line, file, function) itself.
*/
void asc_panic_line(const int status, const char *filename, const int line
		, const char *function, const char *fmt, ...
){
	va_list args;

	va_start(args,fmt);
	asc_va_panic(status,filename,line,function,fmt,args);
	va_end(args);

#ifdef USE_WIN32_FATAL_MSGBOX
	ExitProcess((UINT)status);
#else
# ifndef NDEBUG
	abort();
# else
	exit(status);
# endif
#endif
}

#if !defined(__GNUC__) || defined(__STRICT_ANSI__)
/**
	we only need this function if our platform doesn't support var-arg macros 
*/
void asc_panic(CONST int status, CONST char *function
		,CONST char *fmt, ...
){
	va_list args;

	va_start(args,fmt);
	asc_va_panic(status,NULL,0,function,fmt,args);
	va_end(args);
# ifdef USE_WIN32_FATAL_MSGBOX
	ExitProcess((UINT)status);
# else
#  ifndef NDEBUG
	abort();
#  else
	exit(status);
#  endif
# endif
}

/** this one is also only required if we don't support var-arg macros */
void asc_panic_nofunc(const char *fmt, ...){
	va_list args;
	va_start(args,fmt);
	asc_va_panic(2,NULL,0,NULL,fmt,args);
	va_end(args);
# ifdef USE_WIN32_FATAL_MSGBOX
	ExitProcess((UINT)status);
# else
#  ifndef NDEBUG
	abort();
#  else
	exit(status);
#  endif
# endif
}

#endif /* !__GNUC__ || __STRICT_ANSI__ */

void Asc_PanicSetOutfile(CONST char *filename)
{
  if( filename != NULL ) {
    strncpy( g_panic_outfile, filename, PATH_MAX-1 );
    g_panic_outfile[PATH_MAX-1] = '\0';
  } else {
    g_panic_outfile[0] = '\0';
  }
}

PanicCallbackFunc Asc_PanicSetCallback(PanicCallbackFunc func)
{
  PanicCallbackFunc old_func = f_panic_callback_func;
  f_panic_callback_func = func;
  return old_func;
}


void Asc_PanicDisplayMessageBox(int is_displayed)
{
#ifdef USE_WIN32_FATAL_MSGBOX
  f_display_MessageBox = is_displayed;
#else
  UNUSED_PARAMETER(is_displayed);
#endif
}
