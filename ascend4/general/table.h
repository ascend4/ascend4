/*
 *  Table Module
 *  by Kirk A. Abbott
 *  Created December 29, 1994.
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: table.h,v $
 *  Date last modified: $Date: 1998/06/16 15:47:47 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1994 Kirk Andre Abbott
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
 *
 */
/*
 *  Abstract
 *
 *  Many hash tables are used throughout the implementation of a compiler
 *  and/or interpreter. This module (in the spirit of the list module)
 *  attempts to provide a generic table implementation, based on the classic
 *  *bucket and chains* for resolving collisions. Nothing fancy is done,
 *  except that we cache a ptr to the last thing found, so that access to
 *  it if required is fast. We append the new element to the front of the
 *  chain. The hashpjw algorithm is used.
 */
/*
 * This module is appropriate for hash tables keyed with arbitrary strings.
 * It is not appropriate for use with symbol table entry keys.
 */

#ifndef __TABLE_H_SEEN__
#define __TABLE_H_SEEN__


typedef  void (*TableIteratorOne)(void *);
typedef  void (*TableIteratorTwo)(void *,void *);
/*
 * Typedefs for casting of functions being passed to TableApply* functions.
 */

extern struct Table *CreateTable(unsigned long);
/*
 *  struct Table *CreateTable(hashsize)
 *  unsigned long hashsize;
 *  Creates a new hash table with the specified hashsize. This ideally
 *  should be a prime number. A good choice will give better performance
 *  without wasting too much memory. Some good prime numbers are:
 *  31, 97, 113, 229, 541, 1023, 3571. Everything is appropriately
 *  initialized.
 */

extern void DestroyTable(struct Table *,int);
/*
 *  void DestroyTable(table,info);
 *  struct Table *table;
 *  int info;
 *  Destroys the given table. If info is set to TRUE (nonzero), the
 *  information given stored in the table will be deallocated as well.
 *  Do not refer to a table after it has been destroyed.
 *
 *  Bugs: info is ignored. we always destroy the bucketed info.
 */

extern void AddTableData(struct Table *,void *,CONST char *);
/*
 *  void AddTableData(table,data,id);
 *  struct Table *table;
 *  void *data;
 *  char *id;
 *  This function will store *data* in the table and will use id as the
 *  key for looking it up. id at the moment must be a NULL terminated
 *  string. In the future, this may be changed to a generic lookup function
 *  which the user will have to provide. The *data* can be anything. On
 *  return the user must appropriately cast his data to be of the correct
 *  type.
 */

extern void *LookupTableData(struct Table *,CONST char *);
/*
 *  void LookupTableData(table,id)
 *  struct Table *table;
 *  char *id;
 *  This function will lookup the information associated with id,
 *  in the given table. It will return NULL, if not found, otherwise
 *  it will return the information that was stored in the table.
 *  We cache away a ptr to the last thing looked up so, that a subsequent
 *  lookup is fairly fast.
 */

extern void *RemoveTableData(struct Table *,char *);
/*
 *  void *RemoveTableData(table,id);
 *  struct Table *table;
 *  char *id;
 *  This will remove information stored in the table under the key *id*.
 *  It will return NULL, if the information did not exist, otherwise
 *  it will return that the information that was stored under id.
 */

void TableApplyOne(struct Table *,TableIteratorOne,char *);
/*
 *  void TableApplyOne(table,applyfunc,id);
 *  struct Table *table;
 *  void (*applyfunc)(void *);
 *  char *id;
 *  This function will apply the given function to information matching
 *  the key id stored in the table. All that we do is try to find
 *  the data stored with key *id*, once found we apply your function to it.
 *  Your function is entirely responsible for handling NULL cases.
 */

extern void TableApplyAll(struct Table *,TableIteratorOne);
/*
 *  void TableApplyOne(table,applyfunc);
 *  struct Table *table;
 *  TableIteratorOne applyfunc;
 *  The same as TableApplyOne except it is applied to every element stored
 *  in the table. The order of operation is given by how things are stored
 *  in the table. Potentially a very useful function and should be alot
 *  faster than fetching each element yourself, and applying your function
 *  to it.
 */

extern void TableApplyAllTwo(struct Table *,TableIteratorTwo, void *);
/*
 *  struct Table *table;
 *  TableIteratorTwo applyfunc;
 *  void *arg2;
 *  Same as TableApplyAllTwo but allows a closure, by allowing a second
 *  arguement to be passed in.
 */

extern void PrintTable(FILE *,struct Table *);
/*
 *  void PrintTable(f,table);
 *  FILE *f;
 *  struct Table *table;
 *  Will print the table to the given file (which must be opened and
 *  writable -- I dont check!!) the following information:
 *  Entry Number, Bucket Number, Entry Id.
 *  Later more statistics could be given, such as bucket, distribution etc.
 */

extern unsigned long TableSize(struct Table *);
/*
 *  unsigned long TableSize(table);
 *  struct Table *table;
 *  Returns the current number of entries in the table.
 */

extern unsigned long TableHashSize(struct Table *);
/*
 *  unsigned long TableHashSize(table);
 *  struct Table *table;
 *  Returns the current hashsize of the table. If internally we change
 *  the hashing/collision algorithm, this may be useful information to
 *  someone. At the moment it is the size requested and hence is not
 *  very useful to a user.
 */

extern void *TableLastFind(struct Table *);
/*
 *  void *TableLastFind(table);
 *  struct Table *table;
 *  Returns the information that was last requested. Could be useful
 *  for those, "do you exist?; now do something with you".
 */

#endif /* __TABLE_H_SEEN__ */
