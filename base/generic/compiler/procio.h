/** 
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

/** 
 *  When #including procio.h, make sure these files are #included first:
 *         #include "compiler.h"
 *         #include "proc.h"
 */


#ifndef __PROCIO_H_SEEN__
#define __PROCIO_H_SEEN__
/** requires
# #include<stdio.h>
# #include"proc.h"
*/

/** codes for old-style external call messaging */
enum ProcExtError {
  PE_unloaded,
  PE_nulleval,
  PE_argswrong,
  PE_badarg,
  PE_evalerr
};

/** METHODs equivalent of WSEM. should go away soon. */
extern  void WriteInitWarn(struct procFrame *, char *);
extern  void WriteInitErr(struct procFrame *, char *);

/** error message services **/
/** 
 * ProcWriteCaseError(fm,arm,pos);
 * write error encountered while evaluating SWITCH.
 * arm gives the number of the case in question. pos
 * gives the position of the error in the var list.
 */
extern void ProcWriteCaseError(struct procFrame *, int, int);

/** 
 * ProcWriteForError(fm);
 * write error encountered while evaluating FOR/DO.
 */
extern void ProcWriteForError(struct procFrame *);

/** 
 * ProcWriteAssignmentError(fm);
 * write error encountered while evaluating := assignment.
 */
extern void ProcWriteAssignmentError(struct procFrame *);

/** 
 * ProcWriteIfError(fm,cname);
 * write error encountered while evaluating boolean flow control.
 * cname is normally "IF" or "WHILE".
 */
extern void ProcWriteIfError(struct procFrame *, CONST char *);

/** 
 * ProcWriteRunError(fm);
 * write error encountered while evaluating RUN arguments.
 */
extern void ProcWriteRunError(struct procFrame *);

/** 
 * ProcWriteExtError(fm,funcname,err,pos);
 */
extern void ProcWriteExtError(struct procFrame *, CONST char *,
                              enum ProcExtError, int);


/** backtrace output functions. **/

/** 
 * Issue a message if we blew the stack limit or are
 * unwinding the stack.
 */
extern void ProcWriteStackCheck(struct procFrame *,
                                struct Name *, struct Name *);

/** 
 * WriteProcedureBlock(fp,initstack,str);
 * Writes a line to file fp in a format resembling:
 * ("%s %s in %s\n",str,stack->last->proc,stack->last->context)
 * For example: WriteProcedureBlock(ASCERR,stack,"Entering ");
 * --> "Entering METHOD myproc in mysim.myinstance[3]"
 * str may be empty or NULL.
 * The format is subject to change as this feature is in development.
 */
extern void WriteProcedureBlock(FILE *, struct gl_list_t *initstack,
                                CONST char *);

/** 
 * WriteProcedureLine(fp,initstack,str);
 * Writes a line to file fp in a format resembling:
 * ("%s %d: %s in %s\n",str,line,proc,context)
 * For example: WriteProcedureLine(ASCERR,initstack,"Executing line ");
 * --> "Executing line 137 of myprocname in mysim.myinstance[3]"
 * The format is subject to change as this feature is in development.
 */
extern void WriteProcedureLine(FILE *, struct gl_list_t *initstack,
                               CONST char *);
#endif /** __PROCIO_H_SEEN__ */

