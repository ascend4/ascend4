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

#ifndef __COMMANDS_H_SEEN__
#define __COMMANDS_H_SEEN__

enum arg_type {
  instance_arg,
  definition_arg,
  id_arg,
  shell_arg /* same as id almost */
};


#define MAX_COMMAND_ARGS 4
/*
 *  The maximum number of arguments a command can have.
 */


extern void InitializeCommands(void);
/*
 *  void InitializeCommands()
 *  Initialize the command list.  This must be called before anything else.
 */


extern void DestroyCommands(void);
/*
 *  void DestroyCommands()
 *  This will destroy the command structure.
 */


extern void AddCommand(int, ...);
/*
 *  void AddCommand(dummy,cmd,func,help,terminate,num_args,arg_def*)
 *  char *cmd,*help;
 *  void (*func)();
 *  int terminate,num_args;
 *  arg_def is a list of enum arg_type's.
 *  This will add a command to the command list.
 *  dummy is used to keep some compilers happy.
 */


extern unsigned long NumberCommands(void);
/*
 *  unsigned long NumberCommands()
 *  Return the number of defined commands.
 */


extern CONST char *CommandName(unsigned long);
/*
 *  Return the commands name.
 */


extern CONST char *CommandHelp(unsigned long);
/*
 *  Return the commands short help message.
 */


extern void CommandFunc(unsigned long int u, void (**func) (/* ??? */));
/*
 *  void CommandFunc(u,func)
 *  unsigned long u;
 *  void (**func)();
 *  Return the commands function.
 */


extern int CommandTerminate(unsigned long);
/*
 *  Return true if the command is a termination command.
 */


extern int CommandNumArgs(unsigned long);
/*
 *  Return the number of arguments
 */


extern enum arg_type CommandArgument(unsigned long, int);
/*
 *  Return the argument type.
 */


extern void CommandArgsPrint(FILE *fp, unsigned long int);
/*
 *  CommandArgsPrint(fp,u);
 *  Print the args expected on fp for command u.
 */


extern void LimitCommand(unsigned long *, unsigned long *, CONST char *, int);
/*
 *  void LimitCommand(lower,upper,str,place)
 *  unsigned long *lower,*upper;
 *  char *str;
 *  int place;
 *  Find the best possible upper and lower bound on the command.  It is
 *  assumed that on entry that the commands between lower and upper match
 *  up to "place" letters of string.  When str cannot be a command,
 *  *lower > *upper on return.  In such cases, the return *lower and *upper
 *  may be outside the original lower and upper.
 */


extern void CompleteCommand(unsigned long, unsigned long, char *, int *);
/*
 *  void CompleteCommand(lower,upper,str,place)
 *  unsigned long lower,upper;
 *  char *str;
 *  int *place;
 *  Fill in all the letters that are shared by commands between lower and
 *  upper.  It is assumed that str has enough space to hold any command.
 */


extern unsigned long FindCommand(CONST char *);
/*
 *  c = FindCommand(string);
 *  Return the number of the command matching string, or 0 if none does.
 *  Exact matches only.
 */

#endif /* __COMMANDS_H_SEEN__ */
