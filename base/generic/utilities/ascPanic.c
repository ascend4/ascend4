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

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"

/* jds20050119:  windows.h is now included by ascConfig.h
#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
*/

/*  PANIC_MSG_MAXLEN
 *    The maximum length of the panic message.  Used to create a buffer
 *    to hold the message.
 */
#define PANIC_MSG_MAXLEN 2047


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
 *  g_panic_output
 *    Holds the name of the file in which to write panic messages.
 *    Use the Asc_PanicSetOutfile(filename) function to set it.
 */
static char g_panic_outfile[PATH_MAX];


/*
 * Asc_Panic( status, function, format, args )
 *      int status;
 *      CONST char *function
 *      CONST char *format
 *      VAR_ARGS args
 *
 *  This function prints the arguments "args" using the format string
 *  "format" to the ASCERR file handle.  The first line of the panic
 *  message will print ``ASCEND PANIC!! in function'' if the argument
 *  "function" is not NULL.  If ASCERR has not been initialized to a
 *  valid file pointer, the message will not be printed.  Either way,
 *  if an panic output file location has been specified with the 
 *  Asc_PanicSetOutfile() function, the panic message is also stored 
 *  there.  Under Windows, we also pop up a MessageBox containing the 
 *  message.  Finally, we exit the program with the status "status".
 *
 *  Side Effects: Exits the program.
 */
extern void Asc_Panic(CONST int status, CONST char *function,
                      CONST char *format, ...)
{
  char msg[PANIC_MSG_MAXLEN];  /* The message that will be printed */
  unsigned int p;              /* The current length of the msg array */
  FILE *outfile;               /* The file to save the message into */
  va_list args;                /* The arguments to print */
assert(NULL != ASCERR);
  /*
   *  Give the name of the function where the panic occurred
   */
  if( function != NULL ) {
    sprintf( msg, "ASCEND PANIC!!  in function \"%s\"\n", function );
  } else {
    sprintf( msg, "ASCEND PANIC!!\n" );
  }
  p = strlen(msg);

  /*
   *  Add the variable args to the panic message using the format "format"
   */
  va_start(args, format);
  vsprintf( (msg+p), format, args );
  va_end(args);

  p = strlen(msg);
  msg[p++] = '\n';
  msg[p++] = '\0';

  /*
   *  Print the message to ASCERR
   */
  /* TODO:  should ascPanic fail loudly if ASCERR is NULL? */
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
    if( (fclose(outfile)) == 0 && (ASCERR != NULL)) {
      sprintf((msg+p),
              "ASCEND PANIC: Error message stored in %s\n", g_panic_outfile);
      FPRINTF(ASCERR, "%s", (msg+p));
    }
  }

#ifdef __WIN32__
  /*
   *  Display the error in a message box under Windows
   */
  MessageBeep(MB_ICONEXCLAMATION);
  MessageBox(NULL, msg, "Fatal Error in ASCEND",
             MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
  ExitProcess(status);
#endif

  exit(status);
}


/*
 *  Asc_PanicSetOutfile(filename)
 *      CONST char *filename;
 *
 *  Calling this function with a non-NULL "filename" will cause
 *  Asc_Panic() to write panic messages to "filename" in addition to the
 *  ASCERR file handle.  Passing in a "filename" of NULL causes panic
 *  messages not to be written to disk---this undoes the effect of
 *  previous calls to Asc_PanicSetOutfile()
 */
extern void Asc_PanicSetOutfile(CONST char *filename)
{
  if( filename != NULL ) {
    strcpy( g_panic_outfile, filename );
  } else {
    g_panic_outfile[0] = '\0';
  }
}


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
