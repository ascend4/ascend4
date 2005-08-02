/*
 *  Permanent Statement Output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.14 $
 *  Version control file: $RCSfile: statio.h,v $
 *  Date last modified: $Date: 1997/07/28 20:52:13 $
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

/** @file
 *  Permanent Statement Output routines.
 *  <pre>
 *  When #including statio.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "stattypes.h"
 *         #include "list.h"
 *  </pre>
 */

#ifndef __STATIO_H_SEEN__
#define __STATIO_H_SEEN__

extern struct gl_list_t *GetTypeNamesFromStatList(CONST struct StatementList*sl);
/**<
 *  <!--  struct gl_list_t *GetTypesFromStatList(sl)                   -->
 *  <!--  CONST struct StatementList *sl;                              -->
 *  Return a gl_list of types referenced by the statement list
 *  given. If no types referenced, list will be empty but not NULL.
 *  List is sorted alphabetically. There may be duplicates. There
 *  may be undefined types if the compiler allows it.<br><br>
 *
 *  This operator should be reimplemented to use the childlist, since all
 *  types are now caught in that list and it is much faster than
 *  reprocessing the several lists that define a type.
 */

extern void WriteStatement(FILE *f, CONST struct Statement *s, int i);
/**< 
 *  <!--  void WriteStatement(f,s,i)                                   -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  int i;                                                       -->
 *  Print a statement with i leading blanks and a trailing newline.
 */

extern unsigned long StatementListLength(CONST struct StatementList *sl);
/**< 
 *  <!--  unsigned long StatementListLength(CONST struct StatementList *);  -->
 *  Returns the number of statements in the list, which may be 0.
 *  On NULL input, returns 0 -- and much code relies on this fact.
 */

extern void WriteDiffStatementList(FILE *f, CONST struct StatementList *sl1,
                                   CONST struct StatementList *sl2, int i);
/**<
 *  <!--  void WriteDiffStatementList(f,sl1,sl2,i)                     -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct StatementList *sl1, *sl2;                       -->
 *  <!--  int i;                                                       -->
 *  Print a statement list with i leading blanks for each line which
 *  is the statements on sl2 that are not on sl1.
 *  Avoid printing twice the statements inside a SELECT.
 */

extern void WriteStatementList(FILE *f, CONST struct StatementList *sl, int i);
/**<
 *  <!--  void WriteStatementList(f,sl,i)                              -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct StatementList *sl;                              -->
 *  <!--  int i;                                                       -->
 *  Print a statement list with i leading blanks for each line.
 *  Avoid printing twice the statements inside a SELECT.
 */

#define WSS(f,s) WriteStatementSuppressed(f,s)
/**< Shorhand for WriteStatementSuppressed(). */
 extern void WriteStatementSuppressed(FILE *f, CONST struct Statement *stat);
/**<
 *  <!--  macro WSS(f,stat)                                            -->
 *  <!--  void WriteStatementSuppressed(f,stat)                        -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Statement *stat;                                -->
 *
 *  Notify the user that a statement is being suppressed with a message
 *  printed on file f.
 */

#define WSEM(f,s,m) WriteStatementErrorMessage(f,s,m,1,0)
/**< Write a verbose error message using WriteStatementErrorMessage(). */
#define WSSM(f,s,m,l) WriteStatementErrorMessage(f,s,m,0,l)
/**< Write a brief error message using WriteStatementErrorMessage(). */
extern void WriteStatementErrorMessage(FILE *f,
                                       CONST struct Statement *stat,
                                       CONST char *message,
                                       int noisy,
                                       int level);
/**<
 *  <!--  macro WSEM(f,stat,message)  Write a message verbosely.       -->
 *  <!--  macro WSSM(f,stat,message,level)  Write a shorter message.   -->
 *  <!--  void WriteStatementErrorMessage(f,stat,message,noisy,level)  -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Statement *stat;                                -->
 *  <!--  const char *message;                                         -->
 *  <!--  int noisy;                                                   -->
 *  <!--  int level;                                                   -->
 *
 *  This procedure is an attempt to standardize statement error printing,
 *  so that all statement error messages are printed with the same formatting.
 *  Typically this procedure will print the "message" followed by the filename
 *  and line number where the error occurs.<br><br>
 *
 *  If noisy != 0, will include expressions when writing relations.
 *  FOR table information will also be displayed when appropriate.
 *
 *  If level !=0, will write in different format:
 *  - 1 => Asc-Style:   Line %lu <filename>: \n\tmessage\n<statement>
 *  - 2 => Asc-Warning: Line %lu <filename>: \n\tmessage\n<statement>
 *  - 3 => Asc-Error:   Line %lu <filename>: \n\tmessage\n<statement>
 *  - 4 => Asc-Fatal:   Line %lu <filename>: \n\tmessage\n<statement>
 */

extern CONST char *StatioLabel(int level);
/**<
 *  <!--  StatioLabel(level);                                          -->
 *  Returns an Asc-#######: label padded to a uniform length.
 *  You don't own the string returned.
 *  If you give us invalid level, label 0 is returned.
 *  StatioLabels are defined in WriteStatementErrorMessage above.
 */

extern int *GetStatioSuppressions(void);
/**<
 *  Returns a table initialized so that all statement types are
 *  NOT suppressed. To suppress a statement type,
 *  set table[statement_enum_of_type_you_hate] = 1.
 */

extern void DestroySuppressions(int *table);
/**<
 *  Destroys a table of suppressions from GetStatioSuppressions.
 */

#define WSEMSPARSE(f,s,m,t) WriteStatementErrorSparse(f,s,m,t)
/**< Shorthand for WriteStatementErrorSparse(). */
extern void WriteStatementErrorSparse(FILE *f,
                                      CONST struct Statement *stat,
                                      CONST char *message,
                                      int *table);
/**<
 *  <!--  macro WSEMSPARSE(f,stat,message,table)                       -->
 *  <!--  void WriteStatementErrorSparse(f,stat,message,table)         -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Statement *stat;                                -->
 *  <!--  const char *message;                                         -->
 *  <!--  int *table;                                                  -->
 *
 *  This procedure is an attempt to standardize statement error printing,
 *  so that all statement error messages are printed with the same formatting.
 *  Typically this procedure will print the "message" followed by the filename
 *  and line number where the error occurs.
 *  FOR information will also be displayed when appropriate.
 */

extern symchar *StatementTypeString(CONST struct Statement *stat);
/**< 
 * <!--  s = StatementTypeString(stat);                                -->
 * <!--  symchar *s;                                                   -->
 * <!--  CONST struct Statement *stat;                                 -->
 *
 * Returns a string from the symbol table corresponding to the
 * statement type given.
 */

extern void Asc_StatErrMsg_NotAllowedMethod(FILE *f, CONST struct Statement *stat);
/**< 
 * Writes a message to the file given indicating the statement is not
 * allowed in a method.
 */

extern void Asc_StatErrMsg_NotAllowedDeclarative(FILE *f,
                                                 CONST struct Statement *stat);
/**<
 * Writes a message to the file given indicating the statement is not
 * allowed in a declarative MODEL body.
 */

#endif /* __STATIO_H_SEEN__ */

