/*
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: cmpfunc.c,v $
 *  Date last modified: $Date: 1998/02/05 16:35:43 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 */

#include <utilities/ascConfig.h>
#include "compiler.h"
#include "symtab.h"
#include "instance_enum.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "instance_types.h"
#include "cmpfunc.h"

/* it would be really nice if we implemented this some other way
 * on a proper symbol table structure so we need only compare ints
 */
int CmpSymchar(symchar *s1, symchar *s2)
{
  assert(s1!=NULL);
  assert(s2!=NULL);
  assert(AscFindSymbol(s1)!=NULL);
  assert(AscFindSymbol(s2)!=NULL);
  if (s1==s2) return 0;
  return strcmp(SCP(s1),SCP(s2));
}

int CmpPtrs(void *p1, void *p2) {
  if (p1<p2) return -1;
  else return (p1 > p2);
}

int CmpRealPtrs(void *p1, void *p2) {
  assert(p1!=NULL);
  assert(p2!=NULL);
  if (p1<p2) return -1;
  else return (p1 > p2);
}


int CmpParentsF(CONST struct Instance *p1, CONST struct Instance *p2)
{
  assert((p1 != NULL)&&(p2 != NULL));
  if (p1<p2) return -1;
  else return (p1 > p2);
}

int CmpRelationsF(CONST struct Instance *p1, CONST struct Instance *p2)
{
  assert(p1!=NULL);
  assert(p2!=NULL);
  if (p1<p2) return -1;
  else return (p1 > p2);
}

int CmpLogRelationsF(CONST struct Instance *p1, CONST struct Instance *p2)
{
  assert(p1!=NULL);
  assert(p2!=NULL);
  if (p1<p2) return -1;
  else return (p1 > p2);
}

int CmpWhensF(CONST struct Instance *p1, CONST struct Instance *p2)
{
  assert(p1!=NULL);
  assert(p2!=NULL);
  if (p1<p2) return -1;
  else return (p1 > p2);
}

int CmpIntIndex(CONST struct ArrayChild *p1, CONST struct ArrayChild *p2)
{
  assert((p1 != NULL) && (p2 != NULL));
  return (p1->name.index == p2->name.index) ? 0 :
    ((p1->name.index > p2->name.index) ? 1 : -1);
}

int CmpStrIndex(CONST struct ArrayChild *p1, CONST struct ArrayChild *p2)
{
  assert((p1 != NULL) && (p2 != NULL));
  return CmpSymchar(p1->name.str,p2->name.str);
}


/*******************************************************************\
The functions above give a sorted list sorted in increasing address
order. Those below, a DECREASING address order.
\*******************************************************************/

int CmpPtrsReverse(void *p1, void *p2) {
  if (p1>p2) return -1;
  else return (p1 < p2);
}
