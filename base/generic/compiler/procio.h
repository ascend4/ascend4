/*
 *  Procedure Output
 *  by Benjamin Allan
 *  Created: 3/12/98
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: procio.h,v $
 *  Date last modified: $Date: 1998/03/17 22:09:21 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
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

/** @file
 *  Procedure Output.
 *  <pre>
 *  When #including procio.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "proc.h"
 *  </pre>
 */

#ifndef ASC_PROCIO_H
#define ASC_PROCIO_H

/**	addtogroup compiler Compiler
	@{
*/

/** Codes for old-style external call messaging. */
enum ProcExtError {
  PE_unloaded,
  PE_nulleval,
  PE_argswrong,
  PE_badarg,
  PE_evalerr
};

/** METHODs equivalent of WSEM. Should go away soon. */
extern void WriteInitWarn(struct procFrame *fm, char *str);
/** METHODs equivalent of WSEM. Should go away soon. */
extern void WriteInitErr(struct procFrame *fm, char *str);

/* error message services */

/**
 * <!--  ProcWriteCaseError(fm,arm,pos);                               -->
 * Write error encountered while evaluating SWITCH.
 * arm gives the number of the case in question. pos
 * gives the position of the error in the var list.
 */
extern void ProcWriteCaseError(struct procFrame *fm, int arm, int pos);

/**
 * <!--  ProcWriteForError(fm);                                        -->
 * Write error encountered while evaluating FOR/DO.
 */
extern void ProcWriteForError(struct procFrame *fm);

/**
 * <!--  ProcWriteAssignmentError(fm);                                 -->
 * Write error encountered while evaluating := assignment.
 */
extern void ProcWriteAssignmentError(struct procFrame *fm);

/**
 * <!--  ProcWriteFixError(fm,var);                                 -->
 * Write error encountered while evaluating var.fixed assignment.
 */
extern void ProcWriteFixError(struct procFrame *fm, CONST struct Name *var);


/**
 * <!--  ProcWriteIfError(fm,cname);                                   -->
 * Write error encountered while evaluating boolean flow control.
 * cname is normally "IF" or "WHILE".
 */
extern void ProcWriteIfError(struct procFrame *fm, CONST char *cname);

/**
 * <!--  ProcWriteRunError(fm);                                        -->
 * Write error encountered while evaluating RUN arguments.
 */
extern void ProcWriteRunError(struct procFrame *fm);

/**
 * <!--  ProcWriteExtError(fm,funcname,err,pos);                       -->
 * Write error encountered while evaluating Ext arguments.
 */
extern void ProcWriteExtError(struct procFrame *fm,
                              CONST char *funcmane,
                              enum ProcExtError err,
                              int pos);

/* backtrace output functions. */

/**
 * Issue a message if we blew the stack limit or are
 * unwinding the stack.
 */
extern void ProcWriteStackCheck(struct procFrame *fm,
                                struct Name *class,
                                struct Name *name);

/**
 * <!--  WriteProcedureBlock(fp,initstack,str);                        -->
 * Writes a line to file fp.  Write format resembles:
 * ("%s %s in %s\n",str,stack->last->proc,stack->last->context)
 * For example: 
 * <pre>
 * WriteProcedureBlock(ASCERR,stack,"Entering ");
 *    --> "Entering METHOD myproc in mysim.myinstance[3]"
 * </pre>
 * str may be empty or NULL.
 * The format is subject to change as this feature is in development.
 */
extern void WriteProcedureBlock(FILE *fp,
                                struct gl_list_t *initstack,
                                CONST char *str);

/**
 * <!--  WriteProcedureLine(fp,initstack,str);                         -->
 * Writes a line to file fp.  Write format resembles:
 * ("%s %d: %s in %s\n",str,line,proc,context)
 * For example:
 * <pre>
 * WriteProcedureLine(ASCERR,initstack,"Executing line ");
 *    --> "Executing line 137 of myprocname in mysim.myinstance[3]"
 * </pre>
 * The format is subject to change as this feature is in development.
 */
extern void WriteProcedureLine(FILE *fp,
                               struct gl_list_t *initstack,
                               CONST char *str);

/* @} */

#endif  /* ASC_PROCIO_H */

