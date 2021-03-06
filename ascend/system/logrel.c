/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//** @file
	Logical Relation Module.
*//*
	by Vicente Rico-Ramirez, 09/96
	Last in CVS: $Revision: 1.8 $ $Date: 1998/01/29 00:42:26 $ $Author: ballan $
*/

#include "logrel.h"

#include <math.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/mem.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>

#include <ascend/compiler/instance_enum.h>

#include <ascend/compiler/symtab.h>
#include <ascend/compiler/extfunc.h>
#include <ascend/compiler/extcall.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>

#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/find.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/logical_relation.h>
#include <ascend/compiler/logrelation.h>
#include <ascend/compiler/logrel_util.h>
#include <ascend/compiler/packages.h>

#include <ascend/linear/mtx.h>

#include "slv_server.h"

#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif
#define LOGREL_DEBUG FALSE

/* define symchar names needed */
static symchar *g_strings[1];
#define INCLUDED_R g_strings[0]


static const struct logrel_relation g_logrel_defaults = {
   NULL,		/* instance */
   NULL,		/* incidence */
   0,			/* n_incidences */
   -1,			/* mindex */
   -1,			/* sindex */
   -1,			/* model index */
   (LOGREL_INCLUDED)	/* flags */
};

/*
 *  Don't forget to update the
 *  initialization when the structure
 *  is modified.
 */


static struct logrel_relation
*logrel_copy(const struct logrel_relation *logrel)
{
   struct logrel_relation *newlogrel;
   newlogrel = ASC_NEW(struct logrel_relation);
   *newlogrel = *logrel;
   return(newlogrel);
}


struct logrel_relation *logrel_create(SlvBackendToken instance,
				      struct logrel_relation *newlogrel)
{
  if (newlogrel==NULL) {
    newlogrel = logrel_copy(&g_logrel_defaults); /* malloc the logrelation */
  } else {
    *newlogrel = g_logrel_defaults; /* init the space we've been sent */
  }
  assert(newlogrel!=NULL);
  newlogrel->instance = instance;
  return(newlogrel);
}



SlvBackendToken logrel_instance(struct logrel_relation *logrel)
{
  if (logrel==NULL) return NULL;
  return (SlvBackendToken) logrel->instance;
}

void logrel_write_name(slv_system_t sys,struct logrel_relation *logrel,
		       FILE *fp)
{
  if (logrel == NULL || fp==NULL) return;
  if (sys!=NULL) {
    WriteInstanceName(fp,logrel_instance(logrel),slv_instance(sys));
  } else {
    WriteInstanceName(fp,logrel_instance(logrel),NULL);
  }
}


void logrel_destroy(struct logrel_relation *logrel)
{
   struct Instance *inst;

   ascfree((POINTER)logrel->incidence);
   inst = IPTR(logrel->instance);
   if (inst) {
     if (GetInterfacePtr(inst)==logrel) {
       SetInterfacePtr(inst,NULL);
     }
   }
   ascfree((POINTER)logrel);
}


uint32 logrel_flags( struct logrel_relation *logrel)
{
  return logrel->flags;
}

void logrel_set_flags(struct logrel_relation *logrel, unsigned int flags)
{
  logrel->flags = flags;
}


uint32 logrel_flagbit(struct logrel_relation *logrel, uint32 one)
{
  if (logrel==NULL || logrel->instance == NULL) {
    FPRINTF(stderr,"ERROR: logrel_flagbit called with bad logrel.\n");
    return 0;
  }
  return (logrel->flags & one);
}


void logrel_set_flagbit(struct logrel_relation *logrel, uint32 field,
			uint32 one)
{
  if (one) {
    logrel->flags |= field;
  } else {
    logrel->flags &= ~field;
  }
}


boolean logrel_not_equal(struct logrel_relation *logrel)
{
  switch( LogRelRelop(GetInstanceLogRelOnly(IPTR(logrel->instance))) ) {
  case e_boolean_neq:
    return(TRUE);
  default:
    return(FALSE);
  }
}


boolean logrel_equal( struct logrel_relation *logrel)
{
  switch( LogRelRelop(GetInstanceLogRelOnly(IPTR(logrel->instance))) ) {
  case e_boolean_eq:
    return(TRUE);
  default:
    return(FALSE);
  }
}


static enum logrel_enum compenum2logrelenum(enum Expr_enum t)
{
  switch (t) {
  case e_boolean_eq:
    return e_logrel_equal;
  case e_boolean_neq:
    return e_logrel_not_equal;
  default:
    FPRINTF(ASCERR,"ERROR (logrel.c): compenum2logrelenum miscalled.\n");
    return e_logrel_equal;
  }
}


enum logrel_enum logrel_relop( struct logrel_relation *logrel)
{
  return
    compenum2logrelenum(LogRelRelop(
        GetInstanceLogRelOnly(IPTR(logrel->instance))));
}


char *logrel_make_name(slv_system_t sys,struct logrel_relation *logrel)
{
  return WriteInstanceNameString(IPTR(logrel->instance),
				 IPTR(slv_instance(sys)));
}


int32 logrel_mindex( struct logrel_relation *logrel)
{
   return( logrel->mindex );
}


void logrel_set_mindex( struct logrel_relation *logrel, int32 mindex)
{
   logrel->mindex = mindex;
}


int32 logrel_sindex( struct logrel_relation *logrel)
{
   return( logrel->sindex );
}

void logrel_set_sindex( struct logrel_relation *logrel, int32 sindex)
{
   logrel->sindex = sindex;
}


int32 logrel_model(const struct logrel_relation *logrel)
{
   return((const int32) logrel->model );
}


void logrel_set_model( struct logrel_relation *logrel, int32 mindex)
{
   logrel->model = mindex;
}


int32 logrel_residual(struct logrel_relation *logrel)
{
   return( LogRelResidual(GetInstanceLogRelOnly(IPTR(logrel->instance))));
}


void logrel_set_residual( struct logrel_relation *logrel, int32 residual)
{
  struct logrelation *logreln;
  logreln = (struct logrelation *)
            GetInstanceLogRelOnly(IPTR(logrel->instance));
  SetLogRelResidual(logreln,residual);
}



int32 logrel_nominal( struct logrel_relation *logrel)
{
  return( LogRelNominal(GetInstanceLogRelOnly(IPTR(logrel->instance))));
}


int32 logrel_n_incidencesF(struct logrel_relation *logrel)
{
  if (logrel!=NULL) return logrel->n_incidences;
  FPRINTF(stderr,"logrel_n_incidences miscalled with NULL\n");
  return 0;
}


void logrel_set_incidencesF(struct logrel_relation *logrel,int n,
                            struct dis_discrete **bv)
{
  if(logrel!=NULL && n >=0) {
    if (n && bv==NULL) {
      FPRINTF(stderr,"logrel_set_incidence miscalled with NULL list\n");
    }
    logrel->n_incidences = n;
    logrel->incidence = bv;
    return;
  }
  FPRINTF(stderr,"logrel_set_incidence miscalled with NULL or n < 0\n");
}


const struct dis_discrete
**logrel_incidence_list( struct logrel_relation *logrel)
{
  if (logrel==NULL) return NULL;
  return( (const struct dis_discrete **)logrel->incidence );
}


struct dis_discrete
**logrel_incidence_list_to_modify( struct logrel_relation *logrel)
{
  if (logrel==NULL) return NULL;
  return( (struct dis_discrete **)logrel->incidence );
}


int logrel_apply_filter( struct logrel_relation *logrel,
			 const logrel_filter_t *filter)
{
  if (logrel==NULL || filter==NULL) {
    FPRINTF(stderr,"logrel_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  /* AND to mask off irrelevant bits in flags and match value,
     then compare */
  return ( (filter->matchbits & logrel->flags) ==
           (filter->matchbits & filter->matchvalue) );
}

/* too bad there's no entry point that rel must call before being used
 * generally, like the FindType checking stuff in var.c
 */
static void check_included_flag(void){
  if (INCLUDED_R == NULL || AscFindSymbol(INCLUDED_R) == NULL) {
    INCLUDED_R = AddSymbol("included");
  }
}

uint32 logrel_included( struct logrel_relation *logrel)
{
   struct Instance *c;
   check_included_flag();
   c = ChildByChar(IPTR(logrel->instance),INCLUDED_R);
   if( c == NULL ) {
      FPRINTF(stderr,"ERROR:  (logrel) logrel_included\n");
      FPRINTF(stderr,"        No 'included' field in logrelation.\n");
      WriteInstance(stderr,IPTR(logrel->instance));
      return FALSE;
   }
   logrel_set_flagbit(logrel,LOGREL_INCLUDED,GetBooleanAtomValue(c));
   return( GetBooleanAtomValue(c) );
}


void logrel_set_included( struct logrel_relation *logrel, uint32 included)
{
   struct Instance *c;
   check_included_flag();
   c = ChildByChar(IPTR(logrel->instance),INCLUDED_R);
   if( c == NULL ) {
      FPRINTF(stderr,"ERROR:  (logrel) logrel_set_included\n");
      FPRINTF(stderr,"        No 'included' field in logrelation.\n");
      WriteInstance(stderr,IPTR(logrel->instance));
      return;
   }
   SetBooleanAtomValue(c,included,(unsigned)0);
   logrel_set_flagbit(logrel,LOGREL_INCLUDED,included);
}
