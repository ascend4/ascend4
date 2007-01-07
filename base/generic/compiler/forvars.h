/*
 *  FOR Loop Index Variable Table
 *  by Tom Epperly
 *  Created: 1/14/89
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: forvars.h,v $
 *  Date last modified: $Date: 1998/02/05 16:36:08 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  FOR Loop Index Variable Table.
 *  <pre>
 *  When #including forvars.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_FORVARS_H
#define ASC_FORVARS_H

/**	@addtogroup compiler Compiler
	@{
*/

enum for_kind {
  f_untyped,
  f_integer,
  f_symbol,
  f_set
};

union for_union_t{
  long ivalue;
  symchar *sym_ptr;
  struct set_t *sptr;
};

struct for_var_t {
  symchar *name;    /**< internally used as a recycle ptr */
  enum for_kind t;  /**< what type of index variable it is */
  union for_union_t value;
};

#define for_table_t gl_list_t

extern struct for_table_t *CreateForTable(void);
/**<
 *  <!--  struct for_table_t *CreateForTable()                         -->
 *  This function creates an empty FOR table which is then ready for use.
 */

extern void DestroyForTable(struct for_table_t *ft);
/**<
 *  <!--  void DestroyForTable(ft)                                     -->
 *  <!--  struct for_table_t *ft;                                      -->
 *  This procedure deallocates the memory associated with the for table.
 */

extern void WriteForTable(FILE *out, struct for_table_t *ft);
/**< 
 *  <!--  void WriteForTable(out,ft)                                   -->
 *  <!--  FILE *out;                                                   -->
 *  <!--  struct for_table_t *ft;                                      -->
 *  This procedure writes the contents of a for table.
 */

extern unsigned long ActiveForLoops(CONST struct for_table_t *ft);
/**< 
 *  <!--  unsigned long ActiveForLoops(ft)                             -->
 *  <!--  struct for_table_t *ft;                                      -->
 *  Returns the number of active FOR loops.
 */

extern void AddLoopVariable(struct for_table_t *ft,struct for_var_t *var);
/**< 
 *  <!--  void AddLoopVariable(ft,var)                                 -->
 *  <!--  struct for_table_t *ft;                                      -->
 *  <!--  struct for_var_t *var;                                       -->
 *  Add another loop variable to the for table.  Adding a loop variable to
 *  the for table is like activating the FOR loop variable
 */

extern struct for_var_t *LoopIndex(CONST struct for_table_t *ft,
                                   unsigned long num);
/**<
 *  <!--  struct for_var_t *LoopIndex(ft,num)                          -->
 *  <!--  const struct for_table_t *ft;                                -->
 *  <!--  unsigned long num;                                           -->
 *  Returns the num'th loop index.  Loop indices are numbered in the
 *  order they are added to the list.  The first being one and the last
 *  being the number given by ActiveForLoops(ft).
 */

extern struct for_var_t *FindForVar(CONST struct for_table_t *ft, 
                                    symchar *name);
/**< 
 *  <!--  struct for_var_t *FindForVar(ft,name)                        -->
 *  <!--  CONST struct for_table_t *ft;                                -->
 *  <!--  symchar *name;                                               -->
 *  Searches for a FOR index variable which matches name.  If it finds a
 *  match it returns the for_var_t; otherwise, it returns NULL.
 */

extern void RemoveForVariable(struct for_table_t *ft);
/**< 
 *  <!--  void RemoveForVariable(ft)                                   -->
 *  <!--  struct for_table_t *ft;                                      -->
 *  This removes the most recently added FOR index variable.  The
 *  for_var_t is automatically deallocated.
 */

/*
 *  Routines to create, query, modify and destroy for_var_t's.
 */

extern struct for_var_t *CreateForVar(symchar *name);
/**< 
 *  <!--  struct for_var_t *CreateForVar(name)                         -->
 *  <!--  const char *name;                                            -->
 *  Create a for_var_t with the name given.  This for_var_t starts out being
 *  f_untyped.  You can use the routines below to set type and values.
 *  Never free a for_var_t except by using DestroyForVar below.
 */

extern void SetForVarType(struct for_var_t *ft, enum for_kind t);
/**< 
 *  <!--  void SetForVarType(fv,t)                                     -->
 *  <!--  struct for_var_t *fv;                                        -->
 *  <!--  enum for_kind t;                                             -->
 *  Set the type to t.  fv must be untyped.
 */

extern void SetForInteger(struct for_var_t *fv, long ivalue);
/**< 
 *  <!--  void SetForInteger(fv,ivalue)                                -->
 *  <!--  struct for_var_t *fv;                                        -->
 *  <!--  long ivalue;                                                 -->
 *  Set an integer for variable's value to ivalue.
 */

extern void SetForSymbol(struct for_var_t *fv, symchar *sym_ptr);
/**< 
 *  <!--  void SetForSymbol(fv,sym_ptr)                                -->
 *  <!--  struct for_var_t *fv;                                        -->
 *  <!--  const char *sym_ptr;                                         -->
 *  Set a symbol for variable's value to sym_ptr.
 */

extern void SetForSet(struct for_var_t *fv, struct set_t *sptr);
/**<
 *  <!--  void SetForSet(fv,sptr)                                      -->
 *  <!--  struct for_var_t *fv;                                        -->
 *  <!--  struct set_t *sptr;                                          -->
 *  Set a set for variable's value to sptr.
 */

extern enum for_kind GetForKind(CONST struct for_var_t *fv);
/**< 
 *  <!--  for_kind GetForKind(fv)                                      -->
 *  <!--  const struct for_var_t *fv;                                  -->
 *  Return the type of the for variable.
 */

extern symchar *GetForName(CONST struct for_var_t *fv);
/**< 
 *  <!--  symchar *GetForName(fv)                                      -->
 *  <!--  const struct for_var_t *fv;                                  -->
 *  Return the name of the for variable
 */

extern long GetForInteger(CONST struct for_var_t *fv);
/**< 
 *  <!--  long GetForInteger(fv)                                       -->
 *  <!--  const struct for_var_t *fv;                                  -->
 *  Return the value of an integer for variable.
 */

extern symchar *GetForSymbol(CONST struct for_var_t *fv);
/**<
 *  <!--  symchar *GetForSymbol(fv)                                    -->
 *  <!--  const struct for_var_t *fv;                                  -->
 *  Return the value of a symbol for variable.
 */

extern CONST struct set_t *GetForSet(CONST struct for_var_t *fv);
/**< 
 *  <!--  const struct set_t *GetForSet(fv)                            -->
 *  <!--  const struct for_var_t *fv;                                  -->
 *  Return the value of a set for variable.
 */

extern void DestroyForVar(struct for_var_t *fv);
/**< 
 *  <!--  void DestroyForVar(fv)                                       -->
 *  <!--  struct for_var_t *fv;                                        -->
 *  Deallocate the memory of this for variable.  In the case of a set
 *  for variable, this will also deallocate the set.
 */

extern int ClearForVarRecycle(void);
/**< 
 *  <!--  int ClearForVarRecycle();                                    -->
 *  Deallocates the recycle list of forvar_t. returns the list length,
 *  if anyone cares.
 *  This function may be safely called at any time.
 *  There is no recycle initialization function.
 */

/* @} */

#endif /* ASC_FORVARS_H */

