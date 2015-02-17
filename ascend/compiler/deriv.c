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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Routines for managing pointers between derivative, state and independent
	variable instances.
*//*
	by Ksenija Bestuzheva (GSOC 2012)
	Created: 27/06/2012
*/

#include "deriv.h"
#include "find.h"
#include "instance_enum.h"
#include "instance_io.h"
#include "instance_types.h"
#include "instmacro.h"
#include "instquery.h"
#include "mathinst.h"
#include "name.h"
#include "parentchild.h"
#include "vlist.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

void SetDerInfo(struct Instance *deriv,
                struct Instance *state,
                struct Instance *indep)
{
  if (InstanceKind(deriv)!=REAL_ATOM_INST || InstanceKind(state)!=REAL_ATOM_INST || InstanceKind(indep)!=REAL_ATOM_INST) ASC_PANIC("Wrong instance kinds in SetDerInfo");
  struct RealAtomInstance *rinst;
  /* Set DerInfo for the derivative */
  rinst = RA_INST(deriv);
  rinst->derinf = ASC_NEW(struct DerInfo);
  rinst->derinf->state = gl_create(7L);
  gl_append_ptr(rinst->derinf->state,(VOIDPTR)state);
  rinst->derinf->indep = gl_create(7L);
  gl_append_ptr(rinst->derinf->indep,(VOIDPTR)indep);
  rinst->derinf->sderiv = NULL;
  rinst->derinf->ideriv = NULL;
  /* Set DerInfo for the state variable */
  rinst = RA_INST(state);
  if (!rinst->derinf) {
    rinst->derinf = ASC_NEW(struct DerInfo);
    rinst->derinf->sderiv = gl_create(7L); 
    gl_append_ptr(rinst->derinf->sderiv,(VOIDPTR)deriv);
    rinst->derinf->ideriv = NULL;
    rinst->derinf->state = NULL;
    rinst->derinf->indep = NULL;
  }else{
    if (!rinst->derinf->sderiv) rinst->derinf->sderiv = gl_create(7L);
    gl_append_ptr(rinst->derinf->sderiv,(VOIDPTR)deriv);
  }
  /* Set DerInfo for the independent variable */
  rinst = RA_INST(indep);
  if (!rinst->derinf) {
    rinst->derinf = ASC_NEW(struct DerInfo);
    rinst->derinf->ideriv = gl_create(7L); 
    gl_append_ptr(rinst->derinf->ideriv,(VOIDPTR)deriv);
    rinst->derinf->sderiv = NULL;
    rinst->derinf->state = NULL;
    rinst->derinf->indep = NULL;
  }else{
    if (!rinst->derinf->ideriv) rinst->derinf->ideriv = gl_create(7L);
    gl_append_ptr(rinst->derinf->ideriv,(VOIDPTR)deriv);
  }
}

struct Instance *FindDerByArgs(struct Instance *state, struct Instance *indep)
{
  struct RealAtomInstance *rinst;
  struct Instance *inst; 
  unsigned long len,c,len1,c1;
  if (!state || !indep) return NULL;
  if (InstanceKind(state)!=REAL_ATOM_INST || InstanceKind(indep)!=REAL_ATOM_INST) ASC_PANIC("Wrong instance kinds in FindDerByArgs.");
  rinst = RA_INST(state);
  if (rinst->derinf == NULL || rinst->derinf->sderiv == NULL) return NULL;
  len = gl_length(rinst->derinf->sderiv);
  for(c=1;c<=len;c++) {
    inst = (struct Instance*)gl_fetch(rinst->derinf->sderiv,c);
    len1 = StatesCount(inst);
    for (c1=1;c1<=len1;c1++) {
      if (StatesForAtom(inst,c1) == state && IndepsForAtom(inst,c1) == indep) return inst;
    }
  }
  return NULL;
}

void ModifyIderivPointers(struct Instance *deriv,
                          struct gl_list_t *indlist,
                          CONST struct Instance *old,
                          CONST struct Instance *new)
{
  assert(indlist!=NULL);
  unsigned long len,c;
  len = gl_length(indlist);
  for (c=1;c<=len;c++) {
    if ((struct Instance*)gl_fetch(indlist,c)==old)
      gl_store(indlist,c,(VOIDPTR)new);
  }
}

void ModifyStatePointers(struct Instance *state,
                         struct gl_list_t *derlist,
                         CONST struct Instance *old,
                         CONST struct Instance *new)
{
  assert(derlist!=NULL);
  unsigned long len,c;
  len = gl_length(derlist);
  for (c=1;c<=len;c++) {
    if ((struct Instance*)gl_fetch(derlist,c)==old)
      gl_store(derlist,c,(VOIDPTR)new);
  }
}

void ModifyIndepPointers(struct Instance *indep,
                         struct gl_list_t *derlist,
                         CONST struct Instance *old,
                         CONST struct Instance *new)
{
  assert(derlist!=NULL);
  unsigned long len,c;
  len = gl_length(derlist);
  for (c=1;c<=len;c++) {
    if ((struct Instance*)gl_fetch(derlist,c)==old)
      gl_store(derlist,c,(VOIDPTR)new);
  }
}

void ModifySderivPointers(struct Instance *deriv,
                          struct gl_list_t *stlist,
                          CONST struct Instance *old,
                          CONST struct Instance *new)
{
  assert(stlist!=NULL);
  unsigned long len,c;
  len = gl_length(stlist);
  for (c=1;c<=len;c++) {
    if ((struct Instance*)gl_fetch(stlist,c)==old)
      gl_store(stlist,c,(VOIDPTR)new);
  }
}

void WriteDerInfo(FILE *f, struct Instance *inst)
{
  unsigned long c,len;
  if (inst == NULL) return;
  FPRINTF(f,"Instance ");
  WriteInstanceName(f,inst,NULL);
  if (RA_INST(inst)->derinf == NULL) {
    FPRINTF(f," has no DerInfo.\n");
    return;
  }
  FPRINTF(f," has the following DerInfo:\n");
  if (RA_INST(inst)->derinf->sderiv != NULL) {
    FPRINTF(f,"Its derivatives are:\n");
    len = gl_length(RA_INST(inst)->derinf->sderiv);
    for(c=1;c<=len;c++) {
      WriteInstanceName(f,(struct Instance*)gl_fetch(RA_INST(inst)->derinf->sderiv,c),NULL);
      FPRINTF(f,"\n");
    }
  }
  if (RA_INST(inst)->derinf->ideriv != NULL) {
    FPRINTF(f,"There are derivatives with respect to this instance:\n");
    len = gl_length(RA_INST(inst)->derinf->ideriv);
    for(c=1;c<=len;c++) {
      WriteInstanceName(f,(struct Instance*)gl_fetch(RA_INST(inst)->derinf->ideriv,c),NULL);
      FPRINTF(f,"\n");
    }
  }
  if (RA_INST(inst)->derinf->state != NULL) {
    FPRINTF(f,"It is the derivative of:\n");
    len = gl_length(RA_INST(inst)->derinf->state);
    for(c=1;c<=len;c++) {
      WriteInstanceName(f,(struct Instance*)gl_fetch(RA_INST(inst)->derinf->state,c),NULL);
      FPRINTF(f,"\n");
    }
  }
  if (RA_INST(inst)->derinf->indep != NULL) {
    FPRINTF(f,"With respect to:\n");
    len = gl_length(RA_INST(inst)->derinf->indep);
    for(c=1;c<=len;c++) {
      WriteInstanceName(f,(struct Instance*)gl_fetch(RA_INST(inst)->derinf->indep,c),NULL);
      FPRINTF(f,"\n");
    }
  }
}

int IsDeriv(struct Instance *inst)
{
  assert(InstanceKind(inst)==REAL_ATOM_INST);
  if ((RA_INST(inst)->derinf!=NULL) && (RA_INST(inst)->derinf->state!=NULL) && (RA_INST(inst)->derinf->indep!=NULL)) return 1;
  return 0;
}

int IsState(struct Instance *inst)
{
  assert(InstanceKind(inst)==REAL_ATOM_INST);
  if ((RA_INST(inst)->derinf!=NULL) && (RA_INST(inst)->derinf->sderiv!=NULL)) return 1;
  return 0;
}

int IsIndep(struct Instance *inst)
{
  assert(InstanceKind(inst)==REAL_ATOM_INST);
  if ((RA_INST(inst)->derinf!=NULL) && (RA_INST(inst)->derinf->ideriv!=NULL)) return 1;
  return 0;
}

struct gl_list_t *StateVars(struct Instance *inst)
{
  if (InstanceKind(inst)!=REAL_ATOM_INST || RA_INST(inst)->derinf==NULL || RA_INST(inst)->derinf->state==NULL) return NULL;
  return gl_copy(RA_INST(inst)->derinf->state);
}

struct gl_list_t *Sderivs(struct Instance *inst)
{
  if (InstanceKind(inst)!=REAL_ATOM_INST || RA_INST(inst)->derinf==NULL || RA_INST(inst)->derinf->sderiv==NULL) return NULL;
  return gl_copy(RA_INST(inst)->derinf->sderiv);
}

struct gl_list_t *Iderivs(struct Instance *inst)
{
  if (InstanceKind(inst)!=REAL_ATOM_INST || RA_INST(inst)->derinf==NULL || RA_INST(inst)->derinf->ideriv==NULL) return NULL;
  return gl_copy(RA_INST(inst)->derinf->ideriv);
}

struct gl_list_t *IndepVars(struct Instance *inst)
{
  if (InstanceKind(inst)!=REAL_ATOM_INST || RA_INST(inst)->derinf==NULL || RA_INST(inst)->derinf->indep==NULL) return NULL;
  return gl_copy(RA_INST(inst)->derinf->indep);
}

