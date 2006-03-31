/*
 *  Temporary Procedure Output
 *  by Tom Epperly
 *  Created: 1/10/90
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: procio.c,v $
 *  Date last modified: $Date: 1998/05/12 19:57:43 $
 *  Last modified by: $Author: ballan $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#include<math.h>
#include<stdio.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include <general/dstring.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "stattypes.h"
#include "statement.h"
#include "slist.h"
#include "statio.h"
#include "instance_enum.h"
#include "instance_io.h"
#include "nameio.h"
#include "module.h"
#include "proc.h"
#include "watchpt.h"
#include "procframe.h"
#include "initialize.h"
#include "procio.h"

#ifndef lint
static CONST char ProcedureIORCSid[] = "$Id: procio.c,v 1.11 1998/05/12 19:57:43 ballan Exp $";
#endif


void WriteInitWarn(struct procFrame *fm, char *str)
{
  CONSOLE_DEBUG("...");
  WriteStatementErrorMessage(fm->err, fm->stat, str, 0,2);
}

void WriteInitErr(struct procFrame *fm, char *str)
{
  WSEM(fm->err,fm->stat,str);
  FFLUSH(fm->err);
}

void ProcWriteCaseError(struct procFrame *fm, int arm, int pos)
{
  static char nostr[] = "";
  char *fmt;
  char *tail;
  char armstr[20];
  char errmsg[100];
  
  sprintf(armstr,"in arm %d.",arm);

  switch (fm->ErrNo) {
  case Proc_instance_not_found:
    fmt = "NULL (as yet unmade) instance for SWITCH argument %d %s";
    tail = nostr;
    break;
  case Proc_name_not_found:
    fmt = "Undefined name for SWITCH argument %d %s";
    tail = nostr;
    break;
  case Proc_illegal_name_use:
    fmt = "Incorrect name (subscript?) for SWITCH argument %d %s";
    tail = nostr;
    break;
  case Proc_CallError:
    fmt = "Unexpected 'OK' message for SWITCH argument %d %s";
    tail = nostr;
    break;
  case Proc_case_undefined_value:
    fmt = "Variable %d value UNDEFINED in SWITCH %s";
    tail = nostr;
    break;
  case Proc_case_boolean_mismatch:
    fmt = "Nonboolean instance for argument %d %s";
    tail = armstr;
    break;
  case Proc_case_integer_mismatch:
    fmt = "Noninteger instance for argument %d %s";
    tail = armstr;
    break;
  case Proc_case_symbol_mismatch:
    fmt = "Nonsymbol instance for argument %d %s";
    tail = armstr;
    break;
  case Proc_case_wrong_index:
    fmt = "Incorrect index expression in AnalyzeSwitchCase";
    tail = nostr;
    break;
  case Proc_case_wrong_value:
    fmt = "Wrong value expression in AnalyzeSwitchCase";
    tail = nostr;
    break;
  case Proc_case_extra_values:
    fmt = "Multiple instances for a variable in a SWITCH statement";
    tail = nostr;
    break;
  default:
    fmt = "Confusion in ProcWriteCaseError, %d %s";
    tail = armstr;
    break;
  }
  sprintf(errmsg,fmt,pos,tail);
  WriteInitErr(fm,errmsg);
}

void ProcWriteIfError(struct procFrame *fm, CONST char *cname)
{
  char em[85];
  char cn[20];

  CONSOLE_DEBUG("...");

  if (strlen(cname) > 19) {
    strncpy(cn,cname,19);
    cn[19] = '\0';
  } else {
    strcpy(cn,cname);
  }
  switch (fm->ErrNo) {
  case Proc_if_expr_error_emptyintersection:
    sprintf(em,"%s expression - empty set intersection",cn); 
    break;
  case Proc_if_expr_error_emptychoice:
    sprintf(em,"%s expression - CHOICE on empty set",cn); 
    break;
  case Proc_if_expr_error_dimensionconflict:
    sprintf(em,"%s expression - real dimensionality conflict",cn); 
    break;
  case Proc_if_expr_error_undefinedvalue:
    sprintf(em,"%s expression - unassigned variable value",cn);
    break;
  case Proc_if_expr_error_incorrectname:
    sprintf(em,"%s expression - name of impossible variable",cn);
    break;
  case Proc_if_expr_error_typeconflict:
    sprintf(em,"%s expression - type conflict of operands",cn);
    break;
  case Proc_if_expr_error_nameunfound:
    sprintf(em,"%s expression - variable not found",cn);
    break;
  case Proc_if_expr_error_confused:
    sprintf(em,"evaluating confusing %s expression",cn);
    break;
  case Proc_if_real_expr:
    sprintf(em,"%s (expression) : real valued expression illegal",cn);
    break;
  case Proc_if_integer_expr:
    sprintf(em,"%s (expression) : integer valued expression illegal",cn);
    break;
  case Proc_if_symbol_expr:
    sprintf(em,"%s (expression) : symbol valued expression illegal",cn);
    break;
  case Proc_if_set_expr:
    sprintf(em,"%s (expression) : set valued expression illegal",cn);
    break;
  case Proc_if_not_logical:
    sprintf(em,"%s (expression) : expression is not boolean-valued",cn);
    break;
  case Proc_infinite_loop:
    sprintf(em,"%s (expression) : looping infinitely?",cn);
    break;
  case Proc_stop:
    sprintf(em,"Found %s statement in METHOD",cn);
    break;
  default:
    sprintf(em,"%s unexpected error message",cn);
    break;
  }
  CONSOLE_DEBUG("...");
  WriteInitErr(fm,em);
  CONSOLE_DEBUG("...");
}

void ProcWriteAssignmentError(struct procFrame *fm)
{
  switch (fm->ErrNo) {
  case Proc_nonatom_assignment:
    WriteInitErr(fm,"Assignment to a non-atomic instance");
    break;
  case Proc_noninteger_assignment:
    WriteInitErr(fm,"Right hand side of assignment is not an integer");
    break;
  case Proc_declarative_constant_assignment:
    WriteInitErr(fm, "Assignment to a set or constant instance");
    break;
  case Proc_nonconsistent_assignment:
    WriteInitErr(fm,"Dimensionally inconsistent assignment");
    break;
  case Proc_nonreal_assignment:
    WriteInitErr(fm, "Right hand side of assignment is not a real expression");
    break;
  case Proc_nonboolean_assignment:
    WriteInitErr(fm,"Right hand side of assignment is not a boolean");
    break;
  case Proc_nonsymbol_assignment:
    WriteInitErr(fm,"Right hand side of assignment is not a symbol");
    break;
  case Proc_nonsense_assignment:
    WriteInitErr(fm,"Assignment to bogus instance type.");
    break;
  case Proc_rhs_error:
    WriteInitErr(fm,"Error evaluating assignment right hand side");
    break;
  case Proc_lhs_error:
    WriteInitErr(fm,"Undefined or NULL instance in left hand side of :=.");
    break;
  default:
    WriteInitErr(fm,"Assignment (:=) unexpected error message"); 
    break;
  }
}

void ProcWriteForError(struct procFrame *fm)
{
  switch (fm->ErrNo) {
  case Proc_for_duplicate_index:
    WriteInitErr(fm,"FOR/DO uses duplicate index variable.");
    break;
  case Proc_for_set_err:
    WriteInitErr(fm,"Error evaluating FOR/DO index set.");
    break;
  case Proc_for_not_set:
    WriteInitErr(fm,"FOR/DO index expression is not a set");
    break;
  default:
    WriteInitErr(fm,"FOR/DO unexpected error message."); 
    break;
  }
}

/* error messages for oldstyle external functions */
void ProcWriteExtError(struct procFrame *fm, CONST char *funcname,
                       enum ProcExtError peerr, int pos)
{
  char *errmsg;
  assert(funcname != NULL);
  errmsg = (char *)ascmalloc(80+strlen(funcname));
  switch (peerr) {
  case PE_unloaded:
    WriteInitErr(fm,"External function has not been loaded.");
    break;
  case PE_nulleval:
    WriteInitErr(fm,"Nonexistent Evaluation for old-style external function.");
    break;
  case PE_argswrong:
    WriteInitErr(fm,"Incorrect arguments to old-style external function.");
    break;
  case PE_badarg:
    switch (fm->ErrNo) {
    case Proc_instance_not_found:
      sprintf(errmsg,
        "EXTERNAL %s: NULL (as yet unmade) instance for argument %d.",
        funcname,pos);
      WriteStatementErrorMessage(fm->err,fm->stat,errmsg, 0,3);
      break;
    case Proc_name_not_found:
      sprintf(errmsg,"EXTERNAL %s: Undefined name for argument %d.",
              funcname, pos);
      WriteStatementErrorMessage(fm->err,fm->stat,errmsg, 0,3);
      break;
    case Proc_illegal_name_use:
      sprintf(errmsg,
              "EXTERNAL %s: Incorrect name (subscript?) for argument %d.",
              funcname, pos);
      WriteStatementErrorMessage(fm->err,fm->stat,errmsg, 0,3);
      break;
    case Proc_bad_name:
      sprintf(errmsg,"EXTERNAL %s: Unknown error message for argument %u.",
              funcname, pos);
      WriteStatementErrorMessage(fm->err,fm->stat,errmsg, 0,3);
      FPRINTF(fm->err,"  Expect crash soon!\n");
      break;
    case Proc_CallError:
        sprintf(errmsg,"EXTERNAL %s: Unexpected 'OK' message for argument %u.",
                funcname, pos);
        WriteStatementErrorMessage(fm->err,fm->stat,errmsg, 0,3);
        fm->ErrNo = Proc_CallError;
    default:
      break;
    }
    break;
  case PE_evalerr:
    WriteInitErr(fm,"Error in evaluating external function.");
    break;
  }
  ascfree(errmsg);
}
  
void ProcWriteStackCheck(struct procFrame *fm,
                         struct Name *class, struct Name *name)
{
  int unwind = 0;
  if ( fm->ErrNo == Proc_return) {
    return;
  }
  if(fm->stat != NULL){
    error_reporter_start(ASC_PROG_ERROR,SCP(Asc_ModuleBestName(StatementModule(fm->stat))),StatementLineNum(fm->stat),NULL);
  }else{
	error_reporter_start(ASC_PROG_ERROR,NULL,0,NULL);
  }

  if (fm->ErrNo == Proc_stack_exceeded_this_frame) {
    /* stack error message not suppressible */
    unwind = 1;
    FPRINTF(fm->err,
       "Initialization stack overflow (possible recursion) in call to\n"); 
  } else { 
    if (fm->gen & WP_BTUIFSTOP || fm->ErrNo == Proc_stack_exceeded) {
      unwind = 1;
      FPRINTF(fm->err,"  In call to"); 
    }
  } 
  if (!unwind) {
    return;
  }
  FPRINTF(fm->err," METHOD ");
  if (class != NULL) {
    WriteName(fm->err,class);
    FPRINTF(fm->err,"::");
  }
  WriteName(fm->err,name);
  FPRINTF(fm->err," (depth %d) in instance %s\n", fm->depth, fm->cname);

  error_reporter_end_flush();
}

void ProcWriteRunError(struct procFrame *fm) 
{
  char *errmsg;
  errmsg = "Unexpected RUN statement error";
  switch (fm->ErrNo) {
  case Proc_bad_name:
    errmsg = "Bad method name in RUN statement";
    break;
  case Proc_proc_not_found:
    errmsg = "Method not found in RUN statement";
    break;
  case Proc_illegal_name_use:
    errmsg = "Illegal name use in RUN statement";
    break;
  case Proc_type_not_found:
    errmsg = "Type name not found in RUN statement";
    break;
  case Proc_illegal_type_use:
    errmsg = "Unconformable types in RUN statement";
    break;
  case Proc_stack_exceeded_this_frame:
  case Proc_stack_exceeded:
    errmsg = "METHOD call too deeply nested in statement";
    break;
  default:
    errmsg = "Type name not found in RUN statement";
    break;
  }
  WriteInitErr(fm,errmsg);
}

void ProcWriteFixError(struct procFrame *fm, struct Name *var){
	char errmsg[255];
	char *name;
	name = WriteNameString(var);
	strcpy(errmsg,"Unexpected FIX statement error");
	switch(fm->ErrNo){
	case  Proc_type_not_found:
		strcpy(errmsg, "Bad setup for FIX statement (Is 'solver_var' present in the library?)");
		break;
	case Proc_illegal_type_use:
		strcpy(errmsg, "Incorrect type for variable being fixed (must be a refined solver_var)");
		break;
	case Proc_bad_name:
		strcpy(errmsg, "Unknown variable in FIX statement");
		break;
	}
	strcat(errmsg,", for variable '");
	strncat(errmsg,name,40);
	strcat(errmsg,"'");
	ascfree(name);
	WriteInitErr(fm,errmsg);
}

