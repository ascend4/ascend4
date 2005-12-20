/*
 *  Atom Child Description Implementation
 *  by Tom Epperly
 *  part of Ascend
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: childinfo.c,v $
 *  Date last modified: $Date: 1998/02/05 16:35:39 $
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/setinstval.h"
#include "compiler/childinfo.h"
#include "general/mathmacros.h"

#ifndef lint
static CONST char ChildDescID[] = "$Id: childinfo.c,v 1.10 1998/02/05 16:35:39 ballan Exp $";
#endif


void CDescInit(struct ChildDesc *v)
{
  memset((char *)v,0,sizeof(struct ChildDesc));
}
/*
 * ICDESC(stackvar) should be called on locally allocated
 * value_t before any other action using them is taken.
 * When NDEBUG is not defined, it causes the stack memory to be
 * initialized to 0. Normally it is a do nothing macro.
 * Proper initialization helps us separate signal from noise in
 * gdb and purify.
 */                                 

struct ChildDesc *CreateChildDescArray(unsigned long int l)
{
#ifndef NDEBUG
  return
    ((struct ChildDesc *)asccalloc(1,(unsigned)(sizeof(struct ChildDesc)*l)));
#else
  return
    ((struct ChildDesc *)ascmalloc((unsigned)(sizeof(struct ChildDesc)*l)));
#endif
}

struct ChildDesc *CreateEmptyChildDescArray(void)
{
  register struct ChildDesc *cda = NULL;
  cda = (struct ChildDesc *)asccalloc(1,sizeof(struct ChildDesc));
  assert(cda != NULL);
  cda->t = bad_child;
  return cda;
}

void DestroyChildDescArray(struct ChildDesc *c, unsigned long int l)
{
  struct ChildDesc *ptr = c;
  AssertAllocatedMemory(c,sizeof(struct ChildDesc)*MAX(l,1));
  while(l--){
    if ((ptr->t == set_child) && (ptr->u.s.slist))
      DestroySet(ptr->u.s.slist);
    ptr++;
  }
  ascfree(c);
}

struct ChildDesc MakeRealDesc(int assigned, double v, CONST dim_type *dims)
{
  struct ChildDesc result;
  ICDESC(result);
  result.t = real_child;
  result.assigned = (assigned)?1:0;
  result.u.rvalue.value = v;
  result.u.rvalue.dims = dims;
  return result;
}

struct ChildDesc MakeIntegerDesc(int assigned, long int i)
{
  struct ChildDesc result;
  ICDESC(result);
  result.t = integer_child;
  result.assigned = (assigned)?1:0;
  result.u.ivalue.value = i;
  return result;
}

struct ChildDesc MakeBooleanDesc(int assigned, int b)
{
  struct ChildDesc result;
  ICDESC(result);
  result.t = boolean_child;
  result.assigned = (assigned)?1:0;
  result.u.bvalue = b;
  return result;
}

struct ChildDesc MakeSetDesc(int assigned, int intset, struct set_t *s)
{
  struct ChildDesc result;
  ICDESC(result);
  result.t = set_child;
  result.assigned = (assigned)?1:0;
  result.u.s.slist = s;
  result.u.s.is_int = intset ? 1 :0;
  return result;
}

struct ChildDesc MakeSymbolDesc(int assigned, symchar *str)
{
  struct ChildDesc result;
  ICDESC(result);
  result.t = symbol_child;
  result.assigned = (assigned)?1:0;
  result.u.svalue = str;
  return result;
}

void AssignChildArrayElementF(struct ChildDesc *a, unsigned long int n,
			      struct ChildDesc e)
{
  AssertMemory(a);
  AssertMemory(&a[n-1]);
  a[n-1] = e;
}

struct ChildDesc GetChildArrayElementF(CONST struct ChildDesc *a,
				       unsigned long int n)
{
  AssertMemory(a);
  AssertMemory(&a[n-1]);
  return a[n-1];
}

enum ChildDescT ChildDescTypeF(struct ChildDesc e)
{
  return e.t;
}

int ValueAssignedF(struct ChildDesc e)
{
  return e.assigned;
}

long IntegerDefaultF(struct ChildDesc e)
{
  return e.u.ivalue.value;
}

symchar *SymbolDefaultF(struct ChildDesc e)
{
  return e.u.svalue;
}

int BooleanDefaultF(struct ChildDesc e)
{
  return e.u.bvalue;
}

CONST struct set_t *SetDefaultF(struct ChildDesc e)
{
  return e.u.s.slist;
}

int SetIsIntegerSetF(struct ChildDesc e)
{
  return e.u.s.is_int;
}

double RealDefaultValueF(struct ChildDesc e)
{
  return e.u.rvalue.value;
}

CONST dim_type *RealDimensionsF(struct ChildDesc e)
{
  return e.u.rvalue.dims;
}
