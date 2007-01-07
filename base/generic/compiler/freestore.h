/*
 *  Implementation of Free Store Module
 *  Kirk A. Abbott
 *  Created Dec 18, 1994
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: freestore.h,v $
 *  Date last modified: $Date: 1998/06/16 16:36:27 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

/** @file
 *  Implementation of Free Store Module.
 *
 *  Note: this file is specific to relation data structures and should
 *  not be used for anything else. In fact it shouldn't even be used
 *  for those because a much better tested module (pool.h) is
 *  available and in use everywhere else in the compiler.
 *  <pre>
 *  When #including freestore.h, make sure these files are #included first:
 *        #include "utilities/ascConfig.h"
 *        #include "compiler.h"
 *        #include "stack.h"
 *        #include "exprsym.h"
 *  </pre>
 */

#ifndef ASC_FREESTORE_H
#define ASC_FREESTORE_H

/**	@addtogroup compiler Compiler
	@{
*/

/** Free store data structure. */
struct FreeStore {
  union RelationTermUnion **root;     /**< ptr to it all */
  union RelationTermUnion *next_free; /**< ptr to next_clean slot */
  struct gs_stack_t *returned;        /**< stack of returned blocks */
  int n_buffers;                      /**< # of big chunks */
  int buffer_length;                  /**< size of chunks */
  int row;                            /**< index of the next free location's row */
  int col;                            /**< index of the next free locations's col */
};

extern long FreeStore_UnitsAllocated();
/**< Retrieve the number of free store units allocated. */
extern void FreeStore__Statistics(FILE *fp, struct FreeStore *store);
/**< Print stats about the free store to fp. */

extern void FreeStore__BlastMem(struct FreeStore *store);
/**<
 *  This function deallocates *all* the memory associated with a free
 *  store. The free store should not be referenced after this call.
 */

/*
 *  These are the normal user level routines.
 *
 *  Known Bugs:
 *  The blocks returned the user are never anything but the size of
 *  a relation token. This is not highly reuseable.
 *
 *  Features?:
 *  This module runs a stack of free_stores, which is sometimes
 *  handy, to avoid passing around a free_store pointer. Note,
 *  however, that it costs just as much to dereference the global
 *  variables as it does to pass the store pointer.
 */

extern struct FreeStore *FreeStore_Create(int n_buffers, int buffer_length);
/**< 
 *  Create a new free store with 1 buffer of size buffer length.
 *  The information within the buffer is *not* initialized in any way.
 */

extern void FreeStore_ReInit(struct FreeStore *store);
/**< 
 *  Reinitializes the free store. This simply tells the free store to
 *  *forget* about anything that may have be allotted to a user. In other
 *  words, after this call the freestore will behave as if it was just
 *  created. Any information that a user wants, had better be copied before
 *  this call, as it is now a candidate to be allotted to someone else.
 */

extern union RelationTermUnion *GetMem();
/**< 
 *  This function returns a marked block to the caller. When not needed
 *  any more the user must use FreeMem(term).
 *  It is the users responsibility to remember
 *  which store that he/she got his block from. It is envisioned that multiple
 *  freestore could be used, such as a store for parse nodes that will last
 *  throughout the life of the code, and a store for relation terms that
 *  are of dynamic nature, such as in doing symbolic manipulations on a
 *  relation.
 */

extern void FreeMem(union RelationTermUnion *term);
/**< 
 *  This function returns a block of memory that was obtained from a
 *  freestore. The appropriate store must be set a priori.
 */

extern union RelationTermUnion
*FreeStoreCheckMem(union RelationTermUnion *term);
/**< 
 *  This will function will return a non NULL pointer if the memory
 *  associated with the term was allocated from the currently active
 *  freestore.
 */

extern void FreeStore_SetFreeStore(struct FreeStore *store);
/**<  Set the current working freestore. */
extern struct FreeStore *FreeStore_GetFreeStore(void);
/**<  Retrieve the current working freestore. */

/* @} */

#endif /* ASC_FREESTORE_H */

