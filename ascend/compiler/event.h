/*	ASCEND modelling environment
	Copyright (C) 2013 Carnegie Mellon University

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
	Event list and utility and output routines.

*//*
	Created: 19/06/2013
*/

#ifndef ASC_EVENT_H
#define ASC_EVENT_H

#include "stattypes.h"
#include "case.h"
#include "instance_enum.h"

/* Event list functions */

extern struct EventList *CreateEvent(struct Set *set, struct StatementList *sl);
/**< 
 *  Create an event node.
 */

extern struct EventList *ReverseEventCases(struct EventList *e);
/**< 
 *  Reverse this list.
 */

extern struct EventList *LinkEventCases(struct EventList *e1, struct EventList *e2);
/**< 
 *  Link two case lists and return the joined list.  This works best when
 *  e1 is a one element list.
 */


#ifdef NDEBUG
#define NextEventCase(e) ((e)->next)
#else
#define NextEventCase(e) NextEventCaseF(e)
#endif
/**<
 *  Return the next case in the list.
 *  @param e struct EventList*, the event list to query.
 *  @return The next case as a struct EventList*.
 *  @see NextEventCaseF()
 */
extern struct EventList *NextEventCaseF(struct EventList *e);
/**<
 *  Implementation function for NextEventCase() (debug mode).
 *  Do not call this function directly - use NextEventCase() instead.
 */

#ifdef NDEBUG
#define EventSetList(e) ((e)->values)
#else
#define EventSetList(e) EventSetListF(e)
#endif
/**<
 *  This will return the set list part of an EventList structure.
 *  @param e struct EventList*, the event list to query.
 *  @return The set list part as a struct Set*.
 *  @see EventSetListF()
 */
extern struct Set *EventSetListF(struct EventList *e);
/**<
 *  Implementation function for EventSetList() (debug mode).
 *  Do not call this function directly - use EventSetList() instead.
 */

#ifdef NDEBUG
#define EventStatementList(e) ((e)->slist)
#else
#define EventStatementList(e) EventStatementListF(e)
#endif
/**<
 *  Return the statement list.
 *  @param e struct EventList*, the event list to query.
 *  @return The statement list part as a struct StatementList*.
 *  @see EventStatementListF()
 */
extern struct StatementList *EventStatementListF(struct EventList *e);
/**<
 *  Implementation function for EventStatementList() (debug mode).
 *  Do not call this function directly - use EventStatementList() instead.
 */

extern void DestroyEventList(struct EventList *e);
/**< 
 *  Destroy a whole list.
 */

extern void DestroyEventNode(struct EventList *e);
/**< 
 *  Destroy just this node.
 */

extern struct EventList *CopyEventNode(struct EventList *e);
/**< 
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct EventList *CopyEventList(struct EventList *e);
/**< 
 *  Copy the whole list contents. not a reference count change.
 */

/* Event utility functions */

extern void ModifyEventPointers(struct gl_list_t *reforvar,
                                CONST struct Instance *old,
                                CONST struct Instance *new);
/**<
 *  Variable List or Case List Maintenance.
 *
 *  This requires some explanation. There are a number of cases
 *  to consider.
 *
 *  -# the old instance does not exist in the reforvar list -- do nothing.
 *
 *  -# the old instance exists, but the new does not -- store the
 *     the new instance in the slot where the old instance was and
 *     return.
 *
 *  -# the old instance exists, *and* the new instance also exists in
 *     the reforvar list.
 *
 *  -# the new instance is NULL, which can happen transiently during
 *     some operations. This defaults to case 2).
 */

extern struct gl_list_t *CopyEventBVarList(struct Instance *dest_inst,
                                           struct gl_list_t *copylist);
/**<
 *  Copy an Event list of variables.
 */

extern void DestroyEventVarList(struct gl_list_t *l, struct Instance *inst);
/**<
 *  Destroy an Event list of variables.
 */


extern void DestroyEventCaseList(struct gl_list_t *l, struct Instance *inst);
/**<
 *  Destroy a Event list of cases.
 */

/* Event output routines */

extern void WriteEvent(FILE *f, CONST struct Instance *eventinst,
                       CONST struct Instance *ref);
/**<
 *  Write an event instance to the file f.
 */

ASC_DLLSPEC char *WriteEventString(CONST struct Instance *eventinst,
                                   CONST struct Instance *ref);
/**<
 *  Write an event instance to a char.
 */

#endif  /* ASC_EVENT_H */

