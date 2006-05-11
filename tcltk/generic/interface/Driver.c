/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*
	Driver.c
	by Kirk Abbott and Ben Allan
	Created: 1/94
	Version: $Revision: 1.48 $
	Version control file: $RCSfile: Driver.c,v $
	Date last modified: $Date: 2003/08/23 18:43:06 $
	Last modified by: $Author: ballan $
*/

#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <tcl.h>
#include <tk.h>
#include <utilities/ascConfig.h>
#include <general/ospath.h>
#include <utilities/ascPrint.h>
#include <utilities/error.h>

#ifndef __WIN32__
# include <unistd.h>
#else

/* jds20041229 - windows.h now included in ascConfig.h. */
/* jp - i took it back out of ascConfig.h - Apr 2005 */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# include <locale.h>
# include "tkConsole.h"
# undef WIN32_LEAN_AND_MEAN
#endif /* __WIN32__ */

#include <utilities/config.h>
#include <utilities/ascMalloc.h>    /* for ascshutdown */
#include <utilities/ascPanic.h>     /* for Asc_Panic */
#include <utilities/ascEnvVar.h>
#include <compiler/compiler.h>
#include <compiler/ascCompiler.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/compiler.h>      /* for symchar for units.h */
#include <compiler/units.h>
#include <compiler/redirectFile.h>  /* for Asc_RedirectCompilerDefault() */
#include <solver/slv_types.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/logrel.h>
#include <solver/discrete.h>
#include <solver/mtx.h>
#include <solver/slv_stdcalls.h>
#include "AscBitmaps.h"
#include <utilities/ascPrint.h>
#include "AscPrintTcl.h"
#include "HelpProc.h"
#include "Commands.h"
#include "Driver.h"
#include "ScriptProc.h"
#include "SolverProc.h"
#include "UnitsProc.h"

#ifndef lint
static CONST char DriverID[] = "$Id: Driver.c,v 1.48 2003/08/23 18:43:06 ballan Exp $";
#endif

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

static void AscTrap(int);
static int  AscCheckEnvironVars(Tcl_Interp*,const char *progname);
static void AscPrintHelpExit(CONST char *);
static int  AscProcessCommandLine(Tcl_Interp*, int, CONST char **);
static void Prompt(Tcl_Interp*, int);
static int  AscSetStartupFile(Tcl_Interp*);
static void StdinProc(ClientData, int);
#ifdef DEBUG_MALLOC
static void InitDebugMalloc(void);
#endif /* DEBUG_MALLOC */


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

/**
	Moved 'main' and 'WinMain' to separate 'main.c'
	so that ascend4.exe can be built without linkage to Tcl/Tk
*/


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
int AscDriver(int argc, CONST char *argv[])
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

  color_on(stderr,"34;1");
  ASC_FPRINTF(stderr,"\nASCEND modelling environment\n");
  ASC_FPRINTF(stderr,"Copyright(C) 1997, 2006 Carnegie Mellon University\n");
  ASC_FPRINTF(stderr,"Copyright(C) 1993-1996 Kirk Andre Abbott, Ben Allan\n");
  ASC_FPRINTF(stderr,"Copyright(C) 1990, 1993, 1994 Thomas Guthrie Epperly\n");
  ASC_FPRINTF(stderr,"Built %s %s %s\n\n",__DATE__,__TIME__,build_name);
  ASC_FPRINTF(stderr,"ASCEND comes with ABSOLUTELY NO WARRANTY, and is free software that you may\n");
  ASC_FPRINTF(stderr,"redistribute within the conditions of the GNU General Public License. See the\n");
  ASC_FPRINTF(stderr,"included file 'LICENSE.txt' for full details.\n\n");
  color_off(stderr);

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
  AscCheckEnvironVars(interp,argv[0]);
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

/*-------------------------------------------------------
  SETTING UP ENVIRONMENT...
*/

#define GETENV Asc_GetEnv
#define PUTENV Asc_PutEnv

/**
	This is a quick macro to output a FilePath to an environment
	variable.
*/
#define OSPATH_PUTENV(VAR,FP) \
	CONSOLE_DEBUG("VAR: %s",VAR); \
	sprintf(envcmd,"%s=",VAR); \
	ospath_strcat(FP,envcmd,MAX_ENV_VAR_LENGTH); \
	CONSOLE_DEBUG("ENVCMD: %s",envcmd); \
	PUTENV(envcmd)

/**
	This is a quick macro to send data to Tcl using Tcl_SetVar.
	It uses an intermediate buffer which is assumed to be
	empty already.

	usage: ASC_SEND_TO_TCL(tclvarname,"some string value");
*/
#define ASC_SEND_TO_TCL(VAR,VAL) \
	Tcl_DStringAppend(&buffer,VAL,-1); \
	Tcl_SetVar(interp,#VAR,Tcl_DStringValue(&buffer),TCL_GLOBAL_ONLY); \
	Tcl_DStringFree(&buffer);

/**
	This is a quick macro to send data to Tcl using Tcl_SetVar2.
	It uses an intermediate buffer which is assumed to be
	empty already.

	usage: ASC_SEND_TO_TCL2(arrayname,"keyname","some string value");
*/
#define ASC_SEND_TO_TCL2(ARR,KEY,VAL) \
	Tcl_DStringAppend(&buffer,VAL,-1); \
	Tcl_SetVar2(interp,#ARR,KEY,Tcl_DStringValue(&buffer),TCL_GLOBAL_ONLY); \
	Tcl_DStringFree(&buffer);

static void printenv(){
	int n;
	char **l;
	l = Asc_EnvNames(&n);
	CONSOLE_DEBUG("VARS = %d",n);
}

/**
	Ensure that all required environment variables are present
	and set values for them if they are not present. The names
	for the environment variables are specified in
	<utilities/config.h>. The following comments assume that you
	use the usual names for each of these:

	ASCENDDIST defaults to @ASC_DATADIR@ (also in config.h)
	ASCENDTK defaults to $ASCENDDIST/@ASC_TK_SUBDIR_NAME@ (latter is from config.h)
	ASCENDBITMAPS defaults $ASCENDTK/bitmaps
	ASCENDLIBRARY defaults to $ASCENDDIST/models

	Also check for the existence of the file AscendRC in $ASCENDTK
	and if found, export the location of that file to the Tcl
	variable tcl_rcFileName.
*/
static int AscCheckEnvironVars(Tcl_Interp *interp,const char *progname){
	char *distdir, *tkdir, *bitmapsdir, *librarydir;
	struct FilePath *fp, *fp1, *distfp, *tkfp, *bitmapsfp, *libraryfp;
	char envcmd[MAX_ENV_VAR_LENGTH];
	char s1[PATH_MAX];

	Tcl_DString buffer;

	Tcl_DStringInit(&buffer);

	/*
	Asc_ImportPathList(ASC_ENV_DIST);
	Asc_ImportPathList(ASC_ENV_TK);
	Asc_ImportPathList(ASC_ENV_BITMAPS);
	Asc_ImportPathList(ASC_ENV_LIBRARY);
	*/

    CONSOLE_DEBUG("IMPORTING VARS");

	distdir = GETENV(ASC_ENV_DIST);
	tkdir = GETENV(ASC_ENV_TK);
	bitmapsdir = GETENV(ASC_ENV_BITMAPS);
	librarydir = GETENV(ASC_ENV_LIBRARY);

	int guessedtk=0;

	/* Create an ASCENDDIST value if it's missing */

	if(distdir == NULL){
		CONSOLE_DEBUG("NO " ASC_ENV_DIST " VAR DEFINED");

# ifdef ASC_RELATIVE_PATHS

		// read the executable's name/relative path.
        fp = ospath_new(progname);

        ospath_strcpy(fp,s1,PATH_MAX);
        CONSOLE_DEBUG("PROGNAME = %s",s1);

		// get the directory name from the exe path
        CONSOLE_DEBUG("Calculating dir...");
        fp1 = ospath_getdir(fp);
        CONSOLE_DEBUG("Done calculating dir...");
        ospath_free(fp);

        ospath_strcpy(fp1,s1,PATH_MAX);
        CONSOLE_DEBUG("DIR = %s",s1);

		// append the contents of ASC_DISTDIR to this path
        fp = ospath_new_noclean(ASC_DISTDIR);
		distfp = ospath_concat(fp1,fp);
		ospath_cleanup(distfp);

        ospath_strcpy(fp1,s1,PATH_MAX);
        CONSOLE_DEBUG("DIST = %s",s1);

# else
		distfp = ospath_new(ASC_DATADIR);
		fp = ospath_new("ascend");

		ospath_append(distfp,fp);
		ospath_free(fp);
# endif
		distdir = ospath_str(distfp);
		CONSOLE_DEBUG("GUESSING %s = %s",ASC_ENV_DIST,distdir);
		OSPATH_PUTENV(ASC_ENV_DIST,distfp);
		distdir = GETENV(ASC_ENV_DIST);
		CONSOLE_DEBUG("RETRIEVED %s = %s",ASC_ENV_DIST,distdir);
		printenv();
	}

	if(tkdir == NULL){
		CONSOLE_DEBUG("NO " ASC_ENV_TK " VAR DEFINED");

		guessedtk=1;
		/* Create a path $ASCENDDIST/tcltk */
		strcpy(envcmd,"$ASCENDDIST/");
		strcat(envcmd,ASC_TK_SUBDIR_NAME);
		CONSOLE_DEBUG("TK RAW = %s",envcmd);
		tkfp = ospath_new_expand_env(envcmd, &GETENV);
		tkdir = ospath_str(tkfp);

		ospath_strcpy(tkfp,envcmd,MAX_ENV_VAR_LENGTH);
		CONSOLE_DEBUG("TK = %s",envcmd);

		OSPATH_PUTENV(ASC_ENV_TK,tkfp);
	}

	if(bitmapsdir == NULL){
	    CONSOLE_DEBUG("NO  " ASC_ENV_BITMAPS " VAR DEFINED");
		/* Create a path $ASCENDTK/bitmaps */
		bitmapsfp = ospath_new_expand_env("$ASCENDTK/bitmaps", &GETENV);
		OSPATH_PUTENV(ASC_ENV_BITMAPS,bitmapsfp);
		bitmapsdir = ospath_str(bitmapsfp);
	}

	/**
		@TODO FIXME Note, at present this default library path only caters for a
		** SINGLE PATH COMPONENT **

		@TODO Also, what about ASCEND_DEFAULTLIBRARY ?
	*/
	if(librarydir == NULL){
	    CONSOLE_DEBUG("NO  " ASC_ENV_LIBRARY " VAR DEFINED");
		libraryfp = ospath_new_expand_env("$ASCENDDIST/models", &GETENV);
		ospath_free(fp);
		OSPATH_PUTENV(ASC_ENV_LIBRARY,libraryfp);
		librarydir = ospath_str(libraryfp);
	}


    CONSOLE_DEBUG("CHECKING FOR AscendRC FILE");

	fp1 = ospath_new("AscendRC");
	fp = ospath_concat(tkfp,fp1);
	ospath_free(fp1);
	FILE *f = ospath_fopen(fp,"r");
	if(f==NULL){
		if(guessedtk){
			Asc_Panic(2, "AscCheckEnvironVars",
				"Cannot located AscendRC file in expected (guessed) location:\n%s\n"
				"Please set the %s environment variable to the correct location (typically\n"
				"it would be c:\\Program Files\\ASCEND\\TK or /usr/share/ascend/tcltk/TK. You\n"
				"should do this, then start ASCEND again."
					,tkdir,ASC_ENV_TK
			);
		}else{
			Asc_Panic(2, "AscCheckEnvironVars",
				"Cannot located AscendRC file in the specified location:\n%s\n"
				"Please check your value for the %s environment variable.\n"
					,tkdir,ASC_ENV_TK
			);
		}
		/* can't get here, hopefully */
	}
	fclose(f);
	/* reuse 'envcmd' to get the string file location for AscendRC */
	ospath_strcpy(fp,envcmd,MAX_ENV_VAR_LENGTH);
	ospath_free(fp);

	/* export the value to Tcl/Tk */
    ASC_SEND_TO_TCL(tcl_rcFileName, envcmd);

    /* send all the environment variables to Tcl/Tk as well */
    ASC_SEND_TO_TCL2(env,ASC_ENV_DIST,distdir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_LIBRARY,librarydir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_BITMAPS,bitmapsdir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_TK,tkdir);
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
static int AscProcessCommandLine(Tcl_Interp *interp, int argc, CONST char **argv)
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


