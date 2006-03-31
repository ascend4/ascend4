/*
 *  Discrete Variable Module
 *  by Vicente Rico-Ramirez
 *  Created: 06/96
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: discrete.c,v $
 *  Date last modified: $Date: 1998/02/05 15:59:21 $
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

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/symtab.h>
#include <compiler/child.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/type_desc.h>
#include <compiler/atomvalue.h>
#include <compiler/parentchild.h>
#include <compiler/instquery.h>
#include <compiler/instance_io.h>
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_server.h"
#include "slv_common.h"
#include "slv_client.h"
#include "analyze.h"


#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif

/* useful symbol table things to know */
#define FIXED_V g_strings[0]
#define NOMINAL_V g_strings[1]

/*
 * array of those symbol table entries we need.
 */
static symchar * g_strings[2];

static const struct dis_discrete g_dis_defaults = {
   e_dis_error_t,       /* kind */
   NULL,		/* instance datom */
   NULL,		/* sos */
   NULL,		/* source */
   NULL,		/* whens */
   -1,			/* range */
   -1,			/* value */
   -1,			/* pre_value */
   -1,			/* mindex */
   -1,			/* sindex */
   0            	/* flags */
};


static struct dis_discrete *dis_copy(const struct dis_discrete *dis)
{
   struct dis_discrete *newdis;
   newdis = (struct dis_discrete *)ascmalloc( sizeof(struct dis_discrete) );
   *newdis = *dis;
   return(newdis);
}


struct dis_discrete *dis_create(SlvBackendToken instance,
			        struct dis_discrete *newdis)
{
  if (newdis==NULL) {
    newdis = dis_copy(&g_dis_defaults);    /* malloc the discrete var */
  } else {
    *newdis = g_dis_defaults;            /* init the space we've been sent */
  }
  assert(newdis!=NULL);
  newdis->datom = instance;
  return(newdis);
}


SlvBackendToken dis_instanceF(const struct dis_discrete *dis)
{ if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_instance called on bad dvar\n");
    return NULL;
  }
  return dis->datom;
}

void dis_set_instanceF(struct dis_discrete *dis, SlvBackendToken i)
{
  if (dis==NULL)  {
    FPRINTF(stderr,"dis_set_instance called on NULL dvar\n");
    return;
  }
  dis->datom = i;
}


char *dis_make_name(const slv_system_t sys,const  struct dis_discrete *dis)
{
  return WriteInstanceNameString(IPTR(dis->datom),IPTR(slv_instance(sys)));
}


char *dis_make_xname(const struct dis_discrete *dis)
{
   static char name[81];
   char *res;
   sprintf(name,"dis%d",dis_sindex(dis));
   res=(char *)ascmalloc((strlen(name)+1)*sizeof(char));
   sprintf(res,"%s",name);
   return res;
}


void dis_write_name(const slv_system_t sys,
                    const struct dis_discrete *dis,FILE *fp)
{
  if (dis == NULL || fp==NULL) return;
  if (sys!=NULL) {
    WriteInstanceName(fp,dis_instance(dis),slv_instance(sys));
  } else {
    WriteInstanceName(fp,dis_instance(dis),NULL);
  }
}


void dis_destroy(struct dis_discrete *dis)
{
  if (dis==NULL) return;
  dis->datom = NULL;
  if (dis->whens != NULL) {
    gl_destroy(dis->whens);
    dis->whens = NULL;
  }
}


struct gl_list_t *dis_whens_list(struct dis_discrete *dis)
{
   assert(dis);
   return( dis->whens );
}


void dis_set_whens_list( struct dis_discrete *dis, struct gl_list_t *wlist)
{
   assert(dis);
   dis->whens = wlist;
}


enum discrete_kind dis_kindF(const struct dis_discrete *dis)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_kind called on bad dis var\n");
    return  e_dis_error_t;
  }
  return dis->t;
}

void dis_set_kindF(struct dis_discrete *dis, enum discrete_kind kind)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_kind called on bad dis var\n");
    return;
  }
  dis->t = kind;
}


int32 dis_mindexF(const struct dis_discrete *dis)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_mindex called on bad dis var\n");
    return -1;
  }
  return dis->mindex;
}


void dis_set_mindexF(struct dis_discrete *dis, int32 index)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_mindex called on bad dis var\n");
    return;
  }
  dis->mindex = index;
}

int32 dis_sindexF(const struct dis_discrete *dis)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_sindex called on bad dis var\n");
    return -1;
  }
  return dis->sindex;
}


void dis_set_sindexF(struct dis_discrete *dis, int32 index)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_sindex called on bad dis\n");
    return;
  }
  dis->sindex = index;
}


int32 dis_value(const struct dis_discrete *dis)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_value called on bad dis\n");
    return 0;
  }
  return(dis->cur_value);
}


void dis_set_value(struct dis_discrete *dis, int32 value)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_value called on bad dis\n");
    return;
  }
  dis->pre_value = dis_value(dis);
  dis->cur_value = value;
}


void dis_set_inst_and_field_value(struct dis_discrete *dis, int32 value)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_inst_and_field_value called on bad dis\n");
    return;
  }
  if (dis_const(dis)) {
    return;
  }
  switch(dis->t) {
  case e_dis_boolean_t:
    dis->pre_value = dis_value(dis);
    SetBooleanAtomValue(dis->datom,value,(unsigned)0);
    dis->cur_value = GetBooleanAtomValue(dis->datom);
    break;
  case e_dis_integer_t:
    dis->pre_value = dis_value(dis);
    SetIntegerAtomValue(dis->datom,value,(unsigned)0);
    dis->cur_value = GetIntegerAtomValue(dis->datom);
    break;
  case e_dis_symbol_t:
    break;
  default:
    FPRINTF(stderr,"dis_set_inst_value called on bad dis\n");
    return;
  }
}


void dis_set_value_from_inst(struct dis_discrete *dis,
			     struct gl_list_t *symbol_list)
{
  CONST char *symval;

  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_value_from_inst called on bad dis\n");
    return;
  }
  if (dis_const(dis)) {
    return;
  }
  switch(dis->t) {
  case e_dis_boolean_t:
    dis->pre_value = dis_value(dis);
    dis->cur_value = GetBooleanAtomValue(dis->datom);
    break;
  case e_dis_integer_t:
    dis->pre_value = dis_value(dis);
    dis->cur_value = GetIntegerAtomValue(dis->datom);
    break;
  case e_dis_symbol_t:
    dis->pre_value = dis_value(dis);
    symval = SCP(GetSymbolAtomValue(dis->datom));
    dis->cur_value = GetIntFromSymbol(symval,symbol_list);
    break;
  default:
    FPRINTF(stderr,"dis_set_value_from_inst called on bad dis\n");
    return;
  }
}


int32 dis_previous_value(const struct dis_discrete *dis)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_previous_value called on bad dis\n");
    return 0;
  }
  return(dis->pre_value);
}


void dis_set_previous_value(struct dis_discrete *dis, int32 value)
{
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_previous_value called on bad dis\n");
    return;
  }
  dis->pre_value = value;
}



static struct TypeDescription *g_solver_dis_type;

boolean set_boolean_types(void)
{
  boolean nerr = 0;
  if( (g_solver_dis_type = FindType(AddSymbol(BOOLEAN_VAR_STR))) == NULL ) {
    FPRINTF(stderr,"ERROR:  (dis.c) set_boolean_types\n");
    FPRINTF(stderr,"        Type boolean_var not defined.\n");
    nerr++;
  }
  NOMINAL_V = AddSymbolL("nominal",7);
  FIXED_V = AddSymbolL("fixed",5);
  return nerr;
}


boolean boolean_var( SlvBackendToken inst)
{
  struct TypeDescription *type;
  if (!g_solver_dis_type) return FALSE;
  type = InstanceTypeDesc(IPTR(inst));
  return( type == MoreRefined(type,g_solver_dis_type) );
}


int32 dis_nominal(struct dis_discrete *dis)
{
  struct Instance *c;
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_nominal called on bad dis\n");
    return 1;
  }
  if (!boolean_var(dis->datom)) {
    FPRINTF(stderr,"dis_nominal called on a non-boolean_var\n");
    return 1;
  }
  c = ChildByChar(dis->datom,NOMINAL_V);
  if( c == NULL ) {
    FPRINTF(stderr,"ERROR:  (dis) dis_nominal\n");
    FPRINTF(stderr,"        No 'nominal' field in variable.\n");
    WriteInstance(stderr,IPTR(dis->datom));
    return 1;
  }
  return( GetBooleanAtomValue(c) );
}

void dis_set_nominal(struct dis_discrete *dis, int32 nominal)
{
  struct Instance *c;
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_nominal called on bad dis\n");
    return;
  }
  if (!boolean_var(dis->datom)) {
    FPRINTF(stderr,"dis_set_nominal called on a non-boolean_var\n");
    return;
  }
  c = ChildByChar(IPTR(dis->datom),NOMINAL_V);
  if( c == NULL ) {
    FPRINTF(stderr,"ERROR:  (dis) dis_set_nominal\n");
    FPRINTF(stderr,"        No 'nominal' field in variable.\n");
    WriteInstance(stderr,IPTR(dis->datom));
    return;
  }
  SetBooleanAtomValue(c,nominal,(unsigned)0);
}


uint32 dis_flagsF(const struct dis_discrete *dis)
{
  return dis->flags;
}


void dis_set_flagsF(struct dis_discrete *dis, uint32 flags)
{
  dis->flags = flags;
}

uint32 dis_fixed(struct dis_discrete *dis)
{
  struct Instance *c;
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_fixed called on bad dis\n");
    return FALSE;
  }
  if (!boolean_var(dis->datom)) {
    if (dis_const(dis)) {
      FPRINTF(stderr,"dis_fixed called on a dis constant\n");
      return TRUE;
    }
    else {
      FPRINTF(stderr,"dis_fixed called on a bad dis var\n");
      return FALSE;
    }
  }
  c = ChildByChar(IPTR(dis->datom),FIXED_V);
  if( c == NULL ) {
    FPRINTF(stderr,"ERROR:  (dis) dis_fixed\n");
    FPRINTF(stderr,"        No 'fixed' field in variable.\n");
    WriteInstance(stderr,IPTR(dis->datom));
    return FALSE;
  }
  dis_set_flagbit(dis,DIS_FIXED,GetBooleanAtomValue(c));
  return( GetBooleanAtomValue(c) );
}


void dis_set_fixed(struct dis_discrete *dis, uint32 fixed)
{
  struct Instance *c;
  if (dis==NULL || dis->datom==NULL) {
    FPRINTF(stderr,"dis_set_fixed called on bad dis var\n");
    return;
  }
  if (!boolean_var(dis->datom)) {
    if (dis_const(dis)) {
      FPRINTF(stderr,"dis_set_fixed called on a dis constant\n");
      return;
    }
    else {
      FPRINTF(stderr,"dis_set_fixed called on a bad dvar\n");
      return;
    }
  }
  c = ChildByChar(IPTR(dis->datom),FIXED_V);
  if( c == NULL ) {
    FPRINTF(stderr,"ERROR:  (dis) dis_set_fixed\n");
    FPRINTF(stderr,"        No 'fixed' field in variable.\n");
    WriteInstance(stderr,IPTR(dis->datom));
    return;
  }
  SetBooleanAtomValue(c,fixed,(unsigned)0);
  dis_set_flagbit(dis,DIS_FIXED,fixed);
}


extern uint32 dis_flagbit(const struct dis_discrete *dis,const uint32 one)
{
  if (dis==NULL || dis->datom == NULL) {
    FPRINTF(stderr,"ERROR: dis_flagbit called with bad dis.\n");
    return 0;
  }
  return (dis->flags & one);
}

void dis_set_flagbit(struct dis_discrete *dis, uint32 field,uint32 one)
{
  if (dis==NULL || dis->datom == NULL) {
    FPRINTF(stderr,"ERROR: dis_set_flagbit called with bad dvar.\n");
    return;
  }
  if (one) {
    dis->flags |= field;
  } else {
    dis->flags &= ~field;
  }
}

int32 dis_apply_filter(const struct dis_discrete *dis,
                       const dis_filter_t *filter)
{
  if (dis==NULL || filter==NULL || dis->datom == NULL) {
    FPRINTF(stderr,"dis_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & dis->flags) ==
           (filter->matchbits & filter->matchvalue) );
}

/* global for use with the push function. Sets the ip to the
 * value in g_dis_tag;
 */
static void *g_dis_tag = NULL;

/*
 * should be using vp instead of a global counter.
 */
static 
void *SetDisTags(struct Instance *i,VOIDPTR vp)
{
  (void)vp;
  if (i!=NULL && InstanceKind(i)==BOOLEAN_ATOM_INST)  {
    return g_dis_tag;
  } else {
    return NULL;
  }
}

struct dis_discrete **dis_BackendTokens_to_dis(slv_system_t sys,
                                               SlvBackendToken *atoms,
                                               int32 len)
{
  int32 i,distot,bvlen,count=0;
  uint32 apos,ulen;
  struct dis_discrete **result;
  struct dis_discrete **dislist;
  struct gl_list_t *oldips;
  ulen = (uint32)len;

  if (sys==NULL || atoms == NULL || len < 1) return NULL;
  result = (struct dis_discrete **)malloc(len*sizeof(struct dis_discrete *));
  if (result == NULL) return result;
  /* init results to null */
  for (i=0; i<len; i++) result[i] = NULL;
  /* fill ips w/len in all the vars in tree. */
  g_dis_tag = (void *)len;
  distot = slv_get_num_master_dvars(sys) +
           slv_get_num_master_disunatt(sys);
  oldips = PushInterfacePtrs(slv_instance(sys),SetDisTags,distot,0,NULL);
  /* fill ips of wanted atoms with i */
  for (i=0; i<len; i++) {
    if (GetInterfacePtr(atoms[i])==g_dis_tag &&
	InstanceKind(atoms[i]) == BOOLEAN_ATOM_INST) {
	/* guard a little */
      SetInterfacePtr((struct Instance *)atoms[i],(void *)i);
    } else {
      /* 
       * the odds of g_dis_tag being a legal pointer are vanishingly
       * small, so if we find an ATOM without g_dis_tag we assume it
       * is outside the tree and shouldn't have been in the list. 
       */
      FPRINTF(stderr,"dis_BackendTokens_to_dis called with bad token.\n");
    }
  }
  /* run through the master lists and put the dis vars with their atoms */
  dislist = slv_get_master_dvar_list(sys);
  bvlen = slv_get_num_master_dvars(sys);
  for (i = 0; i <bvlen; i++) {
    apos = (uint32)GetInterfacePtr(dis_instance(dislist[i]));
    if ( apos < ulen ) {
      result[apos] = dislist[i];
      count++;
    }
  }
  dislist = slv_get_master_disunatt_list(sys);
  bvlen = slv_get_num_master_disunatt(sys);
  for (i = 0; i <bvlen; i++) {
    apos = (uint32)GetInterfacePtr(dis_instance(dislist[i]));
    if ( apos < ulen ) {
      result[apos] = dislist[i];
      count++;
    }
  }
  if (count < len) {
    FPRINTF(stderr,
            "dis_BackendTokens_to_dis found less than expected dis vars\n");
    FPRINTF(stderr,"len = %d, diss found = %d\n",len,count);
  } else {
    FPRINTF(stderr,
           "dis_BackendTokens_to_dis found more than expected dis vars\n");
    FPRINTF(stderr,"len = %d, diss found = %d\n",len,count);
  }
  PopInterfacePtrs(oldips,NULL,NULL);
  return result;
}

