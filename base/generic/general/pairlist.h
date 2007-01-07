/*	ASCEND modelling environment
	Copyright (C) 2006 Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

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
	Ascend pointer pair manager.

	This module uses (internally) a pair of gl_list_t
	to manage pointer pairs.

	The following definitions provide a quick-and-dirty pointer lookup by
	pointer key. The lookup is order N. The table does not treat NULL keys
	or values specially. elements of the pairlist are numbered [1..length].

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
	#include "compiler.h"
*//*
	by Benjamin Andrew Allan
	Created: 10/2006

	Dates:        10/2006 - original version
*/

#ifndef __pairlist_h_seen__
#define __pairlist_h_seen__

struct pairlist_t;

#define pairlist_DEBUG FALSE
/**<
 *  Flag controlling extra checking of the pairlist management routines.
 *  Setting pairlist_DEBUG to TRUE causes the pairlist_store routines to do
 *  some RATHER expensive checking. It should be set to FALSE.
 */

ASC_DLLSPEC struct pairlist_t * pairlist_create(unsigned long capacity);
/**<
 *  Creates and returns a new pairlist.
 *  Complexity O(1).
 *
 *  @param capacity the initial capacity of the list; the initial size is 0.
 *
 *  @return A pointer to the newly created pairlist, NULL if an error occurred.
 */

ASC_DLLSPEC void *pairlist_keyAt(struct pairlist_t * pl, unsigned long eindex);
/**<
 *  Get the key at eindex'th element of the list.
 *  Complexity O(1).
 *  @return the value pointer.
 */

ASC_DLLSPEC void *pairlist_valueAt(struct pairlist_t * pl, unsigned long eindex);
/**<
 *  Get the value at eindex'th element of the list.
 *  Complexity O(1).
 *  @return the value pointer.
 */

ASC_DLLSPEC unsigned long pairlist_contains(struct pairlist_t * pl, void *key);
/**<
 *  Return the index of the key, if it is in the list, or 0 if not.
 *  Complexity O(n).
 *  @return the key index.
 */

ASC_DLLSPEC unsigned long pairlist_append(struct pairlist_t * pl, void *key, void * value);
/**<
 *  Add a pair to the list. Uniqueness not guaranteed. Complexity O(1).
 *  @return the key index.
 */

ASC_DLLSPEC unsigned long pairlist_append_unique(struct pairlist_t * pl, void *key, void * value);
/**<
 *  Add a pair to the list. Uniqueness of key guaranteed. If duplicate,
 *  value is ignored. Complexity O(n).
 *  @return the key index.
 */


ASC_DLLSPEC void pairlist_clear(struct pairlist_t * pl);
/**<
 *  Forget the previous contents of a list.
 *  @param the list to clear.
 */

ASC_DLLSPEC void pairlist_destroy(struct pairlist_t * pl);
/**<
 *  Deallocates everything associated with the pl.
 *  @param pl The pairlist store to destroy.
 */

ASC_DLLSPEC long pairlist_length(struct pairlist_t * pl);
/**<
 *  @return the amount of data in the list.
 *  @param pl The pairlist store to destroy.
 */

extern void pairlist_print(FILE *fp, struct pairlist_t * pl);
/**<
 *  Prints statistics about a struct pairlist_t * to the file stream given.
 *
 *  @param fp     The open file stream on which to print the report.
 *  @param pl     The pairlist store on which to report.
 */

#endif  /* __pairlist_h_seen__ */

