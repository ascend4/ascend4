/*
 *  Conditional Module
 *  by Vicente Rico-Ramirez
 *  Created: 09/96
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: conditional.c,v $
 *  Date last modified: $Date: 1998/03/30 22:06:52 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/instance_enum.h>
#include <compiler/compiler.h>
#include <compiler/child.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/type_desc.h>
#include <compiler/atomvalue.h>
#include <compiler/parentchild.h>
#include <compiler/instquery.h>
#include <compiler/instance_io.h>
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_server.h"
#include "analyze.h"


#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif
#define WHEN_DEBUG FALSE

/*
 *                        When utility functions
 */

/* forward declaration */
void when_case_destroy(struct when_case *);

static const struct w_when g_when_defaults = {
   NULL,		/* instance */
   NULL,		/* variables */
   NULL,		/* cases */
   -1,		        /* number of cases */
   -1,			/* mindex */
   -1,			/* sindex */
   -1,			/* model index */
   (WHEN_INCLUDED)	/* flags */
};


static struct w_when *when_copy(const struct w_when *when)
{
   struct w_when *newwhen;
   newwhen = (struct w_when *)ascmalloc( sizeof(struct w_when) );
   *newwhen = *when;
   return(newwhen);
}


struct w_when *when_create(SlvBackendToken instance,
			   struct w_when *newwhen)
{
  if (newwhen==NULL) {
    newwhen = when_copy(&g_when_defaults); /* malloc the when */
  } else {
    *newwhen = g_when_defaults;        /* init the space we've been sent */
  }
  assert(newwhen!=NULL);
  newwhen->instance = instance;
  return(newwhen);
}


void when_destroy_cases(struct w_when *when)
{
  int32 c,len;
  struct when_case *cur_case;

  len = gl_length(when->cases);
  for(c=1;c<=len;c++){
    cur_case = (struct when_case *)(gl_fetch(when->cases,c));
    when_case_destroy(cur_case);
  }
  gl_destroy(when->cases);
}


void when_destroy(struct w_when *when)
{
   struct Instance *inst;
   if (when==NULL) return;
   if (when->dvars != NULL) {
     gl_destroy(when->dvars);
     when->dvars = NULL;
   }
   if (when->cases != NULL) {
     when_destroy_cases(when);
     when->cases = NULL;
   }
   inst = IPTR(when->instance);
   if (inst) {
     if (GetInterfacePtr(inst)==when) {
       SetInterfacePtr(inst,NULL);
     }
   }
}

SlvBackendToken when_instance(struct w_when *when)
{
  if (when==NULL) return NULL;
  return (SlvBackendToken) when->instance;
}


void when_write_name(slv_system_t sys,struct w_when *when,
		     FILE *fp)
{
  if (when == NULL || fp==NULL) return;
  if (sys!=NULL) {
    WriteInstanceName(fp,when_instance(when),slv_instance(sys));
  } else {
    WriteInstanceName(fp,when_instance(when),NULL);
  }
}


struct gl_list_t *when_dvars_list( struct w_when *when)
{
   assert(when);
   return( when->dvars );
}


void when_set_dvars_list( struct w_when *when, struct gl_list_t *dvlist)
{
   assert(when);
   when->dvars = dvlist;
}


struct gl_list_t *when_cases_list( struct w_when *when)
{
   assert(when);
   return( when->cases );
}


void when_set_cases_list( struct w_when *when, struct gl_list_t *clist)
{
   assert(when);
   when->cases = clist;
}

int32 when_num_cases(struct w_when *when)
{
   return( when->num_cases );
}

void when_set_num_cases( struct w_when *when, int32 num_cases)
{
   when->num_cases = num_cases;
}

int32 when_mindex( struct w_when *when)
{
   return( when->mindex );
}

void when_set_mindex( struct w_when *when, int32 index)
{
   when->mindex = index;
}


int32 when_sindex( struct w_when *when)
{
   return( when->sindex );
}

void when_set_sindex( struct w_when *when, int32 index)
{
   when->sindex = index;
}

int32 when_model(const struct w_when *when)
{
   return((const int32) when->model );
}

void when_set_model( struct w_when *when, int32 index)
{
   when->model = index;
}

int32 when_apply_filter(struct w_when *w,
                        when_filter_t *filter)
{
  if (w==NULL || filter==NULL) {
    FPRINTF(stderr,"when_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & w->flags) ==
           (filter->matchbits & filter->matchvalue) );
}


uint32 when_flags( struct w_when *when)
{
  return when->flags;
}

void when_set_flags(struct w_when *when, uint32 flags)
{
  when->flags = flags;
}

uint32 when_flagbit(struct w_when *when, uint32 one)
{
  if (when==NULL || when->instance == NULL) {
    FPRINTF(stderr,"ERROR: when_flagbit called with bad when.\n");
    return 0;
  }
  return (when->flags & one);
}

void when_set_flagbit(struct w_when *when, uint32 field,
		      uint32 one)
{
  if (one) {
    when->flags |= field;
  } else {
    when->flags &= ~field;
  }
}


/*
 *                  When Case  utility functions
 */


/* we are depending on ansi initialization to 0 for the
 * values field OF this struct.
 */
static
const struct when_case g_case_defaults = {
   {0},			/* values */
   NULL,		/* relations */
   NULL,		/* logrelations */
   NULL,		/* whens */
   -1,                  /* case number */
   -1,                  /* number relations */
   -1,                  /* number vars */
   NULL,                /* master indeces of incidente vars */
   (0x0)
};


static struct when_case *when_case_copy(const struct when_case *wc)
{
   struct when_case *newcase;
   newcase = (struct when_case *)ascmalloc( sizeof(struct when_case) );
   *newcase = *wc;
   return(newcase);
}


struct when_case *when_case_create(struct when_case *newcase)
{
  if (newcase==NULL) {
    newcase = when_case_copy(&g_case_defaults); /* malloc the case */
  } else {
    *newcase = g_case_defaults;    /* init the space we've been sent */
  }
  return(newcase);
}


void when_case_destroy(struct when_case *wc)
{
   if (wc->rels != NULL) {
     gl_destroy(wc->rels);
     wc->rels = NULL;
   }
   if (wc->logrels != NULL) {
     gl_destroy(wc->logrels);
     wc->logrels = NULL;
   }
   if (wc->whens != NULL ) {
     gl_destroy(wc->whens);
     wc->whens = NULL;
   }
   if (wc->ind_inc != NULL ) {
     ascfree(wc->ind_inc);
   }

   ascfree((POINTER)wc);
}


int32 *when_case_values_list( struct when_case *wc)
{
   assert(wc);
   return( &(wc->values[0]) );
}


void when_case_set_values_list( struct when_case *wc, int32 *vallist)
{
   int32 *value,index;
   assert(wc);
   value = &(wc->values[0]);
   for(index=0;index<MAX_VAR_IN_LIST;index++) {
      *value = *vallist;
      value++;
      vallist++;
   }
}


struct gl_list_t *when_case_rels_list( struct when_case *wc)
{
   assert(wc);
   return( wc->rels );
}


void when_case_set_rels_list( struct when_case *wc, struct gl_list_t *rlist)
{
   assert(wc);
   wc->rels = rlist;
}


struct gl_list_t *when_case_logrels_list( struct when_case *wc)
{
   assert(wc);
   return( wc->logrels );
}


void when_case_set_logrels_list(struct when_case *wc,
                                struct gl_list_t *lrlist)
{
   assert(wc);
   wc->logrels = lrlist;
}


struct gl_list_t *when_case_whens_list( struct when_case *wc)
{
   assert(wc);
   return( wc->whens );
}


void when_case_set_whens_list( struct when_case *wc, struct gl_list_t *wlist)
{
   assert(wc);
   wc->whens = wlist;
}


int32 when_case_case_number( struct when_case *wc)
{
  assert(wc);
  return wc->case_number;
}

void when_case_set_case_number(struct when_case *wc, int32 case_number)
{
  assert(wc);
  wc->case_number = case_number;
}

int32 when_case_num_rels( struct when_case *wc)
{
  assert(wc);
  return wc->num_rels;
}

void when_case_set_num_rels(struct when_case *wc, int32 num_rels)
{
  assert(wc);
  wc->num_rels = num_rels;
}

int32 when_case_num_inc_var( struct when_case *wc)
{
  assert(wc);
  return wc->num_inc_var;
}

void when_case_set_num_inc_var(struct when_case *wc, int32 num_inc_var)
{
  assert(wc);
  wc->num_inc_var = num_inc_var;
}

int32 *when_case_ind_inc( struct when_case *wc)
{
  assert(wc);
  return wc->ind_inc;
}

void when_case_set_ind_inc(struct when_case *wc, int32* ind_inc)
{
  assert(wc);
  wc->ind_inc = ind_inc;
}

int32 when_case_apply_filter(struct when_case *wc,
                           when_case_filter_t *filter)
{
  if (wc==NULL || filter==NULL) {
    FPRINTF(stderr,"when_case_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & wc->flags) ==
           (filter->matchbits & filter->matchvalue) );
}

uint32 when_case_flags( struct when_case *wc)
{
  assert(wc);
  return wc->flags;
}

void when_case_set_flags(struct when_case *wc, uint32 flags)
{
  assert(wc);
  wc->flags = flags;
}

uint32 when_case_flagbit(struct when_case *wc, uint32 one)
{
  if (wc==NULL) {
    FPRINTF(stderr,"ERROR: when_case_flagbit called with bad case.\n");
    return 0;
  }
  return (wc->flags & one);
}

void when_case_set_flagbit(struct when_case *wc, uint32 field,
		           uint32 one)
{
  if (one) {
    wc->flags |= field;
  } else {
    wc->flags &= ~field;
  }
}


