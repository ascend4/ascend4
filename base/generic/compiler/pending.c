/*
 *  Ascend Pending Instance Routines
 *  by Tom Epperly
 *  Created: 1/24/90
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: pending.c,v $
 *  Date last modified: $Date: 1998/01/27 11:00:08 $
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
 *
 */
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/pool.h>
#include "bit.h"
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "stattypes.h"
#include "instance_enum.h"
#include "instance_types.h"
#include "instquery.h"
#include "visitinst.h"
#include "instantiate.h"
#include "pending.h"

#ifndef lint
static CONST char PendingRCSid[] = "$Id: pending.c,v 1.9 1998/01/27 11:00:08 ballan Exp $";
#endif

#define FAST_PENDINGS 1
/* if FAST_PENDINGS, then the instance and pendings are mutually referred.
 * this makes some operations faster and many bitlist operation unneeded.
 * This file needs to have the old pending implementation stripped out
 * once FAST_PENDINGS has been fully qualified by user testing.
 */

unsigned long g_unresolved_count=0;
static unsigned long g_pending_count=0;
static struct pending_t *g_pending_list = NULL;
static struct pending_t *g_pending_list_end = NULL;

/* forward declarations we do NOT want to header */
static void RemoveFromList(struct pending_t *);
/*********************************************************************\
void RemoveFromList(pt)
struct pending_t *pt;

This deallocates and removes pt from the list.  Clients should *NEVER*
free a pending_t pointer themselves.  It should always be deallocated by
pending list module routines.
\*********************************************************************/

static pool_store_t g_pending_pool = NULL;
/* A pool_store for pending list elements.
 * The pool should be reset/restarted every time the pending list
 * is emptied. Be sure no instances have ptrs to pending_t
 * before clearing pool.
 */

#define POOL_ALLOCPEND (struct pending_t *)(pool_get_element(g_pending_pool))
/* get a token. Token is the size of the struct pending_t */
#define POOL_FREEPEND(p) (pool_free_element(g_pending_pool,((void *)p)))
/* return a pending_t */
#define POOL_RESET pool_clear_store(g_pending_pool)
/* reset the pool for next time, if you can figure out when
   'this time' is. Probably not critical. */

#define PP_LEN 2
#if (SIZEOF_VOID_P == 8)
#define PP_WID 41
#else
#define PP_WID 84
#endif
/* retune rpwid if the size of pending_t changes dramatically */
#define PP_ELT_SIZE (sizeof(struct pending_t))
#define PP_MORE_ELTS 1
/* Number of slots filled if more elements needed.
   So if the pool grows, it grows by PP_MORE_ELTS*PP_WID elements at a time. */
#define PP_MORE_BARS 50
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown.
   One could also recall these every time there is a delete all types. */
void InitPendingPool(void) {
  if (g_pending_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: InitPendingPool called twice.\n");
  }
  g_pending_pool =
    pool_create_store(PP_LEN, PP_WID, PP_ELT_SIZE, PP_MORE_ELTS, PP_MORE_BARS);
  if (g_pending_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: InitPendingPool unable to allocate pool.\n");
  }
}

void DestroyPendingPool(void) {
  if (g_pending_pool==NULL) return;
  pool_destroy_store(g_pending_pool);
  g_pending_pool = NULL;
}

void ReportPendingPool(FILE *f)
{
  if (g_pending_pool==NULL) {
    FPRINTF(f,"PendingPool is empty\n");
  }
  FPRINTF(f,"PendingPool ");
  pool_print_store(f,g_pending_pool,0);
}

#ifndef NDEBUG
static
int NotDuplicated(struct pending_t *pl, struct Instance *inst)
{
  while (pl!=NULL){
    AssertContainedMemory(pl,sizeof(struct pending_t));
    if (pl->inst == inst) {
      return 0;
    }
    pl = pl->next;
  }
  return 1;
}

static
unsigned long CalcNumberPending(void)
{
  register struct pending_t *ptr;
  register unsigned long c=0;
  ptr = g_pending_list;
  while (ptr){
    AssertContainedMemory(ptr,sizeof(struct pending_t));
    c++;
    ptr = ptr->next;
  }
  return c;
}
#endif

struct Instance *PendingInstanceF(CONST struct pending_t *pt)
{
  assert(pt!=NULL);
  AssertContainedMemory(pt,sizeof(struct pending_t));
  return pt->inst;
}

void ClearList(void)
{
  register struct pending_t *ptr,*next;

  ptr = g_pending_list;
  g_pending_list_end = g_pending_list = NULL;
  while(ptr!=NULL){
    next = ptr->next;
#if FAST_PENDINGS
    if (ptr->inst != NULL) {
#ifndef NDEBUG
      if (ptr != ((struct PendInstance *)(ptr->inst))->p) {
	FPRINTF(ASCERR,"Misdirected pending_entry in instance!\n");
      } else
#endif
             {
        ((struct PendInstance *)(ptr->inst))->p = NULL;
      }
    }
#endif
    POOL_FREEPEND(ptr);
    ptr = next;
  }
  g_pending_count = 0;
}

unsigned long NumberPending(void)
{
  assert(g_pending_count == CalcNumberPending());
  return g_pending_count;
}

void AddBelow(struct pending_t *pt,
	      struct Instance *i)
{
  struct pending_t *new;
  assert(NotDuplicated(g_pending_list,i));
#if FAST_PENDINGS
#ifndef NDEBUG
  if (InstanceKind(i) != MODEL_INST &&
      InstanceKind(i) != ARRAY_ENUM_INST &&
      InstanceKind(i) != ARRAY_INT_INST ) {
    FPRINTF(ASCERR,
	"Error: Attempt to add noncompound instance to pending list.\n");
    return;
  }
#endif
#endif

  new = POOL_ALLOCPEND;
  if (pt){
    AssertContainedMemory(pt,sizeof(struct pending_t));
    if ( (new->next = pt->next)!=NULL ) {
      new->next->prev = new;
    }
    pt->next = new;
    new->prev = pt;
    if (g_pending_list_end == pt) {
      g_pending_list_end = new;
    }
  } else {
    new->next = g_pending_list;
    new->prev = NULL;
    if (g_pending_list){
      AssertContainedMemory(g_pending_list,sizeof(struct pending_t));
      g_pending_list->prev = new;
    } else {
      g_pending_list_end = new;
    }
    g_pending_list = new;
  }
  new->inst = i;
#if FAST_PENDINGS
#ifndef NDEBUG
  if ( ((struct PendInstance *)i)->p != NULL) {
    FPRINTF(ASCERR,
            "ERROR: Instance apparently added to pending list again.\n");
  } else
#endif
         {
    ((struct PendInstance *)i)->p = new;
  }
#endif
  g_pending_count++;
}

void AddToEnd(struct Instance *i)
{
  register struct pending_t *ptr;
  assert(NotDuplicated(g_pending_list,i));
#if FAST_PENDINGS
#ifndef NDEBUG
  if (InstanceKind(i) != MODEL_INST &&
      InstanceKind(i) != ARRAY_ENUM_INST &&
      InstanceKind(i) != ARRAY_INT_INST ) {
    FPRINTF(ASCERR,
	"Error: Attempt to add noncompound instance to pending list.\n");
    return;
  }
#endif
#endif
  if (g_pending_list_end){
    ptr = POOL_ALLOCPEND;
    AssertContainedMemory(g_pending_list_end,sizeof(struct pending_t));
    g_pending_list_end->next = ptr;
    ptr->next = NULL;
    ptr->inst = i;
#if FAST_PENDINGS
#ifndef NDEBUG
    if ( ((struct PendInstance *)i)->p != NULL) {
      FPRINTF(ASCERR,
	"ERROR: Instance apparently added to pending list again.\n");
    } else
#endif
#endif
           {
      ((struct PendInstance *)i)->p = ptr;
    }
    ptr->prev = g_pending_list_end;
    g_pending_list_end = ptr;
    g_pending_count++;
  } else {
    AddBelow(NULL,i);
  }
}


#if FAST_PENDINGS
void PendingInstanceRealloced(struct Instance *old, struct Instance *new)
{
  register struct pending_t *ptr;
  if (g_pending_list!=NULL){
    ptr = ((struct PendInstance *)(new))->p;
    /* handshake */
    /* goodness this is assumption ridden. The use of this function
       is to fix up after a realloc. This means that new copied the
       value of pending_entry field from old already and we can
       skip fixing up new->p. Further it is assumed that old is or
       very shortly will be out of existence so we cannot safely
       (nor do we need to) set old->p = NULL.
    */
    if (ptr == NULL) return; /* why are we here? */
    AssertContainedMemory(ptr,sizeof(struct pending_t));
    if (ptr->inst==old){
      ptr->inst = new;
      /* ((struct PendInstance *)(new))->p = ptr;  assumed done already. */
    } else {
      FPRINTF(ASCERR,"Bogus ChangeInstance call to pendings.c\n");
    }
  }
}
#else
void PendingInstanceRealloced(struct Instance *old, struct Instance *new)
{
  register struct pending_t *ptr;
  if (g_pending_list!=NULL){
    ptr = g_pending_list;
    while(ptr!=NULL){
      AssertContainedMemory(ptr,sizeof(struct pending_t));
      if (ptr->inst==old){
	ptr->inst = new;
	assert(NotDuplicated(ptr,old));
	return;
      }
      ptr = ptr->next;
    }
  }
}
#endif

#if FAST_PENDINGS
void RemoveInstance(struct Instance *i)
{
  register struct pending_t *ptr;
  if (g_pending_list!=NULL) {
#ifndef NDEBUG
    if (InstanceKind(i) != MODEL_INST &&
        InstanceKind(i) != ARRAY_ENUM_INST &&
        InstanceKind(i) != ARRAY_INT_INST ) {
      FPRINTF(ASCERR,
	  "Error: Attempt to remove noncompound instance in pending list.\n");
      return;
    }
#endif
    ptr = ((struct PendInstance *)(i))->p;
    if (ptr != NULL && ptr->inst == i){
      RemoveFromList(ptr);
      ((struct PendInstance *)(i))->p = NULL;
    } else {
      FPRINTF(ASCERR,"ERROR: Bogus instance to RemoveInstance in pending.c\n");
    }
  } else {
    FPRINTF(ASCERR,
      "ERROR: No pendings in call to RemoveInstance in pending.c\n");
  }
}
#else
void RemoveInstance(struct Instance *i)
{
  register struct pending_t *ptr;
  ptr = g_pending_list;
  while(ptr){
    if (ptr->inst == i){
      RemoveFromList(ptr);
      assert(NotDuplicated(g_pending_list,i));
      break;
    }
    ptr = ptr->next;
  }
}
#endif

static void RemoveFromList(struct pending_t *pt)
{
  if (pt){
    AssertContainedMemory(pt,sizeof(struct pending_t));
    if (g_pending_list == pt){
      if ((g_pending_list = pt->next)!=NULL) {
	g_pending_list->prev = NULL;
      } else {
	g_pending_list_end = NULL;
      }
    }
    else if (g_pending_list_end == pt){
      g_pending_list_end = pt->prev;
      g_pending_list_end->next = NULL;
    }
    else{
      AssertContainedMemory(pt->prev,sizeof(struct pending_t));
      pt->prev->next = pt->next;
      AssertContainedMemory(pt->next,sizeof(struct pending_t));
      pt->next->prev = pt->prev;
    }
#ifndef NDEBUG
    pt->inst = NULL;
    pt->next = NULL;
    pt->prev = NULL;
#endif
    POOL_FREEPEND(pt);
    g_pending_count--;
  }
}

#if FAST_PENDINGS
int InstanceInList(struct Instance *i)
{
  register struct pending_t *ptr;
#ifndef NDEBUG
  if (InstanceKind(i) != MODEL_INST &&
      InstanceKind(i) != ARRAY_ENUM_INST &&
      InstanceKind(i) != ARRAY_INT_INST ) {
    FPRINTF(ASCERR,
      "Error: Attempt to find noncompound instance to pending list.\n");
    return 0;
  }
#endif
  ptr = ((struct PendInstance *)(i))->p;
  if (ptr!=NULL){
    if (ptr->inst==i) {
      return 1;
    } else {
      FPRINTF(ASCERR,"ERROR: Incorrect handshake in pending.c\n");
      return 0;
    }
  }
  return 0;
}
#else
int InstanceInList(struct Instance *i)
{
  register struct pending_t *ptr;
  ptr = g_pending_list;
  while(ptr!=NULL){
    if (ptr->inst==i) return 1;
    ptr = ptr->next;
  }
  return 0;
}
#endif
struct pending_t *TopEntry(void)
{
  return g_pending_list;
}

struct pending_t *BottomEntry(void)
{
  return g_pending_list_end;
}

struct pending_t *ListEntry(unsigned long n)
{
  struct pending_t *pt;
  if (!n || n > g_pending_count) return NULL;
  pt = g_pending_list;
  while(pt && --n){
    pt = pt->next;
  }
  return pt;
}

void MoveToBottom(struct pending_t *pt)
{
  if (g_pending_list_end == pt) return;
  if (g_pending_list == pt){
    g_pending_list = pt->next;
    g_pending_list->prev = NULL;
  }
  else{
    pt->prev->next = pt->next;
    pt->next->prev = pt->prev;
  }
  g_pending_list_end->next = pt;
  pt->next = NULL;
  pt->prev = g_pending_list_end;
  g_pending_list_end = pt;
}

/**********************************************************\
  The following are utilities for outsiders.
\**********************************************************/

#if FAST_PENDINGS
static
void CheckForUnresolved(struct Instance *i)
{
  if (!i) {
    FPRINTF(ASCERR,"null child pointer in CheckForUnresolved\n");
    return;
  }
  if (InstanceKind(i)==MODEL_INST && ((struct PendInstance *)(i))->p != NULL) {
    g_unresolved_count++;
    return;
  }
  if ( (InstanceKind(i)==ARRAY_ENUM_INST || InstanceKind(i)==ARRAY_INT_INST) ){
    if ( IncompleteArray(i) ) {
      g_unresolved_count++;
    }
  }
}

unsigned long NumberPendingInstances(struct Instance *i)
{
  if (i==NULL) {
    FPRINTF(ASCERR,"null pointer given to NumberPendingInstances\n");
    return 1; /* the instance must be pending, eh? */
  }
  g_unresolved_count = 0L;
  VisitInstanceTree(i,CheckForUnresolved,1,0);
  return g_unresolved_count;
}
#else
void CheckForUnresolved(struct Instance *i)
{
  struct BitList *blist;
  if (!i) {
    FPRINTF(ASCERR,"null child pointer in CheckForUnresolved\n");
    return;
  }
  if (((blist = InstanceBitList(i)) && (!BitListEmpty(blist)))||
      IncompleteArray(i)){
    g_unresolved_count++;
  }
}

unsigned long NumberPendingInstances(struct Instance *i)
{
  if (i==NULL) {
    FPRINTF(ASCERR,"null pointer given to NumberPendingInstances\n");
    return 1; /* the instance must be pending, eh? */
  }
  g_unresolved_count = 0L;
  VisitInstanceTree(i,CheckForUnresolved,1,0);
  return g_unresolved_count;
}
#endif
