/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Ascend Pending Instance Routines.

	The pending list is implemented as a doubly linked list.
	Clients of this module should NOT access the internals of this list.
	They should not access next and prev in particular.
	Note that only complex types (models, arrays) and not atomic ones
	(atoms, relations, constants) should ever be put on the list.
	No client should ever free a struct pending_t *. That is our job.

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
	#include "compiler.h"
*//*
	by Tom Epperly
	Created: 1/24/90
	Version: $Revision: 1.6 $
	Version control file: $RCSfile: pending.h,v $
	Date last modified: $Date: 1997/07/18 12:32:40 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_PENDING_H
#define ASC_PENDING_H

/**	@addtogroup compiler_inst Compiler Instance Hierarchy
	@{
*/

#include <ascend/general/platform.h>

struct pending_t {
  struct pending_t *next, *prev;
  struct Instance *inst;
};

extern void InitPendingPool(void);
/**<
 *  Sets up pending structure data management
 *  before anything can be built, ideally at startup time.
 *  Do not call it again unless DestroyPendingPool is called first.
 *  If insufficient memory to compile anything at all, does exit(2).
 */

extern void DestroyPendingPool(void);
/**<
 *  Destroy pending structure data management. This must be called to
 *  clean up before shutting down ASCEND.
 *  Do not call this function while there are any instances actively
 *  pending unless you are shutting down.
 *  Do not attempt to instantiate anything after you call this unless you
 *  have recalled InitPendingPool.
 */

extern void ReportPendingPool(FILE *f);
/**<
 *  Reports on the pending pool to f.
 */

#ifdef NDEBUG
#define PendingInstance(pt) ((pt)->inst)
#else
#define PendingInstance(pt) PendingInstanceF(pt)
#endif
/**<
 *  Returns the instance part of a pending_t structure.
 *  @param pt CONST struct pending_t*, pending instance to query.
 *  @return Returns the instance as a <code>struct Instance*</code>.
 *  @see PendingInstanceF()
 */
extern struct Instance *PendingInstanceF(CONST struct pending_t *pt);
/**<
 *  Implementation function for PendingInstance().  Do not call this
 *  function directly - use PendingInstance() instead.
 *  This returns the instance part of a pending_t structure.
 */

extern void ClearList(void);
/**<
 *  Prepare an empty list.  This gets rid of any remaining list and makes
 *  a new empty list ready for use.
 *  Causes any instance remaining in the list to forget that they are
 *  pending.
 */

extern unsigned long NumberPending(void);
/**<
 *  Return the number of instances in the pending instance list.
 */

extern void AddBelow(struct pending_t *pt, struct Instance *i);
/**<
 *  This adds i into the pending list just below the entry pt.  If pt
 *  is NULL, this adds i to the top.
 *  i should be a MODEL_INST or ARRAY_*_INST
 */

extern void AddToEnd(struct Instance *i);
/**<
 *  Insert instance i at the end of the pending instance list.
 *  i should be a MODEL_INST or ARRAY_*_INST
 */

extern void RemoveInstance(struct Instance *i);
/**<
 *  Remove instance i from the pending instance list if it is in it.
 *  i should be a MODEL_INST or ARRAY_*_INST
 */

extern void PendingInstanceRealloced(struct Instance *old_inst, struct Instance *new_inst);
/**<
 *  Change references to old to new.
 *  Assumes the old instance will never be used by anyone at all ever again.
 *  new should be a MODEL_INST or ARRAY_*_INST recently realloced.
 */

extern int InstanceInList(struct Instance *i);
/**<
 *  Return true iff i is in the list.
 *  i should be a MODEL_INST or ARRAY_*_INST as any other kind cannot be
 *  pending.
 */

extern struct pending_t *TopEntry(void);
/**<
 *  Return the top item in the pending list.
 */

extern struct pending_t *ListEntry(unsigned long n);
/**<
 *  Return the n'th entry in the list.  This returns NULL if n is less
 *  than one or greater than the length of the list.
 */

extern struct pending_t *BottomEntry(void);
/**<
 *  Return the bottom item in the pending list.
 */

extern void MoveToBottom(struct pending_t *pt);
/**<
 *  Move the item pt to the bottom of the list.
 */

ASC_DLLSPEC unsigned long NumberPendingInstances(struct Instance *i);
/**<
 *  Visits the Instance Tree seatch for instances with pending statements.
 *  Increments g_unresolved_count for each pending instance found.
 *  Returns the total count of pendings.
 */

/* @} */

#endif  /* ASC_PENDING_H */

