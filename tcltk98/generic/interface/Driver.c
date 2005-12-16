/*
 *  Driver.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.48 $
 *  Version control file: $RCSfile: Driver.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include "tcl.h"
#include "tk.h"
#include "utilities/ascConfig.h"
#ifndef __WIN32__
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
/* jds20041229 - windows.h now included in ascConfig.h.
#include <windows.h>
*/
#include <locale.h>
#include "interface/tkConsole.h"
#undef WIN32_LEAN_AND_MEAN
#endif /* __WIN32__ */
#include "utilities/ascMalloc.h"    /* for ascshutdown */
#include "utilities/ascPanic.h"     /* for Asc_Panic */
#include "utilities/ascEnvVar.h"
#include "compiler/compiler.h"
#include "compiler/ascCompiler.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/compiler.h"      /* for symchar for units.h */
#include "compiler/units.h"
#include "compiler/redirectFile.h"  /* for Asc_RedirectCompilerDefault() */
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/logrel.h"
#include "solver/discrete.h"
#include "solver/mtx.h"
#include "solver/slv_stdcalls.h"
#include "interface/AscBitmaps.h"
#include "utilities/ascPrint.h"
#include "interface/AscPrintTcl.h"
#include "interface/HelpProc.h"
#include "interface/Commands.h"
#include "interface/Driver.h"
#include "interface/ScriptProc.h"
#include "interface/SolverProc.h"
#include "interface/UnitsProc.h"

#ifndef lint
static CONST char DriverID[] = "$Id: Driver.c,v 1.48 2003/08/23 18:43:06 ballan Exp $";
#endif


/*
 *  The following are the environment variables ASCEND requires.
 *  If the user does not have the DIST_ENVIRONVAR set in his or her
 *  environment, a default value is set based on the directory where the
 *  ascend binary lives.  The other environment variables will be set
 *  to default values keyed off of DIST_ENVIRONVAR.  See the function
 *  CheckEnvironmentVars later in this file for the details.
 */
#define DIST_ENVIRONVAR   "ASCENDDIST"
#define ASCTK_ENVIRONVAR  "ASCENDTK"
#define BITMAP_ENVIRONVAR "ASCENDBITMAPS"
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
/* int g_compiler_timing = 0; */
/* moved to compiler/simlist.c */

/**
 *  g_interp
 *
 *  Interpreter for this application.  We need to make it global
 *  so that our signal/floating-porint traps can access it.
 */
Tcl_Interp *g_interp;

/*
 *  zz_debug
 *
 *  Comes from the yacc file if yacc was built with debugging information
 */
#ifdef ZZ_DEBUG
extern int zz_debug;
#endif


/*
 *  Declarations for procedures defined outside of this file.
 */
extern int  Tktable_Init(Tcl_Interp*);

/*
 *  Forward declarations for procedures defined later in this file.
 */
static int  AscDriver(int, CONST84 char * argv[]);
static void AscTrap(int);
static int  AscCheckEnvironVars(Tcl_Interp*);
static void AscPrintHelpExit(CONST char *);
static int  AscProcessCommandLine(Tcl_Interp*, int, CONST84 char **);
static void Prompt(Tcl_Interp*, int);
static int  AscSetStartupFile(Tcl_Interp*);
static void StdinProc(ClientData, int);
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
static int g_interfacever = 1;

/*
 *  g_command
 *
 *  Used to assemble lines of terminal input into Tcl commands.
 */
static Tcl_DString g_command;

/*
 *  g_line
 *
 *  Used to read the next line from the terminal input.
 */
static Tcl_DString g_line;

/*
 *  tty
 *
 *  Non-zero means standard input is a terminal-like device.
 *  Zero means it's a file.
 */
static int tty;

/*
 *  initScriptTclAdjust
 *  initScriptTkAdjust
 *
 *  These two variables hold Tcl scripts that will set the TCL_LIBRARY
 *  and TK_LIBRARY environment variables if they are not already
 *  set in the user's environment.
 *  TCL_LIBRARY is set to:  dirnameofexecutable/../../Tcl/lib/tcl8.0
 *  TK_LIBRARY is set to $tcl_library/../tk8.0
 */
static char initScriptTclAdjust[] =
"proc asc_tclInit {} {\n\
    global env\n\
    rename asc_tclInit {}\n\
    set errors {}\n\
    set dirs {}\n\
    if [info exists env(TCL_LIBRARY)] {\n\
	return\n\
    } else {\n\
        set parentDir [file dirname [info nameofexecutable]]\n\
	set env(TCL_LIBRARY) $parentDir/../../Tcl/lib/tcl8.0\n\
    }\n\
  }\n\
asc_tclInit";
/* for tcl,     FOO/ascend4/bin/ascend4.exe is the executable, then
 *                              /--  up to FOO/ascend4/bin
 * set parentDir [file dirname [info nameofexecutable]]\n\
 * Then if Tcl is next to ascend4, ../../Tcl/lib/tcl8.0 should work.
 */

static char initScriptTkAdjust[] =
"proc asc_tkInit {} {\n\
    global env\n\
    rename asc_tkInit {}\n\
    set errors {}\n\
    set dirs {}\n\
    if [info exists env(TK_LIBRARY)] {\n\
	return\n\
    } else {\n\
        set parentDir [file dirname [info library]]\n\
	set env(TK_LIBRARY) $parentDir/tk8.0\n\
    }\n\
  }\n\
asc_tkInit";
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

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdLine, int nCmdShow)
{
  int argc;
  char **argv;
  char *p;
  char buffer[MAX_PATH];

  UNUSED_PARAMETER(hInstance);
  UNUSED_PARAMETER(hPrevInstance);
  UNUSED_PARAMETER(lpszCmdLine);
  UNUSED_PARAMETER(nCmdShow);

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
  Tcl_Interp *interp;                   /* local version of global g_interp */
  Tcl_Channel inChannel;
  Tcl_Channel outChannel;         
 
  /* jds20050119:  Initialize ASCERR before any calls to ascPanic(). */
  /* TODO: revisit when interface is decoupled from base - this may change. */
  Asc_RedirectCompilerDefault();
#ifdef USE_ASC_PRINTF  
  Asc_PrintInit_TclVtable();
#endif  /* USE_ASC_PRINTF */
  /*
   *  Create the Tk Console
   *
   *  Create the console channels and install them as the standard
   *  channels.  All I/O will be discarded until TkConsoleInit() is
   *  called to attach the console to a text widget.
   */
#ifdef ASC_USE_TK_CONSOLE
  TkConsoleCreate();
#endif /* ASC_USE_TK_CONSOLE */

  /*
   *  Find the name of the current executable---available in the
   *  Tcl command `info nameofexecutable'
   */
  Tcl_FindExecutable(argv[0]);

  /*
   *  Create the interpreter
   */
  g_interp = Tcl_CreateInterp();
  interp = g_interp;
  if( interp == NULL ) {
    Asc_Panic(2, "Asc_Driver",
              "Call to Tcl_CreateInterp returned NULL interpreter");
  }

  /*
   *  Check the arguments on the command line
   */
  AscProcessCommandLine(interp, argc, argv);

  /*
   *  Set the "tcl_interactive" variable.
   *
   *  ASCEND sets tty to `1', since we assume ASCEND is always interactive.
   */
  tty = 1;
  Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
 
  /*
   *  Initialize Tcl and Tk
   */
  (void)Tcl_Eval(interp,initScriptTclAdjust);
  if (Tcl_Init(interp) == TCL_ERROR) {
    Asc_Panic(2, "Asc_Driver", "Tcl initialization failed:\n%s",
              Tcl_GetStringResult(interp));
  }
  (void)Tcl_Eval(interp,initScriptTkAdjust);
  if (Tk_Init(interp) == TCL_ERROR) {
    Asc_Panic(2, "Asc_Driver", "Tk initialization failed:\n%s",
              Tcl_GetStringResult(interp));
  }
  Tcl_StaticPackage( interp, "Tk", Tk_Init, Tk_SafeInit );

  /*
   *  Initialize the console and the ASCEND printf() substitutes.
   *  All output before this point is lost.
   */
#ifdef ASC_USE_TK_CONSOLE
  if (TkConsoleInit(interp) == TCL_ERROR) {
    Asc_Panic(2, "Asc_Driver",
              "Call to TkConsoleInit failed:\n%s",
              Tcl_GetStringResult(interp));
  }
#endif /* ASC_USE_TK_CONSOLE */
#ifdef USE_ASC_PRINTF
  Asc_PrintInit_Tcl();
#endif /* USE_ASC_PRINTF */

  /*
   *  Now that our console and printing functions are properly
   *  initialized, print our startup banner.
   */
  PRINTF("ASCEND VERSION IV\n");
  PRINTF("Compiler Implemention Version: 2.0\n");
  PRINTF("Written by Tom Epperly, Kirk Abbott, and Ben Allan\n");
  PRINTF("  Built: %s %s %s\n",__DATE__,__TIME__,build_name);
  PRINTF("Copyright(C) 1990, 1993, 1994 Thomas Guthrie Epperly\n");
  PRINTF("Copyright(C) 1993-1996 Kirk Andre Abbott, Ben Allan\n");
  PRINTF("Copyright(C) 1997 Carnegie Mellon University\n");

  /*
   *  Call the init procedures for included packages.
   */
#ifdef STATIC_TKTABLE
  if( Tktable_Init(interp) == TCL_ERROR ) {
    Asc_Panic(2, "Asc_Driver",
              "Call to Tktable_Init failed:\n%s",
              Tcl_GetStringResult(interp));
  }
#endif /* STATIC_TKTABLE */

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
  if( Asc_HelpInit() == TCL_ERROR ) {
    Asc_Panic(2, "Asc_HelpInit",
              "Insufficient memory to initialize help system.");
  }
  Asc_CreateCommands(interp);
  Asc_RegisterBitmaps(interp);

  /*
   *  Set the environment, and set find the
   *  location of ASCEND's startup file.
   */
  AscCheckEnvironVars(interp);
  if( AscSetStartupFile(interp) != TCL_OK ) {
    Asc_Panic(2, "Asc_Driver",
              "Cannot find ~/.ascendrc nor the default AscendRC\n%s",
              Tcl_GetStringResult(interp));
  }

  /*
   *  Evaluate the ~/.ascendrc or $ASCENDTK/AscendRC file
   */
  Tcl_SourceRCFile(interp);

  /*
   *  Establish a channel handlers for stdin and stdout
   */
  inChannel = Tcl_GetStdChannel(TCL_STDIN);
  if (inChannel) {
    Tcl_CreateChannelHandler(inChannel, TCL_READABLE, StdinProc,
                             (ClientData) inChannel);
  }
  if (tty) {
    Prompt(interp, 0);
  }
  outChannel = Tcl_GetStdChannel(TCL_STDOUT);
  if (outChannel) {
    Tcl_Flush(outChannel);
  }

  /*
   *  Initialize the Dynamic Strings used in StdinProc()
   */
  Tcl_DStringInit(&g_command);
  Tcl_DStringInit(&g_line);
  Tcl_ResetResult(interp);

  /*
   *  Loop infinitely, waiting for commands to execute.  When there
   *  are no windows left, Tk_MainLoop returns and we exit.
   */
#ifdef DEBUG_MALLOC
  ascstatus("Memory status before calling Tk_MainLoop()");
#endif /* DEBUG_MALLOC */
  if (Asc_ScriptConfigureInterrupt(1,interp)!=0) {
    Asc_Panic(2, "Asc_ScriptConfigureInterrupt",
              "Unable to configure script interrupt.");
  }
  Tk_MainLoop();
  Asc_ScriptConfigureInterrupt(0,interp);
#ifdef USE_ASC_PRINTF  
  Asc_PrintFinalize_Tcl();
#endif  /* USE_ASC_PRINTF */
#ifdef DEBUG_MALLOC
  ascstatus("Memory status after Tk_MainLoop() exits");
#endif /* DEBUG_MALLOC */

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

  /*
   *  Destroy the interpreter and exit
   */
  Tcl_DeleteInterp(interp);
  Tcl_Exit(0);
  return 0;
}


/*
 *-----------------------------------------------------------------
 *  int AscCheckEnvironVars(interp)
 *      Tcl_Interp *interp;
 *
 *  This function checks the following environment variables and sets them
 *  to the default value if they are not set.  This function will set
 *  DIST_ENVIRONVAR to the parent of the directory where the ASCEND binary
 *  lives if it is not set, e.g., if the ascend binary is
 *      /foo/bar/ascend4
 *  DIST_ENVIRONVAR is set to "/foo".
 *
 *  We check to see if ASCTK_ENVIRONVAR points to the proper place by
 *  looking for the ASCEND startup script "AscendRC".  If we find it, set
 *  the Tcl variable tcl_rcFileName to its location, otherwise, call
 *  Asc_Panic() to exit.
 *
 *  Returns a standard Tcl return code.
 *
 *    CPP_MACRO            ENVIRONMENT VAR    DEFAULT VALUE
 *    =================    ===============    =============
 *    DIST_ENVIRONVAR      ASCENDDIST         parent of binary's directory
 * AWW20041208:   ASCTK_ENVIRONVAR     ASCENDTK           ASCENDDIST/TK
 *    ASCTK_ENVIRONVAR     ASCENDTK           ASCENDDIST/TK
 *    BITMAP_ENVIRONVAR    ASCENDBITMAPS      ASCENDTK/bitmaps
 *    LIBR_ENVIRONVAR      ASCENDLIBRARY
 * AWW20041209: .:ASCENDDIST/models/libraries:ASCENDDIST/models/examples
 *       .:ASCENDDIST/../../models
 *
 */
static int AscCheckEnvironVars(Tcl_Interp *interp)
{
  char *tmpenv;               /* holds values returned by Asc_GetEnv() */
  Tcl_DString ascenddist;     /* holds the value of DIST_ENVIRONVAR */
  Tcl_DString buffer2;        /* used to incrementally build environment
                               * variable values
                               */
  Tcl_DString buffer1;        /* holds the environment variable value in the
                               * native format: result of passing buffer2
                               * into Tcl_TranslateFileName()
                               */
  Tcl_Channel c;              /* used to test if file exists */

  /* initialize */
  Tcl_DStringInit(&ascenddist);
  Tcl_DStringInit(&buffer1);
  Tcl_DStringInit(&buffer2);

  /*
   *  Get the value of the ASCENDDIST environment variable;
   *  if not set, set it to the parent of the directory containing
   *  the ascend binary.  For example, if the ascend binary is
   *  /foo/bar/bin/ascend4, set ASCENDDIST to /foo/bar
   *  If Tcl doesn't know where we are---the Tcl command
   *  `info nameofexecutable' returns ""---then ASCENDDIST is set
   *  to "."
   */
  if( Asc_ImportPathList(DIST_ENVIRONVAR) == 0 ) {
    if( (tmpenv =  Asc_GetEnv(DIST_ENVIRONVAR)) == NULL ) {
      /* shouldn't be NULL since we just imported it successfully */
      Asc_Panic(2, "CheckEnvironmentVars",
                "Asc_GetEnv(%s) returned NULL value.", DIST_ENVIRONVAR);
    }
    Tcl_DStringAppend(&ascenddist, tmpenv, -1);
    ascfree(tmpenv);
  } else {
    char cmd[] =
      "file nativename [file dirname [file dirname [info nameofexecutable]]]";
    if( Tcl_Eval(interp, cmd) == TCL_OK ) {
      Tcl_DStringGetResult(interp, &ascenddist);
      if(Asc_SetPathList(DIST_ENVIRONVAR,Tcl_DStringValue(&ascenddist)) != 0) {
        Asc_Panic(2, "AscCheckEnvironVars",
                  "Asc_SetPathList() returned Nonzero: "
                  "Not enough memory to extend the environment");
      }
    }
  }
  /*  Make sure the Tcl side can also see this variable */
  Tcl_SetVar2(interp, "env", DIST_ENVIRONVAR,
              Tcl_DStringValue(&ascenddist), TCL_GLOBAL_ONLY);

  /*
   *  If the user's environment does not have ASCENDLIBRARY set, then set
   *  it to a reasonable default.
   */
  if( Asc_ImportPathList(LIBR_ENVIRONVAR) == 0 ) {
    if( (tmpenv = Asc_GetEnv(LIBR_ENVIRONVAR)) == NULL ) {
      /* shouldn't be NULL since we just imported it successfully */
      Asc_Panic(2, "CheckEnvironmentVars",
                "Asc_GetEnv(%s) returned NULL value.", LIBR_ENVIRONVAR);
    }
    /*  Make sure the Tcl side can also see this variable */
    Tcl_SetVar2(interp, "env", LIBR_ENVIRONVAR, tmpenv, TCL_GLOBAL_ONLY);
    ascfree(tmpenv);
  } else {
    /*  Add ``.'' to the ASCENDLIBRARY envar */
    if( Asc_SetPathList(LIBR_ENVIRONVAR, ".") != 0 ) {
      Asc_Panic(2, "AscCheckEnvironVars",
                "Asc_SetPathList() returned Nonzero: "
                "Not enough memory to extend the environment");
    }

    /*AWW20041209:  Add ``$ASCENDDIST/models'' to the ASCENDLIBRARY envar */
    /* Add ``$ASCENDDIST/../../models'' to the ASCENDLIBRARY envar */
    Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&ascenddist), -1);
    Tcl_DStringAppend(&buffer2, "/models", -1);
    /* Tcl_DStringAppend(&buffer2, "/../../models", -1); AWW */
    if( NULL != (Tcl_TranslateFileName(interp, Tcl_DStringValue(&buffer2),
                                       &buffer1))) {
      if(Asc_AppendPath(LIBR_ENVIRONVAR, Tcl_DStringValue(&buffer1)) != 0) {
        Asc_Panic(2, "AscCheckEnvironVars",
                  "Asc_AppendPath() returned Nonzero: "
                  "Not enough memory to extend the environment");
      }
      Tcl_DStringFree(&buffer1);
    }
    Tcl_DStringFree(&buffer2);

    /*  Add ``$ASCENDDIST/models/examples'' to the ASCENDLIBRARY envar */
    /*AWW20041209 - remove all this:
    Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&ascenddist), -1);
    Tcl_DStringAppend(&buffer2, "/models/examples", -1);
    if( NULL != (Tcl_TranslateFileName(interp, Tcl_DStringValue(&buffer2),
                                       &buffer1))) {
      if(Asc_AppendPath(LIBR_ENVIRONVAR, Tcl_DStringValue(&buffer1)) != 0) {
        Asc_Panic(2, "AscCheckEnvironVars",
                  "Asc_AppendPath() returned Nonzero: "
                  "Not enough memory to extend the environment");
      }
      Tcl_DStringFree(&buffer1);
    }
    Tcl_DStringFree(&buffer2);
    */
    /*  Add ``$ASCENDDIST/models/libraries'' to the ASCENDLIBRARY envar */
    /*    Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&ascenddist), -1);
    Tcl_DStringAppend(&buffer2, "/models/libraries", -1);
    if( NULL != (Tcl_TranslateFileName(interp, Tcl_DStringValue(&buffer2),
                                       &buffer1))) {
      if(Asc_AppendPath(LIBR_ENVIRONVAR, Tcl_DStringValue(&buffer1)) != 0) {
        Asc_Panic(2, "AscCheckEnvironVars",
                  "Asc_AppendPath() returned Nonzero: "
                  "Not enough memory to extend the environment");
      }
      Tcl_DStringFree(&buffer1);
    }
    Tcl_DStringFree(&buffer2);

    up to here */

    /*  Get the full value of the environment variable and set
     *  $env(ASCENDLIBRARY) in the Tcl code
     */
    if( (tmpenv = Asc_GetEnv(LIBR_ENVIRONVAR)) == NULL ) {
      /* shouldn't be NULL since we just set it.  memory error! */
      Asc_Panic(2, "CheckEnvironmentVars",
                "Asc_GetEnv(%s) returned NULL value.", LIBR_ENVIRONVAR);
    }
    /*  Make sure the Tcl side can also see this variable */
    Tcl_SetVar2(interp, "env", LIBR_ENVIRONVAR, tmpenv, TCL_GLOBAL_ONLY);
    ascfree(tmpenv);
  }

  /*
   *  If the user's environment does not have ASCENDTK set, then set it
   *  by appending `TK' to ASCENDDIST.  Later in this function, we check
   *  to make sure it is a valid directory by checking for the existence
   *  of `AscendRC' in that directory.
   */
  if( Asc_ImportPathList(ASCTK_ENVIRONVAR) == 0 ) {
    if( (tmpenv = Asc_GetEnv(ASCTK_ENVIRONVAR)) == NULL ) {
      /* shouldn't be NULL since we just imported it successfully */
      Asc_Panic(2, "CheckEnvironmentVars",
                "Asc_GetEnv(%s) returned NULL value.", ASCTK_ENVIRONVAR);
    }
    /* store ASCENDTK in ``buffer1'' so we can check for ASCENDTK/AscendRC
     * below
     */
    Tcl_DStringAppend(&buffer1, tmpenv, -1);
    ascfree(tmpenv);
  } else {
    Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&ascenddist), -1);
    /* AWW20041208:    Tcl_DStringAppend(&buffer2, "/TK", -1);
     */
    Tcl_DStringAppend(&buffer2, "/TK", -1);
    if(NULL != (Tcl_TranslateFileName(interp, Tcl_DStringValue(&buffer2),
                                      &buffer1))) {
      if( Asc_SetPathList(ASCTK_ENVIRONVAR, Tcl_DStringValue(&buffer1)) != 0) {
        Asc_Panic(2, "Asc_EnvironmentInit",
                  "Not enough memory to initialize the environment");
      }
    }
    Tcl_DStringFree(&buffer2);
  }
  /*  Make sure the Tcl side can also see this variable */
  Tcl_SetVar2(interp, "env", ASCTK_ENVIRONVAR,
              Tcl_DStringValue(&buffer1), TCL_GLOBAL_ONLY);

  /*
   *  Check to see if ASCENDTK looks reasonable by checking
   *  for ASCENDTK/AscendRC  We use the Tcl channel
   *  mechanism to see if file exists.
   */
  Tcl_DStringAppend(&buffer1, "/AscendRC", -1 );
  c = Tcl_OpenFileChannel( NULL, Tcl_DStringValue(&buffer1), "r", 0 );
  if( c != (Tcl_Channel)NULL ) {
    /*
     *  file exists.  close the channel and set tcl_rcfilename to
     *  this location
     */
    Tcl_Close( NULL, c );
    Tcl_SetVar(interp, "tcl_rcFileName", Tcl_DStringValue(&buffer1),
               TCL_GLOBAL_ONLY);
  } else {
    Asc_Panic(2, "AscCheckEnvironVars",
              "ERROR: Cannot find the file \"%s\" in the subdirectory \"TK\"\n"
              "under the directory \"%s\"\n"
              "Please check the value of the environment variables %s and\n"
              "and %s and start ASCEND again.\n",
              "AscendRC", Tcl_DStringValue(&ascenddist), DIST_ENVIRONVAR,
              ASCTK_ENVIRONVAR);
  }
  Tcl_DStringFree(&buffer1);

  /*
   *  If the user's environment does not have ASCENDBITMAPS set, then set
   *  it by appending `bitmaps' to ASCENDTK.
   */
  if( Asc_ImportPathList(BITMAP_ENVIRONVAR) == 0 ) {
    if( (tmpenv = Asc_GetEnv(BITMAP_ENVIRONVAR)) == NULL ) {
      /* shouldn't be NULL since we just imported it successfully */
      Asc_Panic(2, "CheckEnvironmentVars",
                "Asc_GetEnv(%s) returned NULL value.", BITMAP_ENVIRONVAR);
    }
    /*  Make sure the Tcl side can also see this variable */
    Tcl_SetVar2(interp, "env", BITMAP_ENVIRONVAR, tmpenv, TCL_GLOBAL_ONLY);
    ascfree(tmpenv);
  } else {
    Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&ascenddist), -1);
    Tcl_DStringAppend(&buffer2, "/TK/bitmaps", -1);
    if(NULL != (Tcl_TranslateFileName(interp, Tcl_DStringValue(&buffer2),
                                      &buffer1))) {
      if(Asc_SetPathList(BITMAP_ENVIRONVAR, Tcl_DStringValue(&buffer1)) != 0) {
        Asc_Panic(2, "Asc_EnvironmentInit",
                  "Not enough memory to initialize the environment");
      }
    }
    Tcl_DStringFree(&buffer2);
    /*  Make sure the Tcl side can also see this variable */
    Tcl_SetVar2(interp, "env", BITMAP_ENVIRONVAR,
                Tcl_DStringValue(&buffer1), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&buffer1);
  }

  /*  Cleanup  */
  Tcl_DStringFree(&ascenddist);

  return TCL_OK;
}


/*
 *  int AscSetStartupFile(interp)
 *      Tcl_Interp *interp;
 *
 *  Look for ~/.ascendrc; if found, set  the Tcl variable tcl_rcFileName
 *  to this file's location.  This overrides the value set in
 *  AscCheckEnvironVars().
 *  If ~/_ascendrc is available it only gets used if ~/.ascendrc is not.
 *  Returns a standard Tcl return code.
 */
static int AscSetStartupFile(Tcl_Interp *interp)
{
  char *fullname;        /* try to find this if first fails */
  Tcl_DString buffer;
  Tcl_Channel c;         /* used to check for file existance */

  Tcl_ResetResult(interp);

  fullname = Tcl_TranslateFileName( interp, "~/.ascendrc", &buffer );
  if( fullname != NULL ) {
    /*
     *  Use the Tcl file channel routines to determine if ~/.ascendrc
     *  exists.  We cannot use access() since Windows doesn't use it.
     */
    c = Tcl_OpenFileChannel( NULL, fullname, "r", 0 );
    if( c != (Tcl_Channel)NULL ) {
      /* file exists. close the channel and set tcl_rcFileName. */
      Tcl_Close( NULL, c );
      Tcl_SetVar(interp, "tcl_rcFileName", fullname, TCL_GLOBAL_ONLY);
      Tcl_DStringFree(&buffer);
      return TCL_OK;
    }
    Tcl_DStringFree(&buffer);
  }
  fullname = Tcl_TranslateFileName( interp, "~/_ascendrc", &buffer );
  if( fullname != NULL ) {
    /*
     *  Use the Tcl file channel routines to determine if ~/_ascendrc
     *  exists.  We cannot use access() since Windows doesn't use it.
     */
    c = Tcl_OpenFileChannel( NULL, fullname, "r", 0 );
    if( c != (Tcl_Channel)NULL ) {
      /* file exists.  close the channel and set tcl_rcFileName */
      Tcl_Close( NULL, c );
      Tcl_SetVar(interp, "tcl_rcFileName", fullname, TCL_GLOBAL_ONLY);
      Tcl_DStringFree(&buffer);
      return TCL_OK;
    }
    Tcl_DStringFree(&buffer);
  }
  return TCL_OK; /* probably should be TCL_ERROR */
}



/*
 *  file = AscProcessCommandLine(argc, argv)
 *      char *file;
 *      int   argc;
 *      char *argv[];
 *
 *  Process the options given on the command line `argv' where `argc' is
 *  the length of argv.
 *
 *  Strip out ASCEND specific flags and then pass the rest to Tcl so it
 *  can set what it needs.
 *
 *  This function may call exit() if the user requests help.
 */
static int AscProcessCommandLine(Tcl_Interp *interp, int argc, CONST84 char **argv)
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
 
  /*
   *  Make command-line arguments available in the Tcl variables "argc"
   *  and "argv".
   */
  args = Tcl_Merge(new_argc, (argv+1));
  Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
  ckfree(args);
  sprintf(buf, "%d", new_argc);
  Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "argv0", argv[0], TCL_GLOBAL_ONLY);

  return TCL_OK;
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
  Tcl_Exit(0);  /*  Show this help message and leave  */
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
 *  See this file's header for documentation.
 */
int Asc_LoadWin(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(stderr,"call is: ascloadwin <no args> \n");
    return TCL_ERROR;
  }
  if (g_interfacever) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *----------------------------------------------------------------------
 *  The following StdinProc() and Prompt() are from tkMain.c in
 *  the Tk4.1 distribution (and did not change in Tk8.0).
 *----------------------------------------------------------------------
 *----------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(ClientData clientData, int mask)
{
  static int gotPartial = 0;
  char *cmd;
  int code, count;
  Tcl_Channel chan = (Tcl_Channel) clientData;

  Tcl_Interp *interp = g_interp;      /* use a local copy of the
                                       * global tcl interpreter
                                       */

  (void)clientData;  /* stop gcc whine about unused parameter */
  (void)mask;        /* stop gcc whine about unused parameter */

  count = Tcl_Gets(chan, &g_line);

  if (count < 0) {
    if (!gotPartial) {
      if (tty) {
        return;
      } else {
        Tcl_DeleteChannelHandler(chan, StdinProc, (ClientData) chan);
      }
      return;
    } else {
      count = 0;
    }
  }

  (void) Tcl_DStringAppend(&g_command, Tcl_DStringValue(&g_line), -1);
  cmd = Tcl_DStringAppend(&g_command, "\n", -1);
  Tcl_DStringFree(&g_line);

  if (!Tcl_CommandComplete(cmd)) {
    gotPartial = 1;
    goto prompt;
  }
  gotPartial = 0;

  /*
   * Disable the stdin channel handler while evaluating the command;
   * otherwise if the command re-enters the event loop we might
   * process commands from stdin before the current command is
   * finished.  Among other things, this will trash the text of the
   * command being evaluated.
   */

  Tcl_CreateChannelHandler(chan, 0, StdinProc, (ClientData) chan);
  code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
  Tcl_CreateChannelHandler(chan, TCL_READABLE, StdinProc,
                           (ClientData) chan);
  Tcl_DStringFree(&g_command);
  if (*interp->result != 0) {
    if ((code != TCL_OK) || (tty)) {
      /*
       * The statement below used to call "printf", but that resulted
       * in core dumps under Solaris 2.3 if the result was very long.
       *
       * NOTE: This probably will not work under Windows either.
       */

      puts(interp->result);
    }
  }

  /*
   * Output a prompt.
   */

 prompt:
  if (tty) {
    Prompt(interp, gotPartial);
  }
  Tcl_ResetResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Prompt --
 *
 *	Issue a prompt on standard output, or invoke a script
 *	to issue the prompt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A prompt gets output, and a Tcl script may be evaluated
 *	in interp.
 *
 * Parameters:
 *   interp    Interpreter to use for prompting.
 *   partial   Non-zero means there already exists a partial
 *             command, so use the secondary prompt.
 *
 *----------------------------------------------------------------------
 */

static void
Prompt(Tcl_Interp *interp, int partial)
{
  CONST84 char *promptCmd;
  int code;
  Tcl_Channel outChannel, errChannel;
  CONST84 char *subPrompt;

  errChannel = Tcl_GetChannel(interp, "stderr", NULL);

  subPrompt = (partial ? "tcl_prompt2" : "tcl_prompt1");
  promptCmd = Tcl_GetVar(interp, subPrompt, TCL_GLOBAL_ONLY);
  if (promptCmd == NULL) {
  defaultPrompt:

    /*
     * We must check that outChannel is a real channel - it
     * is possible that someone has transferred stdout out of
     * this interpreter with "interp transfer".
     */

    outChannel = Tcl_GetChannel(interp, "stdout", NULL);
    if (outChannel != (Tcl_Channel) NULL) {
      if (!partial) {
        Tcl_Write(outChannel, "AscendIV% ", 10);
      } else {
        Tcl_Write(outChannel, "more? ", 6);
      }
    }
  } else {
    code = Tcl_Eval(interp, promptCmd);
    if (code != TCL_OK) {
      Tcl_AddErrorInfo(interp,
                       "\n    (script that generates prompt)");
      /*
       * We must check that errChannel is a real channel - it
       * is possible that someone has transferred stderr out of
       * this interpreter with "interp transfer".
       */

      errChannel = Tcl_GetChannel(interp, "stderr", NULL);
      if (errChannel != (Tcl_Channel) NULL) {
        Tcl_Write(errChannel, interp->result, -1);
        Tcl_Write(errChannel, "\n", 1);
      }
      goto defaultPrompt;
    }
  }
  outChannel = Tcl_GetChannel(interp, "stdout", NULL);
  if (outChannel != (Tcl_Channel) NULL) {
    Tcl_Flush(outChannel);
  }
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

int Asc_DebugMallocCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  union dbmalloptarg m;

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage __dbmalloc ?on?off?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  m.str = NULL;
  if (strcmp(argv[1],"on")==0) {
    m.i = 1;
  } else if (strcmp(argv[1],"off")==0) {
    m.i = 0;
  } else {
    Tcl_SetResult(interp, "incorrect args : should be \"on\" or \"off\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  dbmallopt(MALLOC_CKDATA,&m); /* turn on str* mem* and b* checking */
  return TCL_OK;
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
 * Parameters:
 *    argcptr  Filled with number of argument strings.
 *    argvptr  Filled with argument strings (malloc'd).
 *
 * This function is from the Tk 8.0 distribution.  See win/winMain.c in
 * their sources.
 *
 *--------------------------------------------------------------------------
 */
static void
setargv(int *argcPtr, char ***argvPtr)
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
