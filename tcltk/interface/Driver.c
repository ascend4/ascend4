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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//*
	by Kirk Abbott and Ben Allan
	Created: 1/94
	Last in CVS: $Revision: 1.48 $ $Date: 2003/08/23 18:43:06 $ $Author: ballan $
*/

#define ASC_BUILDING_INTERFACE

#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <tcl.h>
#include <tk.h>
#include <ascend/utilities/config.h>
#include "config.h"
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascPrint.h>
#include <ascend/utilities/error.h>
#include <ascend/solver/solver.h>
#ifdef ASC_SIGNAL_TRAPS
# include <ascend/utilities/ascSignal.h>
#endif

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

#include <ascend/utilities/config.h>
#include <ascend/general/ascMalloc.h>    /* for ascshutdown */
#include <ascend/general/panic.h>     /* for Asc_Panic */
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/ascPrint.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/units.h>
/* #include <compiler/redirectFile.h> */  /* for Asc_RedirectCompilerDefault() */
#include <ascend/compiler/simlist.h>

#include <ascend/linear/mtx.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/slv_stdcalls.h>

#include "AscBitmaps.h"
#include "AscPrintTcl.h"
#include "HelpProc.h"
#include "Commands.h"
#include "Driver.h"
#include "ScriptProc.h"
#include "SolverProc.h"
#include "UnitsProc.h"

//#define ASCTK_DEBUG

/*
 *  EXPORTED VARIABLES
 */

/**
	Interpreter for this application.  We need to make it global
	so that our signal/floating-porint traps can access it.
*/
Tcl_Interp *g_interp;

/*
	Comes from the yacc file if yacc was built with debugging information
*/
#ifdef ZZ_DEBUG
extern int zz_debug;
#endif


/*
	Declarations for procedures defined outside of this file.
 */
extern int  Tktable_Init(Tcl_Interp*);

#ifdef ASC_SIGNAL_TRAPS
static void AscTrap(int);
#endif

static void  AscCheckEnvironVars(Tcl_Interp*,const char *progname);
static void AscPrintHelpExit(CONST char *);
static int  AscProcessCommandLine(Tcl_Interp*, int, CONST char **);
static int  AscSetStartupFile(Tcl_Interp*);
static void StdinProc(ClientData, int);
#ifdef DEBUG_MALLOC
static void InitDebugMalloc(void);
#endif /* DEBUG_MALLOC */


/*
	LOCALLY GLOBAL VARIABLES
	think global, act local :-)
*/

/**
	TRUE for compiler optimizations default is TRUE, set to FALSE by passing
	+s on the command line
*/
static int g_interface_simplify_relations = TRUE;

/** TRUE if windows to be built; default is TRUE, false is not supported */
static int g_interfacever = 1;

/** Used to assemble lines of terminal input into Tcl commands. */
static Tcl_DString g_command;

/** Used to read the next line from the terminal input. */
static Tcl_DString g_line;

/**
	Non-zero means standard input is a terminal-like device.
	Zero means it's a file.
*/
static int tty;

/*
	initScriptTclAdjust
	initScriptTkAdjust

	These two variables hold Tcl scripts that will set the TCL_LIBRARY
	and TK_LIBRARY environment variables if they are not already
	set in the user's environment.
	TCL_LIBRARY is set to:  dirnameofexecutable/../../Tcl/lib/tcl8.0
	TK_LIBRARY is set to $tcl_library/../tk8.0
	These values are true for cmu only and tk8.0 only (heuristic)
	and so if unset we are changing to accept the tk built in values.
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
#set parentDir [file dirname [info nameofexecutable]]\n\
#set env(TCL_LIBRARY) $parentDir/../../Tcl/lib/tcl8.0\n\
	set env(TCL_LIBRARY) [info library]\n\
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
#set parentDir [file dirname [info tk_library]]\n\
#set env(TK_LIBRARY) $parentDir/tk8.0\n\
	set env(TK_LIBRARY) $tk_library\n\
    }\n\
  }\n\
asc_tkInit";
/**<
	This assumes tcl_library has been found and that tcl8.0 and tk8.0
	are installed in the same lib directory -- the default tcl/tk install.
*/


/**
	who built this binary and when
 */
#ifndef TIMESTAMP
static char build_name[]="by anonymous";
#else
static char build_name[]=TIMESTAMP;
#endif /* TIMESTAMP */

/*
	jp: Moved 'main' and 'WinMain' to separate 'main.c'
	so that ascend4.exe can be built without linkage to Tcl/Tk
*/


/**
	A common entry point for Windows and Unix.  The corresponding
	WinMain() and main() functions just call this function.

	This function creates a Tcl interpreter, initializes Tcl and Tk,
	initializes the Ascend data structures, sets up the user's
	environment, sources ASCEND's startup script, and calls Tk_MainLoop
	so the user can interact with ASCEND.  Cleans up and exits the
	program when Tk_MainLoop returns.

	This function is based on the functions Tk_Main and Tcl_AppInit
	from the Tk8.0 distribution.  See the files tkMain.c and tkAppInit.c
	in the Tk sources.
*/
int AscDriver(int argc, CONST char **argv)
{
  Tcl_Interp *interp;                   /* local version of global g_interp */
  Tcl_Channel inChannel;
  Tcl_Channel outChannel;

  /* Remove the stream redirection stuff for the moment -- JP Nov 2006 */
  /* jds20050119:  Initialize ASCERR before any calls to ascPanic(). */
  /* TODO: revisit when interface is decoupled from base - this may change. */
#ifdef REIMPLEMENT_STREAMS
  Asc_RedirectCompilerDefault();
#endif

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
    ASC_PANIC("Tcl initialization failed:\n%s",
              Tcl_GetStringResult(interp));
  }
  (void)Tcl_Eval(interp,initScriptTkAdjust);
  if (Tk_Init(interp) == TCL_ERROR) {
    ASC_PANIC("Tk initialization failed:\n%s",
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

  color_on(stderr,ASC_FG_BRIGHTBLUE );
  ASC_FPRINTF(stderr,"\nASCEND modelling environment\n");
  ASC_FPRINTF(stderr,"Copyright(C) 1997, 2006-2007 Carnegie Mellon University\n");
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
#ifdef ASC_SIGNAL_TRAPS
  (void)SIGNAL(SIGINT, AscTrap);
#endif

#ifdef DEBUG_MALLOC
  InitDebugMalloc();
  ascstatus("Memory status after calling InitDebugMalloc()");
#endif /* DEBUG_MALLOC */
  if ( Asc_CompilerInit(g_interface_simplify_relations) != 0 ) {
    Asc_Panic(2, "Asc_CompilerInit",
              "Insufficient memory to initialize compiler.");
  }

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

  SlvRegisterStandardClients();
  if( Asc_HelpInit() == TCL_ERROR ) {
    Asc_Panic(2, "Asc_HelpInit",
              "Insufficient memory to initialize help system.");
  }
  Asc_CreateCommands(interp);
  Asc_RegisterBitmaps(interp);



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
    Asc_Prompt(interp, 0);
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
	/*CONSOLE_DEBUG("VAR: %s",VAR);*/ \
	sprintf(tmp,"%s=",VAR); \
	ospath_strcat(FP,tmp,MAX_ENV_VAR_LENGTH); \
	/*CONSOLE_DEBUG("TEMP: %s",tmp);*/ \
	PUTENV(tmp)

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
	const char **l;
	l = Asc_EnvNames(&n);
#ifdef ASCTK_DEBUG
	CONSOLE_DEBUG("VARS = %d",n);
#endif
	ascfree(l);
}

static void AscSaveOrgEnv(Tcl_Interp *interp,const char *progname);

/**
	Ensure that all required environment variables are present
	and set values for them if they are not present. The names
	for the environment variables are specified in
	<utilities/config.h>. The following comments assume that you
	use the usual names for each of these:

	For Tcl/Tk the following #defines can be needed:
		ASCENDDIST
		ASCENDTK_DEFAULT
		ASCENDLIBRARY_DEFAULT
		ASCENDSOLVERS_DEFAULT

		ASC_ABSOLUTE_PATHS
		ASC_DIST_REL_BIN
		ASC_TK_REL_DIST
		ASC_LIBRARY_REL_DIST
		ASC_SOLVERS_REL_DIST

	Then, using these, in the case of relative paths, we choose:

		ASCENDDIST = $PROGDIR/@ASC_DIST_REL_BIN@
		ASCENDTK = $ASCENDROOT/@ASC_TK_REL_DIST@
		ASCENDLIBRARY = $ASCENDROOT/@ASC_LIBRARY_REL_DIST@
		ASCENDSOLVERS = $ASCENDROOT/@ASC_SOLVERS_ROL_DIST@

	Also check for the existence of the file AscendRC in $ASCENDTK
	and if found, export the location of that file to the Tcl
	variable tcl_rcFileName.

	If you set ASC_ABSOLUTE_PATHS then ASCENDDIST defaults to @ASC_DATADIR@ and
	the rest follows through as above.
*/
static void AscCheckEnvironVars(Tcl_Interp *interp,const char *progname){
	char *distdir, *tkdir, *bitmapsdir, *librarydir, *solversdir;
	struct FilePath *fp, *fp1, *distfp, *tkfp, *bitmapsfp, *libraryfp, *solversfp;
	char tmp[MAX_ENV_VAR_LENGTH];
#if !ASC_ABSOLUTE_PATHS
	char s1[PATH_MAX];
#endif
	int guessedtk=0;
	FILE *f;

	Tcl_DString buffer;

	Tcl_DStringInit(&buffer);

	AscSaveOrgEnv(interp, progname);

	/* import these into the environment */
#ifdef ASCTK_DEBUG
	int err =
#endif
	env_import(ASC_ENV_DIST,getenv,PUTENV,0);
#ifdef ASCTK_DEBUG
	if (err) CONSOLE_DEBUG("No %s var imported (error %d)",ASC_ENV_DIST,err);
#endif
	env_import(ASC_ENV_TK,getenv,PUTENV,0);
	env_import(ASC_ENV_BITMAPS,getenv,PUTENV,0);
	env_import(ASC_ENV_LIBRARY,getenv,PUTENV,0);
	env_import(ASC_ENV_SOLVERS,getenv,PUTENV,0);

	/* used for colour console output */
	env_import("TERM",getenv,PUTENV,0);

#ifdef ASCTK_DEBUG
	CONSOLE_DEBUG("IMPORTING VARS");
#endif

	distdir = GETENV(ASC_ENV_DIST);
	tkdir = GETENV(ASC_ENV_TK);
	bitmapsdir = GETENV(ASC_ENV_BITMAPS);
	librarydir = GETENV(ASC_ENV_LIBRARY);
	solversdir = GETENV(ASC_ENV_SOLVERS);

	/* Create an ASCENDDIST value if it's missing */

	if(distdir == NULL){
#ifdef ASCTK_DEBUG
		CONSOLE_DEBUG("Note: No '" ASC_ENV_DIST "' var defined");
#endif

#if ASC_ABSOLUTE_PATHS
#ifdef ASCTK_DEBUG
		CONSOLE_DEBUG("ASC_ABSOLUTE_PATHS=%d",ASC_ABSOLUTE_PATHS);
#endif
		distfp = ospath_new(ASCENDDIST_DEFAULT);
		(void)progname;
#else
		/* read the executable's name/relative path.*/
        fp = ospath_new(progname);

        ospath_strncpy(fp,s1,PATH_MAX);
#ifdef ASCTK_DEBUG
        CONSOLE_DEBUG("PROGNAME = %s",s1);
#endif

		/* get the directory name from the exe path*/
        fp1 = ospath_getdir(fp);
        ospath_free(fp);

		/* convert to absolute */
		fp = ospath_getabs(fp1);
		ospath_free(fp1);

		/* append the contents of ASC_DISTDIR_REL_BIN to this path*/
        fp1 = ospath_new_noclean(ASC_DIST_REL_BIN);

		distfp = ospath_concat(fp,fp1);
		ospath_cleanup(distfp);
		ospath_free(fp1);
		ospath_free(fp);
#endif

		distdir = ospath_str(distfp);

#ifdef ASCTK_DEBUG
		CONSOLE_DEBUG("Setting distdir %s = %s",ASC_ENV_DIST,distdir);
#endif

		OSPATH_PUTENV(ASC_ENV_DIST,distfp);
		distdir = GETENV(ASC_ENV_DIST);
		/* CONSOLE_DEBUG("RETRIEVED %s = %s",ASC_ENV_DIST,distdir); */
		printenv();
		ospath_free(distfp);
	}

	if(tkdir == NULL){
		/* no env var ASCENDTK... create value from compile-time info */
		guessedtk=1;
#if ASC_ABSOLUTE_PATHS
		tkfp = ospath_new_expand_env(ASCENDTK_DEFAULT, &GETENV,1);
#else
		fp = ospath_new(ASC_TK_REL_DIST);
		distfp = ospath_new(distdir);
		tkfp = ospath_concat(distfp,fp);
		ospath_free(distfp);
		ospath_free(fp);
		ospath_cleanup(tkfp);
#endif
		tkdir = ospath_str(tkfp);
		OSPATH_PUTENV(ASC_ENV_TK,tkfp);
	}else{
		/* expand env vars in ASCENDTK */
		tkfp = ospath_new_expand_env(tkdir, &GETENV,1);
		tkdir = ospath_str(tkfp);
		OSPATH_PUTENV(ASC_ENV_TK,tkfp);
	}

	if(bitmapsdir == NULL){
	    /* CONSOLE_DEBUG("NO " ASC_ENV_BITMAPS " VAR DEFINED"); */
		/* Create a path $ASCENDTK/bitmaps */
		bitmapsfp = ospath_new_expand_env("$ASCENDTK/bitmaps", &GETENV,1);
		OSPATH_PUTENV(ASC_ENV_BITMAPS,bitmapsfp);
		bitmapsdir = ospath_str(bitmapsfp);
		ospath_free(bitmapsfp);
	}

	/**
		@TODO FIXME Note, at present this default library path only caters for a
		** SINGLE PATH COMPONENT **

		@TODO Also, what about ASCEND_DEFAULTLIBRARY ?
	*/
	if(librarydir == NULL){
#if ASC_ABSOLUTE_PATHS
		libraryfp = ospath_new(ASCENDLIBRARY_DEFAULT);
#else
		libraryfp = ospath_new_expand_env("$ASCENDDIST/" ASC_LIBRARY_REL_DIST, &GETENV,1);
#endif
		/* CONSOLE_DEBUG("CREATED LIBRARY VAL"); */
		OSPATH_PUTENV(ASC_ENV_LIBRARY,libraryfp);
		librarydir = ospath_str(libraryfp);
		ospath_free(libraryfp);
	}

	if(solversdir == NULL){
#if ASC_ABSOLUTE_PATHS
		solversfp = ospath_new(ASCENDSOLVERS_DEFAULT);
#else
		solversfp = ospath_new_expand_env("$ASCENDDIST/" ASC_SOLVERS_REL_DIST, &GETENV,1);
#endif
		/* CONSOLE_DEBUG("CREATED SOLVERS VAL"); */
		OSPATH_PUTENV(ASC_ENV_SOLVERS,solversfp);
		solversdir = ospath_str(solversfp);
		ospath_free(solversfp);
	}

#ifdef ASCTK_DEBUG
	CONSOLE_DEBUG("ASCENDDIST = %s",GETENV(ASC_ENV_DIST));
	CONSOLE_DEBUG("ASCENDTK = %s",GETENV(ASC_ENV_TK));
	CONSOLE_DEBUG("ASCENDLIBRARY = %s",GETENV(ASC_ENV_LIBRARY));
	CONSOLE_DEBUG("ASCENDSOLVERS = %s",GETENV(ASC_ENV_SOLVERS));

    CONSOLE_DEBUG("CHECKING FOR AscendRC FILE");
#endif

	fp1 = ospath_new("AscendRC");
	fp = ospath_concat(tkfp,fp1);
	ospath_free(fp1);
	ospath_free(tkfp);
	f = ospath_fopen(fp,"r");
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

	/* put AscendRC location in string and export to Tcl */
	ospath_strncpy(fp,tmp,MAX_ENV_VAR_LENGTH);
    ASC_SEND_TO_TCL(tcl_rcFileName, tmp);
	ospath_free(fp);

    /* send all the environment variables to Tcl/Tk as well */
    ASC_SEND_TO_TCL2(env,ASC_ENV_DIST,distdir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_LIBRARY,librarydir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_BITMAPS,bitmapsdir);
    ASC_SEND_TO_TCL2(env,ASC_ENV_TK,tkdir);
}



/**
	Look for ~/.ascendrc; if found, set  the Tcl variable tcl_rcFileName
	to this file's location.  This overrides the value set in
	AscCheckEnvironVars().
	If ~/_ascendrc is available it only gets used if ~/.ascendrc is not.
	Returns a standard Tcl return code.
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


/**
	Process the options given on the command line `argv' where `argc' is
	the length of argv.

	Strip out ASCEND specific flags and then pass the rest to Tcl so it
	can set what it needs.

	This function may call exit() if the user requests help.
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


/**
	Print a help message and exit.  Use invoke_name as the name of the binary
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


#ifdef ASC_SIGNAL_TRAPS
/**
	Function to call when we receive an interrupt.
 */
static
void AscTrap(int sig)
{
  putchar('\n');
  Asc_Panic(sig, "AscTrap", "Caught Signal: %d", sig);
}
#endif

int Asc_LoadWin(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
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

/*---------------------------------------------------------------------
  The following StdinProc() and Asc_Prompt() are from tkMain.c in
  the Tk4.1 distribution (and did not change in Tk8.0).
  ----------------------------------------------------------------------*/

/**
	This procedure is invoked by the event dispatcher whenever
	standard input becomes readable.  It grabs the next line of
	input characters, adds them to a command being assembled, and
	executes the command if it's complete.

	Results:
		None.

	Side effects:
	Could be almost arbitrary, depending on the command that's
	typed.
*/
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
  CONST84 char *iresult = Tcl_GetStringResult(interp);
  if (iresult != 0 && iresult[0] != 0) { // null or empty string ok
    if ((code != TCL_OK) || (tty)) {
      /*
       * The statement below used to call "printf", but that resulted
       * in core dumps under Solaris 2.3 if the result was very long.
       *
       * NOTE: This probably will not work under Windows either.
       */

      printf("%s\n",iresult);
    }
  }

  /*
   * Output a prompt.
   */

 prompt:
  if (tty) {
    Asc_Prompt(interp, gotPartial);
  }
  Tcl_ResetResult(interp);
}

/**
	Issue a prompt on standard output, or invoke a script
	to issue the prompt.

	Results:
		None.

	Side effects:
	A prompt gets output, and a Tcl script may be evaluated
	in interp.

	Parameters:
	 interp    Interpreter to use for prompting.
	 partial   Non-zero means there already exists a partial
	           command, so use the secondary prompt.
*/
void
Asc_Prompt(Tcl_Interp *interp, int partial)
{
  CONST84 char *promptCmd;
  int code;
  Tcl_Channel outChannel, errChannel;
  CONST84 char *subPrompt;

  color_on(stdout,ASC_FG_GREEN);

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
        Tcl_Write(errChannel, Tcl_GetStringResult(interp), -1);
        Tcl_Write(errChannel, "\n", 1);
      }
      goto defaultPrompt;
    }
  }
  outChannel = Tcl_GetChannel(interp, "stdout", NULL);
  if (outChannel != (Tcl_Channel) NULL) {
    Tcl_Flush(outChannel);
  }
  color_off(stdout);
}

#ifdef __WIN32__
# define HAVE_TCLINT_H
#endif

/* include here to avoid contaminating everything above it. */
#ifdef HAVE_TCLINT_H
# include <tclInt.h>
# define HAVE_TCLGETENV
#endif

/**
preserve key stuff in the launching environment where we can check it later.
*/
static void AscSaveOrgEnv(Tcl_Interp *interp,const char *progname) {
#define ENVCOUNT 8
#define ORGVAR ascOrgEnv
  int i;
  CONST char *value;
  const char *vars[ENVCOUNT] = {
    ASC_ENV_DIST, ASC_ENV_TK, ASC_ENV_BITMAPS, ASC_ENV_LIBRARY, ASC_ENV_SOLVERS,
    "TK_LIBRARY", "TCL_LIBRARY", "PRINTER"
  };
  Tcl_DString buffer;
  Tcl_DString search;
#ifdef ASCTK_DEBUG
  CONSOLE_DEBUG("CACHING ENV Vars.");
#endif

  Tcl_DStringInit(&buffer);
  Tcl_DStringInit(&search);
  ASC_SEND_TO_TCL2(ascOrgEnv, "dummy", "0");
  for (i = 0; i < ENVCOUNT; i++) {
#ifdef HAVE_TCLGETENV
    value = TclGetEnv(vars[i], &search);
#else
	value = getenv(vars[i]);
#endif

    if (value != NULL) {
      ASC_SEND_TO_TCL2(ascOrgEnv, vars[i], value);
#ifdef ASCTK_DEBUG
      CONSOLE_DEBUG("CACHING %s.",vars[i]);
#endif
    }

#ifdef HAVE_TCLGETENV
    Tcl_DStringFree(&search);
#endif
  }
}

#ifdef DEBUG_MALLOC
/**
	Tom Epperly's Malloc Debugger
*/
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
