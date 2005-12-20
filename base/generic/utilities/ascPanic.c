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
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"

#define PANIC_MSG_MAXLEN 2047
/**< The maximum length of the panic message.	
	Used to create a buffer to hold the message.
*/

static PanicCallbackFunc f_panic_callback_func = NULL;
/**< Holds a pointer to a callback function if registered using
	Asc_PanicSetCallback().  If NULL (the default), nothing is called.
*/

#ifdef __WIN32__
static int f_display_MessageBox = TRUE;
/**< On Windows only, flag to enable/disable display of the MessageBox 
	in Asc_Panic(). 
*/
#endif

static char g_panic_outfile[PATH_MAX];
/**< Holds the name of the file in which to write panic messages.
	Use the Asc_PanicSetOutfile(filename) function to set it.
*/


void Asc_Panic(CONST int status, CONST char *function,
               CONST char *format, ...)
{
  char msg[PANIC_MSG_MAXLEN];  /* The message that will be printed */
  size_t p;                    /* The current length of the msg array */
  FILE *outfile;               /* The file to save the message into */
  va_list args;                /* The arguments to print */
  int cancel = FALSE;          /* If non-zero, do not call exit().  Default is to exit() */

  /* Fail loudly if ASCERR isn't set to a file pointer -- can't use asc_assert here! */
  assert(NULL != ASCERR);      

  /* Give the name of the function where the panic occurred */
  if( function != NULL ) {
    snprintf( msg, PANIC_MSG_MAXLEN-2, "function '%s':", function );
  }else{
	snprintf(msg, PANIC_MSG_MAXLEN-2, " ");
  }
  p = strlen(msg);

  /* Add the variable args to the panic message using the format "format" */
  va_start(args, format);
  vsnprintf( (msg+p), PANIC_MSG_MAXLEN-p-2, format, args );
  va_end(args);

  p = strlen(msg);
  msg[p++] = '\n';
  msg[p++] = '\0';

  /* 
	Ensure that our messages don't get left in the GUI
	that is about to vanish...
  */
  error_reporter_set_callback(NULL);

  /* Print the message to ASCERR */
  error_reporter(ASC_PROG_FATAL,NULL,0,msg);
	
  /*
	Write the message to g_panic_outfile if it is not empty
	and we can actually write to that location.  Print a
	message on ASCERR (if valid) saying that we did that.
  */
  if(( g_panic_outfile[0] != '\0' )
     && ( (outfile=fopen(g_panic_outfile,"w")) != NULL ))
  {
    FPRINTF(outfile, msg);
    if( ASCERR != NULL ) {
      CONSOLE_DEBUG("Error message written to %s\n", g_panic_outfile);
    }
    fclose(outfile);
  }

  /* Call the registered callback function, if any. */
  if (NULL != f_panic_callback_func) {
    cancel = (*f_panic_callback_func)(status);
  }

  /* Display msg in a MessageBox under Windows unless turned off */
#ifdef __WIN32__
  if (FALSE != f_display_MessageBox) {
    (void)MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(NULL, msg, "Fatal Error in ASCEND",
               (UINT)(MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND));
  }
  if (0 == cancel)
    ExitProcess((UINT)status);
#endif

  if (0 == cancel)
    exit(status);
}


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
#ifdef __WIN32__
  f_display_MessageBox = is_displayed;
#else
  UNUSED_PARAMETER(is_displayed);
#endif
}

/* removed superceded test 'main' */
