/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1994 Kirk Andre Abbott

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Hash Table Module.
	
	Many hash tables are used throughout the implementation of a compiler
	and/or interpreter. This module (in the spirit of the list module)
	attempts to provide a generic table implementation, based on the classic
	*bucket and chains* for resolving collisions. Nothing fancy is done,
	except that we cache a ptr to the last thing found, so that access to
	it if required is fast. We append the new element to the front of the
	chain. The hashpjw algorithm is used.
	
	This module is appropriate for hash tables keyed with arbitrary strings.
	It is not appropriate for use with symbol table entry keys.

	Requires:
	#include "utilities/ascConfig.h"
*//*
	by Kirk A. Abbott
	Created December 29, 1994.
	Last in CVS $Revision: 1.3 $ $Date: 1998/06/16 15:47:47 $ $Author: mthomas $
*/

#ifndef ASC_TABLE_H
#define ASC_TABLE_H

typedef  void (*TableIteratorOne)(void *);
/**<
 * Typedef for casting of 1-parameter functions
 * being passed to TableApply* functions.
 */
typedef  void (*TableIteratorTwo)(void *,void *);
/**<
 * Typedef for casting of 2-parameter functions
 * being passed to TableApply* functions.
 */

ASC_DLLSPEC(struct Table *) CreateTable(unsigned long hashsize);
/**<
 *  Creates a new hash table with the specified hashsize.
 *  This ideally should be a prime number.  A good choice will give
 *  better performance without wasting too much memory.  Some good
 *  prime numbers are: 31, 97, 113, 229, 541, 1023, 3571.
 *  Everything is appropriately initialized.<br><br>
 *
 *  The function returns a pointer to the newly created hash table.
 *  Deallocation of the table is the responsibility of the caller 
 *  using DestroyTable().
 *
 *  @param hashsize The number of buckets in the new hash table.
 *  @return A pointer to the new hash table.
 *  @todo Should general/table:CreateTable have lower limit on hashsize?
 */

ASC_DLLSPEC(void ) DestroyTable(struct Table *table, int dispose);
/**<
 *  Destroys the given table.  If dispose is set to TRUE (nonzero), 
 *  the data items stored in the table will be deallocated as well.  
 *  Do not refer to a table after it has been destroyed.  The 
 *  specified table may be NULL, in which case this function has no
 *  effect.
 *
 *  @param table   Pointer to the hash table to destroy (non-NULL).
 *  @param dispose If non-zero deallocate the stored data also,
 *                 if 0 the data is destroyed as well.
 */

ASC_DLLSPEC(void ) AddTableData(struct Table * table, void *data, CONST char *id);
/**<
 *  Stores *data* in the hash table using id as the lookup key. 
 *  Currently id must be a unique NULL-terminated string.  In the 
 *  future, this may be changed to a generic lookup function 
 *  supplied by the user.  If the id is not unique in the table,
 *  the data is not added.  The *data* can be anything, including 
 *  a NULL pointer.  A pointer to the last item added is cached so 
 *  that subsequent lookup is fairly fast (see TableLastFind()).  
 *  Neither table nor id may be NULL (checked by assertion).
 *
 *  @param table Pointer to the hash table to add to (non-NULL).
 *  @param data  The data to store in the table.
 *  @param id    NULL-terminated string to use for lookup key (non-NULL).
 */

ASC_DLLSPEC(void *) LookupTableData(struct Table *table, CONST char *id);
/**<
 *  Retrieves the data stored in the table under the key *id*.
 *  It will return NULL if id is not found.  A pointer to the last item
 *  looked up is cached so that subsequent lookup is fairly fast
 *  (see TableLastFind()).  The return value will generally need to be
 *  cast to the correct type.  Neither table nor id may be NULL
 *  (checked by assertion).
 *
 *  @param table Pointer to the hash table to query (non-NULL).
 *  @param id    NULL-terminated string to use for lookup key (non-NULL).
 *  @return The data stored under lookup key id, or NULL if not found.
 */

ASC_DLLSPEC(void *) RemoveTableData(struct Table *table, char *id);
/**<
 *  Removes the data stored in the table under the key *id*.
 *  It will return NULL if id is not found.  Otherwise, it will
 *  return the data that was stored under id.  The data is not
 *  deallocated.  The return value will generally need to be
 *  cast to the correct type.  Neither table nor id may be NULL
 *  (checked by assertion).
 *
 *  @param table Pointer to the hash table to query (non-NULL).
 *  @param id    NULL-terminated string to use for lookup key (non-NULL).
 *  @return The data stored under lookup key id, or NULL if not found.
 */

ASC_DLLSPEC(void ) TableApplyOne(struct Table *table,
                          TableIteratorOne applyfunc,
                          char *id);
/**<
 *  Calls the specified function, passing the data stored in the table
 *  under key *id* as argument.  The lookup is fast if the data item was
 *  the last searched for, slower otherwise.  The function applyfunc must
 *  be able to handle NULL pointers gracefully.  Neither table, applyfunc,
 *  nor id may be NULL (checked by assertion).
 *
 *  @param table     Pointer to the hash table to use (non-NULL).
 *  @param applyfunc Pointer to the function to apply (non-NULL).
 *  @param id        NULL-terminated string to use for lookup key (non-NULL).
 */

ASC_DLLSPEC(void ) TableApplyAll(struct Table *table, TableIteratorOne applyfunc);
/**<
 *  Calls the specified function for each data item stored in the table.
 *  The order of operation dependends on internal table structure, so is
 *  not predictable and should not be relied upon.  Using this function
 *  should be a lot faster than fetching each element independently and 
 *  applying applyfunc to it.  The function must be able to handle NULL 
 *  pointers gracefully.  Neither table nor applyfunc may be NULL 
 *  (checked by assertion).
 *
 *  @param table     Pointer to the hash table to use (non-NULL).
 *  @param applyfunc Pointer to the function to apply (non-NULL).
 */

ASC_DLLSPEC(void ) TableApplyAllTwo(struct Table *table,
                             TableIteratorTwo applyfunc,
                             void *arg2);
/**<
 *  Calls the specified function for each data item stored in the table.
 *  This is the same as TableApplyAll(), except that arg2 is passed as a
 *  second argument to applyfunc allowing a closure.  The order of 
 *  operation dependends on internal table structure, so is not 
 *  predictable and should not be relied upon.  Using this function
 *  should be a lot faster than fetching each element independently and
 *  applying applyfunc to it.  The function must be able to handle NULL
 *  pointers gracefully.  Neither table nor applyfunc may be NULL
 *  (checked by assertion).
 *
 *  @param table     Pointer to the hash table to use (non-NULL).
 *  @param applyfunc Pointer to the function to apply (non-NULL).
 *  @param arg2      The 2nd argument to pass to applyfunc.
 */

extern void PrintTable(FILE *file, struct Table *table);
/**<
 *  Prints information about the table to the given file.  This
 *  information currently includes an ordered list of bucket
 *  numbers and id strings for each data item in the table.
 *  The file must be opened and ready for writing.  Neither table
 *  nor file may be NULL (checked by assertion).
 *
 *  @param file  Open, writable file stream to receive the report (non-NULL).
 *  @param table Pointer to the hash table to query (non-NULL).
 */

ASC_DLLSPEC(unsigned long ) TableSize(struct Table *table);
/**<
 *  Returns the current number of entries in the table.  The
 *  specified table may not be NULL (checked by assertion).
 *
 *  @param table Pointer to the hash table to query (non-NULL).
 */

ASC_DLLSPEC(unsigned long ) TableHashSize(struct Table *table);
/**<
 *  Returns the current hashsize of the table.  If internally we 
 *  change the hashing/collision algorithm, this may be useful 
 *  information to someone.  At the moment it is the size 
 *  requested and hence is not very useful.  The specified table 
 *  may not be NULL (checked by assertion).
 *
 *  @param table Pointer to the hash table to query (non-NULL).
 */

ASC_DLLSPEC(void *) TableLastFind(struct Table *table);
/**<
 *  Returns the data item that was last added to or retrieved from the 
 *  table.  Could be useful for those, "do you exist?; now do something 
 *  with you" situations.  Returns NULL if no item was added or retrieved
 *  from the table.  The specified table may not be NULL (checked by
 *  assertion).
 *
 *  @param table Pointer to the hash table to query (non-NULL).
 */

#endif /* ASC_TABLE_H */
