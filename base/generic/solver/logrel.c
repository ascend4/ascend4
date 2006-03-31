/*
 *  Logical Relation Module
 *  by Vicente Rico-Ramirez
 *  Created: 09/96
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: logrel.c,v $
 *  Date last modified: $Date: 1998/01/29 00:42:26 $
 *  Last modified by: $Author: ballan $
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

#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/fractions.h>
#include <compiler/instance_enum.h>
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/dimen.h>
#include <compiler/types.h>
#include <compiler/find.h>
#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/mathinst.h>
#include <compiler/parentchild.h>
#include <compiler/instance_io.h>
#include <compiler/logical_relation.h>
#include <compiler/logrelation.h>
#include <compiler/logrel_util.h>
#include <compiler/packages.h>
#define _SLV_SERVER_C_SEEN_ /* for the extrel stuff in header */
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
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
   newlogrel = (struct logrel_relation *)
                               ascmalloc( sizeof(struct logrel_relation) );
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


void logrel_set_mindex( struct logrel_relation *logrel, int32 index)
{
   logrel->mindex = index;
}


int32 logrel_sindex( struct logrel_relation *logrel)
{
   return( logrel->sindex );
}

void logrel_set_sindex( struct logrel_relation *logrel, int32 index)
{
   logrel->sindex = index;
}


int32 logrel_model(const struct logrel_relation *logrel)
{
   return((const int32) logrel->model );
}


void logrel_set_model( struct logrel_relation *logrel, int32 index)
{
   logrel->model = index;
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
			 logrel_filter_t *filter)
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

/* to bad there's no entry point that rel must call before being used
 * generally, like the FindType checking stuff in var.c
 */
static void check_included_flag(void){
  if (INCLUDED_R == NULL || AscFindSymbol(INCLUDED_R) == NULL) {
    INCLUDED_R = AddSymbolL("included",8);
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
