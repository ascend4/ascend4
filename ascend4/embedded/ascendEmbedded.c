
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include "utilities/ascConfig.h"

#ifndef __WIN32__
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <locale.h>
#undef WIN32_LEAN_AND_MEAN
#endif /* __WIN32__ */

#include "embedded/ascendEmbedded.h"

#include "utilities/ascMalloc.h" /* for ascshutdown */
#include "utilities/ascPanic.h"  /* for Asc_Panic */
#include "utilities/ascEnvVar.h"
#include "compiler/compiler.h"
#include "compiler/ascCompiler.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/compiler.h" /* for symchar for units.h */
#include "compiler/units.h"
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/logrel.h"
#include "solver/discrete.h"
#include "solver/mtx.h"
#include "solver/slv_stdcalls.h"

#ifndef lint
static CONST char DriverID[] = "$Id: ascendEmbedded.c,v 1.1 2004/07/13 23:37:15 aw0a Exp $";
#endif

/*
 *  Forward declarations for procedures defined later in this file.
 */
static void AscTrap(int);
/* maybe delete this static int AscProcessCommandLine( int argc, CONST84 char **argv);*/
#ifdef DEBUG_MALLOC
static void InitDebugMalloc(void);
#endif /* DEBUG_MALLOC */
#ifdef __WIN32__
static void setargv(int*, char ***);
#endif /* __WIN32__ */


/**
   Ascend Environment Object (C version)
*/

/**
   Create an Ascend Environment
*/

A4ptr createAscendEnvironment( )
{

  /*
   *  build_name
   *
   *  who built this binary and when
   */

#ifndef TIMESTAMP
  char build_name[]="by anonymous";
#else
  char build_name[]=TIMESTAMP;
#endif /* TIMESTAMP */


  A4ptr ae = NULL;
  ae = (A4ptr) malloc (sizeof (struct AscendInterface));

  ae->g_compiler_timing = 0;
  ae->g_interface_simplify_relations = TRUE;
  ae->g_interfacever = 0;
  ae->relns_flag = 1;
  /*
   *  ASCEND sets g_tty to `1', since we assume ASCEND is always interactive.
   */
  ae->g_tty = 1;

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
   */

  clock();

  /* the next line should NOT be Asc_SignalHandlerPush */
  (void)signal(SIGINT, AscTrap);

#ifdef DEBUG_MALLOC
  InitDebugMalloc();
  ascstatus("Memory status after calling InitDebugMalloc()");
#endif /* DEBUG_MALLOC */

  if ( Asc_CompilerInit(ae->g_interface_simplify_relations) != 0 ) 
  {
    Asc_Panic(2, "Asc_CompilerInit",
              "Insufficient memory to initialize compiler.");
  }

  SlvRegisterStandardClients();
  
#ifdef DEBUG_MALLOC
  ascstatus("Memory status before calling Tk_MainLoop()");
#endif /* DEBUG_MALLOC */


  return ae;

}


/**
   Destroy an Ascend Environment
*/

void destroyAscendEnvironment(A4ptr ae)
{

#ifdef DEBUG_MALLOC
  ascstatus("Memory status when entering destroyAscendAe");
#endif /* DEBUG_MALLOC */

  /*
   *  Do ASCEND Cleanup
   */
  Asc_HelpDestroy();
  Asc_UnitValue(NULL);
  Asc_SolvMemoryCleanup();
  Asc_CompilerDestroy();
  Asc_DestroyAe();
#ifdef DEBUG_MALLOC
  ascshutdown("Memory status just before exiting");
#endif /* DEBUG_MALLOC */

  free((void *) ae);

}



/* 

*/

int readAscendFile(A4ptr ae, 
		   char * fileName)
{

  struct module_t *mod;
  int result;
  int zz_parse();

  if ( fileName == NULL )
  {
    return -1;
  }


  SetParseRelnsFlag(ae->relns_flag);
  if((mod = Asc_OpenModule(fileName, NULL)) == NULL) 
  {
    return -2;
  } 
  else 
  {
    /*
     * the open was successful.  parse the file.
     */
    zz_parse();
    result = 0;
  }

  SetParseRelnsFlag(1);	  /* always reset */
  return result;

}


/*
   Compile an instance of the named model within the loaded file

   @param ae                Self pointer to environment
   @param rootInstanceName  Name to be given to the compiled "root" instance
   @param modelName         Name of model to be compiled 
   @return                  0: all is well

*/





int compileAscendInstance(A4ptr ae,
			  char * simulationName,
			  char * modelName)
{ fix me }

/**
   Run a method on an instance defined within the enviroment
   @param environment       Self pointer to environment
   @param methodName        Name of the method (including path name within 
                            environment
   @return                  0: all is well
*/

int rundAscendMethod(A4ptr ae,
		     char * methodName)
{ fix me }


/**
   Create/Derive a solve system for an instance (which may be a part
   of the root instance) in the Ascend Environment
   @param environment       Self pointer to environment
   @param solverName        Name of the solver to be used when solving
   @param instanceName      Name of instance for which solve system is built
   @return                  0: all is well
*/

SlvSystemPtr createAscendSolverSystem(A4ptr ae,
				  char * solverName,
				  char * instanceName)
{ fix me }


/**
   Destroy a solve system for an instance in the Ascend Environment
   @param environment       Self pointer to environment
   @system                  Name of system to destroy
*/

void destroyAscendSolverSystem(A4ptr ae,
			       SlvSystemPtr system)
{ fix me }



/**
   Solve an Ascend instance
   @SlvSystemPtr            Pointer to the Solve System created for the instance
   @return                  0: all is well
*/

int solveAscendSlvSystemPtr(SlvSystemPtr system)
{ fix me }



int getSolverBooleanResult(SlvSystemPtr system,
			   char * property)
{ fix me }


int setRealValue(A4ptr ae,
		 char * variableName,
		 double value,
		 char * units)
{ fix me }


int getRealValue(A4ptr ae,
		 char * variableName,
		 double * valuePtr,
		 char * units)
{ fix me }


int setIntValue(A4ptr ae,
		char * variableName,
		int value)
{ fix me }


int getIntValue(A4ptr ae,
		char * variableName,
		int * valuePtr)
{ fix me }


int setBooleanValue(A4ptr ae,
		    char * variableName,
		    int value)
{ fix me }


int getBooleanValue(A4ptr ae,
		    char * variableName,
		    int * valuePtr)
{ fix me }


int setSymbolValue(A4ptr ae,
		   char * variableName,
		   int value)
{ fix me }


int getSymbolValue(A4ptr ae,
		   char * variableName,
		   int * valuePtr)
{ fix me }



/* =============================================================== */
/* ======================= setup functions ======================= */
/* =============================================================== */


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

			 
