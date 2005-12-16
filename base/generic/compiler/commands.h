/*
 *  Routines to Describe Interface Commands
 *  by Tom Epperly
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: commands.h,v $
 *  Date last modified: $Date: 1997/07/29 18:29:27 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
 */

/** @file
 *  Routines to Describe Interface Commands.
 *  <pre>
 *  When #including commands.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __commands_h_seen__
#define __commands_h_seen__

/** Command argument types. */
enum arg_type {
  instance_arg,
  definition_arg,
  id_arg,
  shell_arg   /**< same as id almost */
};

#define MAX_COMMAND_ARGS 4
/**< The maximum number of arguments a command can have. */

extern void InitializeCommands(void);
/**<
 *  Initializes the command list.  
 *  This function must be called before anything else in this module.
 *  It is a logical error to call this function more than once, but
 *  will be tolerated.  Call DestroyCommands() when finished using 
 *  the command facilities.
 */

extern void DestroyCommands(void);
/**<  Destroys the command structure created by InitializeCommands(). */

typedef void (*InterfaceCommandF) (void);
/**<  Type for command functions added using AddCommand(). */

extern void AddCommand(int dummy, ...);
/**<
 *  Adds a command to the command list.
 *  The arguments expected are as follows:
 *    - dummy       not used (present to keep some compilers happy)
 *    - cmd         the command string (char *)
 *    - func        the command's function (InterfaceCommandF)
 *    - help        help string for the command (char *)
 *    - terminate   Non-zero if the command is a terminate command, 0 if not (int)
 *    - num_args    number of arguments to follow (int)
 *    - arg_def     list of arguments (enum arg_type)
 */

extern unsigned long NumberCommands(void);
/**<  Returns the number of defined commands. */

extern CONST char *CommandName(unsigned long cmd_number);
/**<
 *  Returns the name of a command.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().
 */

extern CONST char *CommandHelp(unsigned long cmd_number);
/**<
 *  Returns the short help message of a command.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().
 */

extern void CommandFunc(unsigned long int cmd_number, InterfaceCommandF *func);
/**<
 *  Returns the function of a command.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().  func is the address where the pointer to 
 *  the command's function will be stored.
 */

extern int CommandTerminate(unsigned long cmd_number);
/**<
 *  Returns non-zero if a command is a termination command.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().
 */

extern int CommandNumArgs(unsigned long cmd_number);
/**<
 *  Returns the number of arguments for a command.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().
 */

extern enum arg_type CommandArgument(unsigned long cmd_number, int n);
/**<
 *  Returns the type of the n'th argument for a command.  
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().  n ranges from 0 to CommandNumArgs(pos)-1.
 */

extern void CommandArgsPrint(FILE *fp, unsigned long int cmd_number);
/**<
 *  Prints the args expected for a command on fp.
 *  The cmd_number must be in the range (1 .. NumberCommands()), and may be
 *  looked up using FindCommand().
 */

extern void LimitCommand(unsigned long *lower,
                         unsigned long *upper,
                         CONST char *str,
                         int place);
/**<
 *  Finds the best possible upper and lower bound on the command.  It is
 *  assumed that on entry that the commands between lower and upper match
 *  up to "place" letters of string.  When str cannot be a command,
 *  *lower > *upper on return.  In such cases, the return *lower and *upper
 *  may be outside the original lower and upper.
 */

extern void CompleteCommand(unsigned long lower,
                            unsigned long upper,
                            char *str,
                            int *place);
/**<
 *  Fills in all the letters that are shared by commands between lower and
 *  upper.  It is assumed that str has enough space to hold any command.
 */

extern unsigned long FindCommand(CONST char *string);
/**<
 *  Returns the number of the command matching string, or 0 if none does.
 *  Exact matches only.
 */

#endif  /* __commands_h_seen__ */

