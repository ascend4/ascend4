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
*//** @file
	Routines for managing pointers between derivative, state and independent
	variable instances.
*//*
	by Ksenija Bestuzheva (GSOC 2013)
	Created: 01/08/2013
*/

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
#include "pre.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

void SetPreInfo(struct Instance *pre,
                struct Instance *prearg)
{
  if (InstanceKind(pre)!=REAL_ATOM_INST || InstanceKind(prearg)!=REAL_ATOM_INST)
    ASC_PANIC("Wrong instance kinds in SetPreInfo");
  struct RealAtomInstance *rinst;
  /* Set PreInfo for the pre() variable */
  rinst = RA_INST(pre);
  rinst->preinf = ASC_NEW(struct PreInfo);
  rinst->preinf->prearg = prearg;
  rinst->preinf->pre = NULL;
  /* Set PreInfo for the pre() argument */
  rinst = RA_INST(prearg);
  if (!rinst->preinf) {
    rinst->preinf = ASC_NEW(struct PreInfo);
    rinst->preinf->pre = pre;
    rinst->preinf->prearg = NULL;
  }else{
    ASC_PANIC("One variable can't have two pres.");
  }
}

struct Instance *FindPreByArg(struct Instance *arg)
{
  struct RealAtomInstance *rinst;
  if (arg == NULL) return NULL;
  if (InstanceKind(arg)!=REAL_ATOM_INST) ASC_PANIC("Wrong instance kind in FindPreByArg.");
  rinst = RA_INST(arg);
  if (rinst->preinf == NULL) return NULL;
  return rinst->preinf->pre;
}

void WritePreInfo(FILE *f, struct Instance *inst)
{
  if (inst == NULL) return;
  FPRINTF(f,"Instance ");
  WriteInstanceName(f,inst,NULL);
  if (InstanceKind(inst)==REAL_ATOM_INST && RA_INST(inst)->preinf != NULL) {
    FPRINTF(f," has the following PreInfo:\n");
    if (RA_INST(inst)->preinf->pre != NULL) {
      FPRINTF(f,"Its pre() is:\n");
      WriteInstanceName(f,RA_INST(inst)->preinf->pre,NULL);
      FPRINTF(f,"\n");
    }
    if (RA_INST(inst)->preinf->prearg != NULL) {
      FPRINTF(f,"It is the pre() variable of:\n");
      WriteInstanceName(f,RA_INST(inst)->preinf->prearg,NULL);
      FPRINTF(f,"\n");
    }
  }else{
    FPRINTF(f," has no PreInfo.\n");
  }
}

int IsPre(struct Instance *inst)
{
  if (InstanceKind(inst)==REAL_ATOM_INST) {
    if (RA_INST(inst)->preinf!=NULL && RA_INST(inst)->preinf->prearg!=NULL)
      return 1;
  }
  return 0;
}

int IsPrearg(struct Instance *inst)
{
  if (InstanceKind(inst)==REAL_ATOM_INST) {
    if (RA_INST(inst)->preinf!=NULL && RA_INST(inst)->preinf->pre!=NULL)
      return 1;
  }
  return 0;
}

struct Instance *PreArg(struct Instance *inst)
{
  if (InstanceKind(inst)==REAL_ATOM_INST && RA_INST(inst)->preinf!=NULL) return RA_INST(inst)->preinf->prearg;
  return NULL;
}

struct Instance *Pre(struct Instance *inst)
{
  if (InstanceKind(inst)==REAL_ATOM_INST && RA_INST(inst)->preinf!=NULL) return RA_INST(inst)->preinf->pre;
  return NULL;
}

