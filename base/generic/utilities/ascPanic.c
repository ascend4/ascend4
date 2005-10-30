/*
 *  Ascend Panic
 *  by Mark Thomas
 *  Created: 1997.05.15
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: ascPanic.c,v $
 *  Date last modified: $Date: 1997/07/18 11:43:21 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
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
 */

/*  ChangeLog
 *
 *  10/13/2005  Added callback functionality & ability to cancel exit()
 *              for use in unit test.  Changed sprintf's to snprintf's
 *              to avoid buffer overflow (J.D. St.Clair)
 */

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"

/*  PANIC_MSG_MAXLEN
 *    The maximum length of the panic message.  Used to create a buffer
 *    to hold the message.
 */
#define PANIC_MSG_MAXLEN 2047


/*  THIS IS SUPERCEDED BY EXTERNAL UNIT TEST FOR ascPanic.c */
/*  PANIC_TEST
 *    Define this cpp macro to build a standalone program to test the
 *    panic functions---since they shouldn't be called during normal
 *    operation:  cc -DPANIC_TEST -I.. ascpanic.c -o panictest
 *
 *    TEST_OUTPUT_GOOD is a writable file to write a panic message to
 *    TEST_OUTPUT_BAD is an unwritable file to write a panic message to
 */
#ifdef PANIC_TEST
FILE *ASCERR = stderr;
#define exit(x) FPRINTF(ASCERR, "<<<call exit(%d) if !test>>>\n", (x)); return
#define TEST_OUTPUT_GOOD "/tmp/Asc_Panic.out"
#define TEST_OUTPUT_BAD  "/foo/bar/baz/cow/grumble/Asc_Panic.out"
#endif  /*  PANIC_TEST  */

/*
 *  f_panic_callback_func
 *     Holds a pointer to a callback function if registered using
 *     Asc_PanicSetCallback().  If NULL (the default), nothing is called.
 */
static PanicCallbackFunc f_panic_callback_func = NULL;

#ifdef __WIN32__
/*
 *  On Windows only, flag to enable/disable display of the MessageBox 
 *  in Asc_Panic(). 
 */
static int f_display_MessageBox = TRUE;
#endif

/*
 *  g_panic_output
 *    Holds the name of the file in which to write panic messages.
 *    Use the Asc_PanicSetOutfile(filename) function to set it.
 */
static char g_panic_outfile[PATH_MAX];


void Asc_Panic(CONST int status, CONST char *function,
               CONST char *format, ...)
{
  char msg[PANIC_MSG_MAXLEN];  /* The message that will be printed */
  size_t p;                    /* The current length of the msg array */
  FILE *outfile;               /* The file to save the message into */
  va_list args;                /* The arguments to print */
  int cancel = FALSE;          /* If non-zero, do not call exit().  Default is to exit() */

  assert(NULL != ASCERR);      /* fail loudly so know can't write msg.  Can't use asc_assert(). */
  /*
   *  Give the name of the function where the panic occurred
   */
  if( function != NULL ) {
    snprintf( msg, PANIC_MSG_MAXLEN-2, "ASCEND PANIC!!  in function \"%s\"\n", function );
  } else {
    snprintf( msg, PANIC_MSG_MAXLEN-2, "ASCEND PANIC!!\n" );
  }
  p = strlen(msg);

  /*
   *  Add the variable args to the panic message using the format "format"
   */
  va_start(args, format);
  vsnprintf( (msg+p), PANIC_MSG_MAXLEN-p-2, format, args );
  va_end(args);

  p = strlen(msg);
  msg[p++] = '\n';
  msg[p++] = '\0';

  /*
   *  Print the message to ASCERR
   */
  if (ASCERR != NULL)
    FPRINTF(ASCERR, msg);

  /*
   *  Write the message to g_panic_outfile if it is not empty
   *  and we can actually write to that location.  Print a
   *  message on ASCERR (if valid) saying that we did that.
   */
  if(( g_panic_outfile[0] != '\0' )
     && ( (outfile=fopen(g_panic_outfile,"w")) != NULL ))
  {
    FPRINTF(outfile, msg);
    if( ASCERR != NULL ) {
      FPRINTF(ASCERR, "ASCEND PANIC: Error message written to %s\n", g_panic_outfile);
    }
    fclose(outfile);
  }

  /*
   *  Call the registered callback function, if any.
   */
  if (NULL != f_panic_callback_func) {
    cancel = (*f_panic_callback_func)(status);
  }

#ifdef __WIN32__
  /*
   *  Display msg in a MessageBox under Windows unless turned off
   */
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
    strncpy( g_panic_outfile, filename, PANIC_MSG_MAXLEN-1 );
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


void Asc_PanicDisplayMessageBox(int TRUE_or_FALSE)
{
#ifdef __WIN32__
  f_display_MessageBox = TRUE_or_FALSE;
#else
  UNUSED_PARAMETER(TRUE_or_FALSE);
#endif
}

/*  THIS IS SUPERCEDED BY EXTERNAL UNIT TEST FOR ascPanic.c */
#ifdef PANIC_TEST
/*
 *  main
 *    A test driver for Asc_Panic() and Asc_PanicSetOutfile().  Needed
 *    since we should never actually call Asc_Panic under normal
 *    operation.  (HA HA!)
 */
int main(void)
{
  PRINTF("==>Testing Asc_Panic with no output file and single arg\n");
  Asc_Panic(1, NULL, "Message generate by Asc_Panic, single string arg\n");
  PRINTF("<==\n");

  PRINTF("==>Testing Asc_Panic with no output file and multiple args\n");
  Asc_Panic(2, "main",
            "%s%s\n\t%d%s%d%s\n", "Message generated by ", "Asc_Panic,",
            5, " string args and ", 3, " integer args");
  PRINTF("<==\n");

  PRINTF("==>Testing Asc_Panic with mismatched format/args: too many args\n");
  Asc_Panic(3, "main",
            "%s%s%s\n", "Message generated by ", "Asc_Panic, ",
            "too many args", " for format");
  PRINTF("<==\n");

  PRINTF("==>Testing Asc_Panic with mismatched format/args: too few args\n");
  Asc_Panic(4, NULL,
            "%s%s%s%s\n", "Message generated by ", "Asc_Panic, ",
            "too few args");
  PRINTF("<==\n");

  PRINTF("==>Testing Asc_Panic with file %s\n", TEST_OUTPUT_GOOD);
  Asc_PanicSetOutfile(TEST_OUTPUT_GOOD);
  Asc_Panic(5, NULL,
            "%s%s\n\t%s%s\n", "Message generated by ", "Asc_Panic, ",
            "should be written into ", TEST_OUTPUT_GOOD);
  PRINTF("<==\n");

  PRINTF("==>Testing Asc_Panic with file %s\n", TEST_OUTPUT_BAD);
  Asc_PanicSetOutfile(TEST_OUTPUT_BAD);
  Asc_Panic(6, "main",
            "Message generated by Asc_Panic\n"
            "\tWrite to unwritable file %s", TEST_OUTPUT_BAD);
  PRINTF("<==\n");

  PRINTF(">>End of tests\n");
  return 0;
}
#endif  /* PANIC_TEST */
