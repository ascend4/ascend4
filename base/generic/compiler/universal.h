/*
 *  Universal Routines
 *  by Tom Epperly
 *  Created: 3/27/1990
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: universal.h,v $
 *  Date last modified: $Date: 1997/07/18 12:36:21 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/*
 *  When #including universal.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "child.h"
 */



#ifndef __UNIVERSAL_H_SEEN__
#define __UNIVERSAL_H_SEEN__

/* requires
# #include"list.h"
# #include"instance_enum.h"
# #include"type_desc.h"
*/

#define UniversalTable gl_list_t

extern struct UniversalTable *CreateUniversalTable(void);
/*
 *  struct UniversalTable *CreateUniversalTable()
 *
 *  Create an empty table for holding the universal instances.
 */

extern void DestroyUniversalTable(struct UniversalTable *);
/*
 *  void DestroyUniversalTable(table)
 *  struct UniversalTable *table;
 *
 *  Destroy the list, but not the instances contained in the list.
 */

extern struct UniversalTable *MergeTables(struct UniversalTable *,
       struct UniversalTable *);
/*
 *  struct UniversalTable *MergeTables(table1,table2)
 *
 *  Merge the contents of two universal tables and make a joint table.
 *  The old tables are probably destroyed.
 *  NOT IMPLEMENTED.
 */

extern void SetUniversalTable(struct UniversalTable *);
/*
 *  void SetUniversalTable(table)
 *  struct UniversalTable *table;
 *
 *  Set the global universal table to "table".  This replaces the previous
 *  universal table, if one existed.  In general for instantiation to
 *  correctly handle universal instances, there must be an non-NULL
 *  universal table.  The universal table is initially NULL.
 */

extern struct UniversalTable *GetUniversalTable(void);
/*
 *  struct UniversalTable *GetUniversalTable()
 *
 *  Return a pointer to the current global universal table.
 */

extern struct Instance *LookupInstance(struct UniversalTable *,
           struct TypeDescription *);
/*
 *  struct Instance *LookupInstance(table,desc)
 *  struct UniversalTable *table;
 *  struct TypeDescription *desc;
 *
 *  Return the pointer to the universal instance of type desc.  This will
 *  return NULL if no such instance is in the table.
 */

extern void AddUniversalInstance(struct UniversalTable *,
     struct TypeDescription *,
     struct Instance *);
/*
 *  void AddUniversalInstance(table,desc,inst)
 *  struct UniversalTable *table;
 *  struct TypeDescription *desc;
 *  struct Instance *inst;
 *
 *  Add a type to the universal table.  Add the type "desc" to the universal
 *  list.  inst is the instance that all instances of type "desc" should be.
 *  This assumes that desc is not already in the list.
 *  This does no checking that the instance given is of the type given --
 *  it is assumed the user knows what they are doing, particularly in
 *  handling universals around refinement and merging.
 */

extern unsigned long NumberTypes(struct UniversalTable *);
/*
 *  unsigned long NumberTypes(table)
 *  struct UniversalTable *table;
 *
 *  Return the number of types found in the table.
 */

extern void ChangeUniversalInstance(struct UniversalTable *,
        struct Instance *,
        struct Instance *);
/*
 *  void ChangeUniversalInstance(table,oldinst,newinst)
 *  struct UniversalTable *table;
 *  struct Instance *oldinst,*newinst;
 *  Change any references of oldinst into newinst.
 */

extern void RemoveUniversalInstance(struct UniversalTable *,
        struct Instance *);
/*
 *  void RemoveUniversalInstance(table,inst)
 *  struct UniversalTable *table;
 *  struct Instance *inst;
 *
 *  Remove any type entry that contains instance inst.
 */

extern struct TypeDescription *GetTypeDescription(struct UniversalTable *,
        unsigned long);
/*
 *  struct TypeDescription *GetTypeDescription(table,pos);
 *
 *  Return the type description in position pos.  Note that the instance
 *  may be more refined that the type.
 */

extern struct Instance *GetInstance(struct UniversalTable *,
        unsigned long);
/*
 *  struct Instance *GetInstance(table,pos)
 *  struct UniversalTable *table;
 *  unsigned long pos;
 *
 *  Return the instance in position pos.
 */
#endif /* __UNIVERSAL_H_SEEN__ */
