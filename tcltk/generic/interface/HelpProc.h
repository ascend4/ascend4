/*
 *  HelpProc.h
 *  by Ben Allan          
 *  Created: 4/97
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: HelpProc.h,v $
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

/** @file
 *  Functions & macros for the Ascend Tcl/Tk help system.
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASCTK_HELPPROC_H
#define ASCTK_HELPPROC_H

#include <tcl.h>
#include "config.h"

/**
 * The largest number of strings that will be used in building a long
 * help string.
 */
#define MAXHELPARGS 100
/** Token for end of help strings. */
#define HLFSTOP (char *)NULL

/**
 *  <!--  macro ASCUSE                                                 -->
 *  This macro should be inserted at the beginning of all existing
 *  and future ascend callbacks registered in TCL.
 *  It checks for the command option -help and returns the helpstring
 *  with the code TCL_OK if -help is found. If -help is not found,
 *  this does nothing.
 *  All ASCEND callbacks should be written using the standard arguments
 *  ClientData cdata, Tcl_Interp *interp, int argc, char *argv[].
 *  This macro insists on it.
 */
#define ASCUSE \
  if (Asc_HelpCheck(cdata,interp,argc,argv) != TCL_OK) { return TCL_OK; }

/**
 * Prototype for a function which returns a char *
 * containing the long form help for a command.
 * provide: char *myfunc(void) to HelpDefine
 * The returned char * should be created with ascmalloc, because we
 * will ascfree it eventually.
 */
typedef char *(*HLFunc)(void);

/**
 * Defines a help function using an arbitrary number of strings.
 *
 * To get past the string literal limits of compilers,
 * use the following macro in your .c file like so:
 * <pre>
 * STDHLF(yourCmd,(your,list,of,macro,strings,HLFSTOP)) \* ; *\
 * </pre>
 * Then you can pass yourCmdHLF into the registration of your command.
 * Generally, each option of a command with many options should get
 * its own macro string. The ANSI limit on such strings is 509 chars.
 * Use STDHLF_H(yourCmd); in your header file to define your function.<br><br>
 *
 * This macro is evil because it uses nonstandard-looking syntax
 * to get past the C preprocessor without errors. (Missing ; in use and
 * missing () in definition).
 */
#define STDHLF(baseName,list_of_strings_in_parens_end_in_null) \
char * baseName##HLF(void) \
{ \
  char *result; \
  result = HelpBuildString list_of_strings_in_parens_end_in_null ; \
  return result; \
} extern int g_does_not_exist_so_do_not_use /* hack to use a ; after\} */

/**
 * Macro to make a header declaration for your function made with HLF.
 */
#define STDHLF_H(baseName) extern char *baseName##HLF(void)

extern char *HelpBuildString(char *first, ...);
/**<
 * <!--  HelpBuildString(char *, variable_number_of_args,NULL);        -->
 * Builds a long string from an arbitrary number of input strings
 * so long as the last string is NULL and the first is not.
 * All arguments must be strings (char *).
 * The string returned is the callers responsibility to free.
 * Up to the first MAXHELPARGS are used in the returned string.
 */

extern int Asc_HelpCheck(ClientData cdata, Tcl_Interp *interp, 
                         int argc, CONST84 char **argv);
/**<
 * <!--  Asc_HelpCheck(cdata,interp,argc,argv);                        -->
 * Returns TCL_BREAK if a help message is printed to the interpreter
 * or TCL_OK if not.
 * If this functions returns TCL_BREAK, the caller should return.
 */

extern int Asc_HelpInit(void);
/**<
 *  Initializes the help data structures so that commands can then
 *  be registered.
 */

extern int Asc_HelpDestroy(void);
/**<
 *  Destroys the help data structures. This should only be called at
 *  shutdown.
 */

extern int Asc_HelpDefineGroup(CONST char *froup, CONST char *explanation);
/**<
 *  <!--  Asc_HelpDefineGroup(group,explanation)                       -->
 *  <!--  CONST char *group, *explanation;                             -->
 *  Defines the generic description for the command group given.
 *  explanation should start with Explanation:
 *  group should not contain whitespace.
 *  Follows tcl return conventions.
 */

extern int Asc_HelpDefine(CONST char *name, 
                          CONST char *group, 
                          CONST char *usage,
                          CONST char *desc, 
                          HLFunc longfunc);
/**<
 *  <!--  Asc_HelpDefine(name,group,usage,desc,longfunc)               -->
 *  <!--  CONST char *name, *group, *usage, *desc;                     -->
 *  <!--  char * HLFunc(void);                                         -->
 *  Defines the help strings for the command name given.
 *
 *  All the strings given should be static strings, because we may
 *  store pointers to them.
 *  HLFunc should return a string that we become the owner of, or NULL.
 *  All 5 parameters should be coming via defines in the
 *  C header files for the function being documented.
 *  See Asc_HelpCmd() for an example of same.<br><br>
 *
 *  If you have not written up someplace else the information that
 *  is required here, you obviously have not thought the command you
 *  are registering through and you should not be registering it.
 *
 *  @param name     The tcl registration string.
 *  @param group    The command group name.  It should not contain whitespace.
 *  @param usage    The command syntax by example or in man page style.
 *  @param desc     The basic one-line description of the command. (<= 70 char)
 *  @param longfunc Function returning the detailed explanation of the command.
 */

extern CONST char *Asc_HelpGetShort(Tcl_Interp *interp, CONST84 char *commandname);
/**<
 *  <!--  Asc_HelpGetShort(interp,commandname)                         -->
 *  Returns the short string for the command given, if there is one.
 *  If interp is not NULL, also appends the returned string to the
 *  interpreter's result string.
 */

extern CONST char *Asc_HelpGetLong(Tcl_Interp *interp, CONST84 char *commandname);
/**<
 *  <!--  Asc_HelpGetLong(interp,commandname)                          -->
 *  Returns the long string for the command given, if there is one.
 *  If interp is not NULL, also appends the returned string to the
 *  interpreter's result string.
 */

extern CONST char *Asc_HelpGetUsage(Tcl_Interp *interp, CONST84 char *commandname);
/**<
 *  <!--  Asc_HelpGetUsage(interp,commandname)                         -->
 *  Returns the usage string for the command given, if there is one.
 *  If interp is not NULL, also appends the returned string to the
 *  interpreter's result string.
 */

extern int Asc_HelpGetGroup(Tcl_Interp *interp, CONST84 char *groupname);
/**<
 *  <!--  Asc_HelpGetGroup(interp,groupname)                           -->
 *  Returns the members of a group, and the group explanation
 *  if there is one, in the interpreter.
 *  All is in a list format and the explanation is first
 *  if it exists. If group does not exist, returns an error message.
 *  By use convention: the group explanation is a list element which
 *  starts with "Explanation".
 *  The return code follows TCL conventions.
 */

extern int Asc_HelpCommandGroups(Tcl_Interp *interp);
/**<
 *  <!--  Asc_HelpCommandGroups(interp)                                -->
 *  Appends the complete list of groups to the interpreter.
 *  Commands are sorted in some reasonable fashion.
 *  The return code follows TCL conventions.
 */

extern int Asc_HelpCommandList(Tcl_Interp *interp);
/**<
 *  <!--  Asc_HelpCommandList(interp)                                  -->
 *  Appends the complete list of commands to the interpreter.
 *  Commands are sorted in alphabetical order.
 *  The return code follows TCL conventions.
 */

extern int Asc_HelpCommandsByGroups(Tcl_Interp *interp);
/**<
 *  <!--  Asc_HelpCommandsByGroups(interp)                             -->
 *  Appends the complete list of commands to the interpreter.
 *  Groups are sorted and then commands are sorted in alphabetical order.
 *  The return code follows TCL conventions.
 */

STDHLF_H(Asc_HelpCmd);
/**<  Function returning our long help string. */

extern int Asc_HelpCmd(ClientData cdata, Tcl_Interp *interp, 
                       int arcg, CONST84 char **argv);
/**<
 * <!--  Asc_HelpCmd(cdata,interp,argc,argv);                          -->
 * This is the tcl callback for our commandline help facility.
 */

/** Registered as */
#define Asc_HelpCmdHN "help"
/** Usage */
#define Asc_HelpCmdHU \
  "help [commandname] OR help help"
/** Short help text */
#define Asc_HelpCmdHS \
  "returns information on C functions ASCEND defines as Tcl commands"
/** Long help text part 1. */
#define Asc_HelpCmdHL1 \
"\
 * Returns help on a registered command name, or command group name,\n\
 * or the list of commands alphabetized or the list groups alphabetized,\n\
 * or the list of commands alphabetized by groups.\n\
 * For help on specific commands you can also try: <commandname> -h\n\
 * Most of our callbacks supply their own help.\n\
"
/** Long help text part 2. */
#define Asc_HelpCmdHL2 \
"\
 *\n\
 * help examples:\n\
 * help -h         returns a short definition & UNIX man page style syntax.\n\
 * help -H         returns a long explanation\n\
 * help help       returns this text.\n\
"
/** Long help text part 3. */
#define Asc_HelpCmdHL3 \
"\
 * help system     returns the description of the system group.\n\
 * help groups     returns the alphabetized list of group names.\n\
 * help all        returns the alphabetized list of commands.\n\
 * help commands   returns the alphabetized listing of groups.\n\
 *\n\
"
/** Long help text part 4. */
#define Asc_HelpCmdHL4 \
"\
 * All output is to the tcl interpreter in the form of list elements.\n\
 * For better formatted help output, use Help which has the same\n\
 * syntax as this command but writes the output more neatly to stdout.\n\
"
/*
 * If we follow the style above for all our tcl callback headers we
 * end up with better documented tcl callbacks in C and consistent
 * runtime help.
 * The pattern is to define 3 macros in the pattern
 * nameHN, nameHU, nameHS, and function nameHLF via the macro STDHLF
 * above.
 * In this example name is Asc_HelpCmd.
 */

#endif  /* ASCTK_HELPPROC_H */

