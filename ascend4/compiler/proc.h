/*
 *  Procedure Data Structure
 *  by Tom Epperly
 *  Created: 1/10/90
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: proc.h,v $
 *  Date last modified: $Date: 1998/04/11 01:31:54 $
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

/*
 *  When #including proc.h, make sure these files are #included first:
 *         #include "compiler.h"
 */



#ifndef __PROC_H_SEEN__
#define __PROC_H_SEEN__
/* requires
# #include"compiler.h"
# #include"list.h"
# #include"slist.h"
*/

struct InitProcedure {
  symchar *name;		/* procedure's name */
  struct StatementList *slist;	/* statement list */
  long int parse_id;            /* parse id of type desc with which this is defined */
                                /* 0 if method is not yet associated */
};

extern struct InitProcedure *CreateProcedure(symchar *,
          struct StatementList *);
/*
 *  struct InitProcedure *CreateProcedure(name,stats)
 *  symchar *name;
 *  struct StatementList *stats;
 *  Create a procedure data structure with name and stats.
 */

extern struct InitProcedure *CopyProcedure(struct InitProcedure *);
/*
 *  struct InitProcedure *CopyProcedure(p)
 *  struct InitProcedure *p;
 *  Make a copy of procedure p, but this copy should not be modified.
 *  If you want to change the structure, use CopyProcToModify.  Use
 *  DestroyProcedure when you are done with it.
 */

extern struct InitProcedure *CopyProcToModify(struct InitProcedure *);
/*
 *  struct InitProcedure *CopyProcToModify(p)
 *  struct InitProcedure *p;
 *  Make a copy, but this copy can be changed.
 */

extern struct gl_list_t *MergeProcedureLists(struct gl_list_t *,
                                             struct gl_list_t *);
/*
 *  plr = MergeProcedureLists(old,new);
 *  struct gl_list_t *plr, *old, *new;
 *  old or new may be NULL.
 *  Merges the lists of (struct InitProcedure *) given into a third list,
 *  plr.
 *  new, if it exists, is destroyed because we move the contents to plr.
 *  old is assumed to belong to someone else and is left alone.
 *  The result is sorted with CmpProcs.
 *
 * This is where method inheritance rules occur. We keep all the
 * methods from the new in result, then we search for any method in
 * the old list which does not have an identically named
 * counterpart in the result list and add it to the result.
 * Methods in the old result which have be redefined in the new
 * list given are thereby ignored and overridden.
 */

extern struct gl_list_t *GetUniversalProcedureList(void);
/*
 * Returns the list of methods defined for all MODELs
 * unless they redefine the methods themselves.
 */

extern void SetUniversalProcedureList(struct gl_list_t *);
/*
 * Sets the list of procedures defined for all MODELs.
 * If a UPL already exists, it will be destroyed unless it
 * is the same. If the same, it is resorted.
 */

extern void DestroyProcedure(struct InitProcedure *);
/*
 *  void DestroyProcedure(p)
 *  struct InitProcedure *p;
 *  Destroy this reference to p.  This won't necessary destroy all the parts
 *  of p.
 */

extern void DestroyProcedureList(struct gl_list_t *);
/*
 *  void DestroyProcedureList(pl)
 *  struct gl_list_t *pl contain pointers to
 *  struct InitProcedure *p
 *  Destroy this reference to p.  This won't necessary destroy all the parts
 *  of each p unless the parts have no other references. The gl_list is
 *  destroyed as are all the p in it.
 *  Handles NULL input gracefully.
 */

extern int CompareProcedureLists(struct gl_list_t *,struct gl_list_t *,
                                 unsigned long int *);
/*
 *  CompareProcedureLists(pl1,pl2,&n)
 *  struct gl_list_t *pl1, *pl2 contain pointers to
 *  struct InitProcedure *p.
 *  unsigned long n;
 *  Returns 0 if pl1,pl2 semantically equivalent, 1 if not.
 *  If return is 1, n contains the index of the first different
 *  procedure.
 */  

#ifdef NDEBUG
#define ProcName(p) ((p)->name)
#else
#define ProcName(p) ProcNameF(p)
#endif
extern symchar *ProcNameF(CONST struct InitProcedure *);
/*
 *  macro ProcName(p)
 *  symchar *ProcNameF(p)
 *  const struct InitProcedure *p;
 *  Return the name part of a procedure structure.
 */

#ifdef NDEBUG
#define ProcStatementList(p) ((p)->slist)
#else
#define ProcStatementList(p) ProcStatementListF(p)
#endif
extern struct StatementList *ProcStatementListF(CONST struct InitProcedure *);
/*
 *  struct StatementList *ProcStatementListF(p)
 *  const struct InitProcedure *p;
 *  Return the statement list part of the procedure structure.
 */

#ifdef NDEBUG
#define GetProcParseId(p) ((p)->parse_id)
#else
#define GetProcParseId(p) GetProcParseIdF(p)
#endif
extern long GetProcParseIdF(CONST struct InitProcedure *);
/*
 *  id = GetProcParseIdF(p);
 *  const struct InitProcedure *p;
 *  long id;
 *  Return the parse id of the type which originally defined this 
 *  procedure. This may be a copy of that procedure and not the
 *  original, but this is of no significance.
 */

#ifdef NDEBUG
#define SetProcParseId(p,id) ((p)->parse_id = (id))
#else
#define SetProcParseId(p,id) SetProcParseIdF((p),(id))
#endif
extern void SetProcParseIdF(struct InitProcedure *,long);
/*
 *  SetProcParseIdF(p,id);
 *  const struct InitProcedure *p;
 *  long id.
 *  Sets the parse id of the procedure. The wisdom of this move
 *  is not investigated. The rules should be:
 *  Procs normally get id's at the type desc creation step once
 *  parseid is known. When a proc is inherited (by copy) the copy
 *  retains the id of the original. If a method is added after the
 *  type's creation, it should get that types id. If a method is replaced
 *  in a type, all the refiners of that type which contain an
 *  inherited copy of the method also get replaced and this id
 *  helps us figure out which method was inherited efficiently.
 */

extern int CmpProcs(CONST struct InitProcedure *,CONST struct InitProcedure *);
/*
 *  int CmpProcs(p1,p2)
 *  const struct InitProcedure *p1,*p2;
 *  Compare the two procedures to provide an ordering for them.
 *  Simply alphabetizing.
 */
#endif /* __PROC_H_SEEN__ */
