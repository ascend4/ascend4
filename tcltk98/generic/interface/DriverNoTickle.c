/*
 *  DriverNoGUI.c
 *  by Ben Allan
 *  Created: 7/2004
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: DriverNoTickle.c,v $
 *  Date last modified: $Date: 2004/07/13 07:42:30 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#define WITH_TK 0
#define CONST84 const

#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <utilities/ascConfig.h>
#ifndef __WIN32__
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <locale.h>
#undef WIN32_LEAN_AND_MEAN
#endif /* __WIN32__ */
#include <utilities/ascMalloc.h> /* for ascshutdown */
#include <utilities/ascPanic.h>  /* for Asc_Panic */
#include <utilities/ascEnvVar.h>
#include <compiler/compiler.h>
#include <compiler/ascCompiler.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/compiler.h> /* for symchar for units.h */
#include <compiler/units.h>
#include <solver/slv_types.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/logrel.h>
#include <solver/discrete.h>
#include <solver/mtx.h>
#include <solver/slv_stdcalls.h>

#ifndef lint
static CONST char DriverID[] = "$Id: DriverNoTickle.c,v 1.1 2004/07/13 07:42:30 ballan Exp $";
#endif


/*
 *  The following are the environment variables ASCEND requires.
 *  If the user does not have the DIST_ENVIRONVAR set in his or her
 *  environment, a default value is set based on the directory where the
 *  ascend binary lives.  The other enviornment variables will be set
 *  to default values keyed off of DIST_ENVIRONVAR.  See the function
 *  CheckEnvironmentVars later in this file for the details.
 */
#define DIST_ENVIRONVAR   "ASCENDDIST"
#define LIBR_ENVIRONVAR   "ASCENDLIBRARY"

/*
 *  EXPORTED VARIABLES
 */

/**
 *  g_compiler_timing
 *
 *  TRUE if compiler timing is to be printed.
 *  default is false, set to TRUE by passing -t on the command line
 */
int g_compiler_timing = 0;


/*
 *  zz_debug
 *
 *  Comes from the yacc file if yacc was built with debugging information
 */
#ifdef ZZ_DEBUG
extern int zz_debug;
#endif


/*
 *  Forward declarations for procedures defined later in this file.
 */
static int  AscDriver(int, CONST84 char * argv[]);
static void AscTrap(int);
static void AscPrintHelpExit(CONST char *);
static int AscProcessCommandLine( int argc, CONST84 char **argv);
#ifdef DEBUG_MALLOC
static void InitDebugMalloc(void);
#endif /* DEBUG_MALLOC */
#ifdef __WIN32__
static void setargv(int*, char ***);
#endif /* __WIN32__ */


/*
 *  LOCALLY GLOBAL VARIABLES
 */

/*
 *  g_interface_simplify_relations
 *
 *  TRUE for compiler optimizations
 *  default is TRUE, set to FALSE by passing +s on the command line
 */
static int g_interface_simplify_relations = TRUE;

/*
 *  g_interfacever
 *
 *  TRUE if windows to be built; default is TRUE, false is not supported
 */
static int g_interfacever = 0;

/*
 *  tty
 *
 *  Non-zero means standard input is a terminal-like device.
 *  Zero means it's a file.
 */
static int tty;

/*
 * This assumes tcl_library has been found and that tcl8.0 and tk8.0
 * are installed in the same lib directory -- the default tcl/tk install.
 */

/*
 *  build_name
 *
 *  who built this binary and when
 */
#ifndef TIMESTAMP
static char build_name[]="by anonymous";
#else
static char build_name[]=TIMESTAMP;
#endif /* TIMESTAMP */



/*
 *  main or WinMain
 *
 *  The main entry point for a Unix or Windows application.
 *
 *  Each just calls AscDriver().
 *  These are based on functions from the Tk 8.0 distribution.
 *  See unix/tkAppInit.c and win/winMain.c in their sources.
 */
#ifndef __WIN32__

int main(int argc, CONST84 char *argv[])
{
  AscDriver(argc, argv);
  return 0;
}

#else /* __WIN32__ */
/* this may need fixing undfer windows, if winmain requires tk */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdLine, int nCmdShow)
{
  int argc;
  char **argv;
  char *p;
  char buffer[MAX_PATH];

  /*
   * Set up the default locale to be standard "C" locale so parsing
   * is performed correctly.
   */
  setlocale(LC_ALL, "C");

  /*
   * Increase the application queue size from default value of 8.
   * At the default value, cross application SendMessage of WM_KILLFOCUS
   * will fail because the handler will not be able to do a PostMessage!
   * This is only needed for Windows 3.x, since NT dynamically expands
   * the queue.
   */
  SetMessageQueue(64);

  /*
   * Create the console channels and install them as the standard
   * channels.  All I/O will be discarded until TkConsoleInit is
   * called to attach the console to a text widget.
   */

  /*
   *  Windows expects us to parse our arguments ourselves.
   */
  setargv(&argc, &argv);

  /*
   * Replace argv[0] with full pathname of executable, and forward
   * slashes substituted for backslashes.
   */
  GetModuleFileName(NULL, buffer, sizeof(buffer));
  argv[0] = buffer;
  for (p = buffer; *p != '\0'; p++) {
    if (*p == '\\') {
      *p = '/';
    }
  }

  AscDriver(argc, argv);
  return 1;
}
#endif /* __WIN32__ */


/*
 *  int AscDriver( argc, argv )
 *      int argc;
 *      char *argv;
 *
 *  A common entry point for Windows and Unix.  The corresponding
 *  WinMain() and main() functions just call this function.
 *
 *  This function creates a Tcl interpreter, initializes Tcl and Tk,
 *  initializes the Ascend data structures, sets up the user's
 *  environment, sources ASCEND's startup script, and calls Tk_MainLoop
 *  so the user can interact with ASCEND.  Cleans up and exits the
 *  program when Tk_MainLoop returns.
 *
 *  This function is based on the functions Tk_Main and Tcl_AppInit
 *  from the Tk8.0 distribution.  See the files tkMain.c and tkAppInit.c
 *  in the Tk sources.
 *
 */
static int AscDriver(int argc, CONST84 char *argv[])
{

  /*
   *  Set the "tcl_interactive" variable.
   *
   *  ASCEND sets tty to `1', since we assume ASCEND is always interactive.
   */
  tty = 1;
#ifdef USE_ASC_PRINTF
  Asc_PrintInit();
#endif /* USE_ASC_PRINTF */

  /*
   *  Now that our console and printing functions are properly
   *  initialized, print our startup banner.
   */
  PRINTF("ASCEND VERSION IV\n");
  PRINTF("Compiler Implemention Version: 2.0\n");
  PRINTF("Written by Tom Epperly,Kirk Abbott, and Ben Allan\n");
  PRINTF("Copyright(C) 1990, 1993, 1994 Thomas Guthrie Epperly\n");
  PRINTF("  Built: %s %s %s\n",__DATE__,__TIME__,build_name);
  PRINTF("Copyright(C) 1993-1996 Kirk Andre Abbott, Ben Allan\n");
  PRINTF("Copyright(C) 1997 Carnegie Mellon University\n");


  /*
   *  Initialize ASCEND C Structures
   *  Create ASCEND Tcl Commands
   */
  clock();
  /* the next line should NOT be Asc_SignalHandlerPush */
  (void)signal(SIGINT, AscTrap);
#ifdef DEBUG_MALLOC
  InitDebugMalloc();
  ascstatus("Memory status after calling InitDebugMalloc()");
#endif /* DEBUG_MALLOC */
  if ( Asc_CompilerInit(g_interface_simplify_relations) != 0 ) {
    Asc_Panic(2, "Asc_CompilerInit",
              "Insufficient memory to initialize compiler.");
  }
  SlvRegisterStandardClients();
  /*
   *  Loop infinitely, waiting for commands to execute.  When there
   *  are no windows left, Tk_MainLoop returns and we exit.
   */
#ifdef DEBUG_MALLOC
  ascstatus("Memory status before calling Tk_MainLoop()");
#endif /* DEBUG_MALLOC */
#ifdef DEBUG_MALLOC
  ascstatus("Memory status after Tk_MainLoop() exits");
#endif /* DEBUG_MALLOC */

  AscProcessCommandLine( argc, argv);
  /* * App goes here.  */


  /* app done now. */

  /*
   *  Do ASCEND Cleanup
   */
  Asc_HelpDestroy();
  Asc_UnitValue(NULL);
  Asc_SolvMemoryCleanup();
  Asc_CompilerDestroy();
  Asc_DestroyEnvironment();
#ifdef DEBUG_MALLOC
  ascshutdown("Memory status just before exiting");
#endif /* DEBUG_MALLOC */

  return 0;
}


static int AscProcessCommandLine( int argc, CONST84 char **argv)
{
  int i;
  int flag;                         /* set to 1 for `+arg', -1 for `-arg' */
  size_t length;                    /* length of an argv */
  char *args;
  char buf[MAXIMUM_NUMERIC_LENGTH]; /* space for integer->string conversion */
  int new_argc = 0;                 /* the argc we will pass to Tcl */
#ifdef ZZ_DEBUG
  zz_debug = 0;                     /* nonzero to print parser debugging info*/
#endif

  for( i = 1; i < argc; i++ ) {
    if( (length = strlen(argv[i])) == 0 ) {
      /* ignore 0-length arguments */
      continue;
    }

    if(( length >= 2 ) && ( strncmp(argv[i],"-h",2) == 0 )) {
      AscPrintHelpExit(argv[0]);
    }
    if(( length >= 2 ) && ( strncmp(argv[i],"-H",2) == 0 )) {
      AscPrintHelpExit(argv[0]);
    }
    if(( length >= 4 ) && ( strncmp(argv[i],"help",4) == 0 )) {
      AscPrintHelpExit(argv[0]);
    }

    if( argv[i][0] == '-' ) {
      flag = -1;
    } else if( argv[i][0] == '+' ) {
      flag = 1;
    } else {
      flag = 0;
    }

    if(( length == 2 ) && ( flag != 0 )) {
      switch( argv[i][1] ) {
      case 'd':
        /*  '-d' turns on scanner debugging (if ascend was built with it)
         *  '+d' turns off scanner debugging [default]
         */
        if( flag == -1 ) {
#ifdef ZZ_DEBUG
          zz_debug = 1;
        } else {
          zz_debug = 0;
#else
          FPRINTF(ASCERR, "Sorry, %s wasn't compiled with %s defined.\n",
                  argv[0], "ZZ_DEBUG");
#endif /* ZZ_DEBUG */
        }
        break;
      case 's':
        /*  '-s' turns on compiler optimizations [default]
         *  '+s' turns off compiler optimizations
         */
        if( flag == -1 ) {
          g_interface_simplify_relations = 1;
        } else {
          g_interface_simplify_relations = 0;
        }
        break;
      case 't':
        /*  '-t' turns on timing of compiler optimizations
         *  '+t' turns off timing of compiler optimizations [default]
         */
        if( flag == 0 ) {
          g_compiler_timing = 1;
        } else {
          g_compiler_timing = 0;
        }
        break;
      case 'c':
      case 'g':
        fprintf(ASCERR, "WARNING! Obsolete ASCEND option \"%s\"\n", argv[i]);
        break;
      default:
        /*  unknown ASCEND option, pass it on to Tcl
         */
        argv[++new_argc] = argv[i];
        break;
      }
    } else {
      /*  unknown ASCEND option, pass it on to Tcl
       */
      argv[++new_argc] = argv[i];
    }
  }

  return 0;
}

/*
 *  AscPrintHelpExit(invoke_name)
 *      CONST char *invoke_name;
 *
 *  Print a help message and exit.  Use invoke_name as the name of
 *  the binary
 */
static
void AscPrintHelpExit(CONST char *invoke_name)
{
  PRINTF("usage: %s [options]\n"
         "\n"
         "where options include [default value]:\n"
         "    -h      print this message\n"
         "    -/+d    turn on/off yacc debugging  [off]\n"
         "    -/+s    turn on/off compiler optimizations [on]\n"
         "    -/+t    turn on/off timing of compiler operations  [off]\n",
         invoke_name);
  exit(0);
}


/*
 *  AscTrap(sig)
 *      int sig;
 *
 *  Function to call when we receive an interrupt.
 */
static
void AscTrap(int sig)
{
  putchar('\n');
  Asc_Panic(sig, "AscTrap", "Caught Signal: %d", sig);
}



/*
 *----------------------------------------------------------------------
 *  Tom Epperly's Malloc Debugger
 *----------------------------------------------------------------------
 */
#ifdef DEBUG_MALLOC
static void InitDebugMalloc(void)
{
  union dbmalloptarg m;
  m.str = NULL;
  m.i = 0;
  dbmallopt(MALLOC_CKDATA,&m);
}

#endif /* DEBUG_MALLOC */

#ifdef __WIN32__
/*
 *-------------------------------------------------------------------------
 *
 * setargv --
 *
 *	Parse the Windows command line string into argc/argv.  Done here
 *	because we don't trust the builtin argument parser in crt0.
 *	Windows applications are responsible for breaking their command
 *	line into arguments.
 *
 *	2N backslashes + quote -> N backslashes + begin quoted string
 *	2N + 1 backslashes + quote -> literal
 *	N backslashes + non-quote -> literal
 *	quote + quote in a quoted string -> single quote
 *	quote + quote not in quoted string -> empty string
 *	quote -> begin quoted string
 *
 * Results:
 *	Fills argcPtr with the number of arguments and argvPtr with the
 *	array of arguments.
 *
 * Side effects:
 *	Memory allocated.
 *
 * This function is from the Tk 8.0 distribution.  See win/winMain.c in
 * their sources.
 *
 *--------------------------------------------------------------------------
 */
static void
setargv(argcPtr, argvPtr)
    int *argcPtr;		/* Filled with number of argument strings. */
    char ***argvPtr;		/* Filled with argument strings (malloc'd). */
{
    char *cmdLine, *p, *arg, *argSpace;
    char **argv;
    int argc, size, inquote, copy, slashes;

    cmdLine = GetCommandLine();

    /*
     * Precompute an overly pessimistic guess at the number of arguments
     * in the command line by counting non-space spans.
     */

    size = 2;
    for (p = cmdLine; *p != '\0'; p++) {
	if (isspace(*p)) {
	    size++;
	    while (isspace(*p)) {
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }
    argSpace = (char *) ckalloc((unsigned) (size * sizeof(char *)
	    + strlen(cmdLine) + 1));
    argv = (char **) argSpace;
    argSpace += size * sizeof(char *);
    size--;

    p = cmdLine;
    for (argc = 0; argc < size; argc++) {
	argv[argc] = arg = argSpace;
	while (isspace(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    break;
	}

	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if ((inquote) && (p[1] == '"')) {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
                }
                slashes >>= 1;
            }

            while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }

	    if ((*p == '\0') || (!inquote && isspace(*p))) {
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
        }
	*arg = '\0';
	argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}

#endif /* __WIN32__ */
