/*
 *  Routines to Describe Interface Commands
 *  by Tom Epperly
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: commands.c,v $
 *  Date last modified: $Date: 1997/07/29 18:29:26 $
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

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/commands.h"


#ifndef lint
static CONST char CommandRCSid[] = "$Id: commands.c,v 1.8 1997/07/29 18:29:26 mthomas Exp $";
#endif /* lint */

struct command_t {
  CONST char *str,*help;
  void (*func)();
  int terminate;
  int num_args;
  enum arg_type args[MAX_COMMAND_ARGS];
};

struct gl_list_t *global_command_list=NULL;

extern void InitializeCommands(void)
{
  if (global_command_list == NULL) {
    global_command_list = gl_create(25L);
  } else {
    FPRINTF(stderr,"InitializeCommands has already been called.\n");
  }
}


extern void DestroyCommands(void)
{
  if (global_command_list) {
    gl_free_and_destroy(global_command_list);
    global_command_list=NULL;
  }
}


static
int CmpCommand(struct command_t *ptr1, struct command_t *ptr2)
{
  assert(ptr1&&ptr2);
  return strcmp(ptr1->str,ptr2->str);
}


extern void AddCommand(int dummy, ...)
{
  va_list pvar;
  struct command_t *ptr;
  int c = 0;
  ptr = (struct command_t *)ascmalloc(sizeof(struct command_t));
  va_start(pvar,dummy);

  ptr->str = va_arg(pvar,CONST char *);
  ptr->func = (void (*)())va_arg(pvar,VOIDPTR);
  ptr->help = va_arg(pvar,CONST char *);
  ptr->terminate = va_arg(pvar,int);
  ptr->num_args = va_arg(pvar,int);
  if (ptr->num_args > MAX_COMMAND_ARGS) {
    FPRINTF(stderr,"AddCommand called with %d args > Max of %d\n",
	    ptr->num_args, MAX_COMMAND_ARGS);
    FPRINTF(stderr,"There are too many arguments to command %s\n",ptr->str);
    ascfree((char *)ptr);
    ptr = NULL;
  } else {
    while(c<ptr->num_args) {
      ptr->args[c++] = va_arg(pvar,enum arg_type);
    }
    gl_insert_sorted(global_command_list,ptr,(CmpFunc)CmpCommand);
  }
  va_end(pvar);
}


extern unsigned long NumberCommands(void)
{
  return gl_length(global_command_list);
}


extern CONST char *CommandName(unsigned long int u)
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  return ptr->str;
}


extern CONST char *CommandHelp(unsigned long int u)
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  return ptr->help;
}


extern void CommandFunc(unsigned long int u, void (**func) (/* ??? */))
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  *func = ptr->func;
}


extern int CommandTerminate(unsigned long int u)
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  return ptr->terminate;
}


extern int CommandNumArgs(unsigned long int u)
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  return ptr->num_args;
}


extern enum arg_type CommandArgument(unsigned long int u, int i)
{
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  return ptr->args[i];
}


extern void CommandArgsPrint(FILE *fp, unsigned long int u)
{
  register struct command_t *ptr;
  int i;
  ptr = (struct command_t *)gl_fetch(global_command_list,u);
  FPRINTF(fp,"%s",ptr->str);
  if (ptr->num_args == 0) {
    FPRINTF(fp," takes no arguments\n");
  } else {
    for (i=0; i < ptr->num_args ; i++) {
      FPRINTF(fp," <");
      switch (ptr->args[i]) {
      case instance_arg:
        FPRINTF(fp,"qualified instance name");
        break;
      case definition_arg:
        FPRINTF(fp,"type name");
        break;
      case id_arg:
        FPRINTF(fp,"simple name");
        break;
      case shell_arg:
        FPRINTF(fp,"stuff to send to unix");
        break;
      default:
        FPRINTF(fp,"UNDEFINED");
        break;
      }
      FPRINTF(fp,">");
    }
    FPRINTF(fp,"\n");
  }
}


static
int mycmp(register CONST char *str1,
	  register CONST char *str2)
{
  while(*str1 != '\0') {
    if (*str1 < *str2) return -1;
    if (*str1 > *str2) return 1;
    str1++;
    str2++;
  }
  return 0;
}


/*
 *  Find the greatist lower bound for a command string that could match
 *  str.
 */
static
unsigned long FindGLB(CONST char *str, int place,
		      register unsigned long int lower,
		      register unsigned long int upper)
{
  register struct command_t *ptr;
  unsigned long search;
  str += place;
  while (lower <= upper){
    search = (lower+upper)/2;
    ptr = (struct command_t *)gl_fetch(global_command_list,search);
    switch(mycmp(str,ptr->str+place)){
    case 0:
      do {
	if (--search < lower) return lower;
	ptr = (struct command_t *)gl_fetch(global_command_list,search);
      } while(mycmp(str,ptr->str+place)==0);
      return search+1;
    case -1: upper = search-1; break;
    case 1: lower = search+1; break;
    }
  }
  return lower;
}


/*
 *  Find the least upper bound for a command string that could match
 *  str.
 */
static
unsigned long FindLUB(CONST char *str, int place,
		      register unsigned long int lower,
		      register unsigned long int upper)
{
  register struct command_t *ptr;
  unsigned long search;
  str += place;
  while (lower <= upper){
    search = (lower+upper)/2;
    ptr = (struct command_t *)gl_fetch(global_command_list,search);
    switch(mycmp(str,ptr->str+place)){
    case 0:
      do {
	if (++search > upper) return upper;
	ptr = (struct command_t *)gl_fetch(global_command_list,search);
      } while(mycmp(str,ptr->str+place)==0);
      return search-1;
    case -1: upper = search-1; break;
    case 1: lower = search+1; break;
    }
  }
  return upper;
}


extern void LimitCommand(unsigned long int *lower, unsigned long int *upper,
                         CONST char *str, int place)
{
  register struct command_t *ptr;
  if (*lower > *upper) return;
  if (*lower == *upper){
    ptr = (struct command_t *)gl_fetch(global_command_list,*lower);
    if (mycmp(str+place,ptr->str+place))
      (*upper)--;
  }
  else{
    *lower = FindGLB(str,place,*lower,*upper);
    *upper = FindLUB(str,place,*lower,*upper);
  }
}


static
int NumberMatch(register CONST char *str1,
		register CONST char *str2, int max)
{
  register int c=0;
  while((c<max)&&(*str1 != '\0')&&(*str1 == *str2)){
    str1++;
    str2++;
    c++;
  }
  return c;
}


extern void CompleteCommand(unsigned long int lower, unsigned long int upper,
                            char *str, int *place)
{
  register CONST char *cpy;
  int count;
  unsigned long pos;
  register struct command_t *ptr;
  ptr = (struct command_t *)gl_fetch(global_command_list,lower);
  cpy = ptr->str;
  count = (int)strlen(cpy);
  for(pos=lower+1;pos<=upper;pos++){
    ptr = (struct command_t *)gl_fetch(global_command_list,pos);
    count = NumberMatch(cpy,ptr->str,count);
  }
  strncpy(str,cpy,count);
  str[count] = '\0';
  *place = count;
}


extern unsigned long int FindCommand(CONST char *s)
{
  struct gl_list_t *clist;
  register struct command_t *ptr;
  unsigned long pos,len;
  clist = global_command_list;
  if (clist == NULL || s==NULL) return 0;
  len = gl_length(clist);
  for (pos=1; pos < len; pos++) {
    ptr = (struct command_t *)gl_fetch(global_command_list,pos);
    if (strcmp(s,ptr->str)==0) return pos;
  }
  return 0L;
}
