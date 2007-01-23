/*
 *  Ascend Instance Tree Type Implementation
 *  by Tom Epperly
 *  9/3/89
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: tmpnum.c,v $
 *  Date last modified: $Date: 1998/02/20 02:10:34 $
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
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>

#include "bit.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "instance_enum.h"
#include "instance_types.h"
#include "instmacro.h"
#include "visitinst.h"
#include "instquery.h"
#include "find.h"
#include "tmpnum.h"

#ifndef lint
static CONST char TmpNumModuleID[] = "$Id: tmpnum.c,v 1.11 1998/02/20 02:10:34 ballan Exp $";
#endif

/*
 * Modified by kaa 12 Sep 1995, to be more useful friendly
 * and more efficient. Does not choke on fundamentals.
 * It simply returns an invalid result; 0 is almost never
 * a valid result in the compiler.
 */
unsigned long GetTmpNum(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i == NULL) {
    return LONG_MAX; /* baa */
  }
  if (IsAtomicInstance(i)) {
    return CA_INST(i)->tmp_num;
  }
  switch (i->t) {
  case MODEL_INST:
    return MOD_INST(i)->tmp_num;
  case REL_INST:
    return RELN_INST(i)->tmp_num;
  case LREL_INST:
    return LRELN_INST(i)->tmp_num;
  case WHEN_INST:
    return W_INST(i)->tmp_num;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return ARY_INST(i)->tmp_num;
  case REAL_CONSTANT_INST:
    return RC_INST(i)->tmp_num;
  case BOOLEAN_CONSTANT_INST:
    return BC_INST(i)->tmp_num;
  case INTEGER_CONSTANT_INST:
    return IC_INST(i)->tmp_num;
  case SYMBOL_CONSTANT_INST:
    return SYMC_INST(i)->tmp_num;
  case SIM_INST:
    return SIM_INST(i)->tmp_num;
  case DUMMY_INST:
    return D_INST(i)->tmp_num;
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:
    return 0;		/* this is the change was exit(2).kaa */
  default:
    ASC_PANIC("Incorrect type in GetTmpNum.\n");
    return LONG_MAX; /* but not really. shuts up gcc, though. */
  }
}

void SetTmpNum(struct Instance *i, unsigned long int cn)
{
  AssertMemory(i);
  if (IsAtomicInstance(i)) {
    CA_INST(i)->tmp_num = cn;
    return;
  }
  if (IsConstantInstance(i)) {
    CI_INST(i)->tmp_num = cn;
    return;
  }
  switch(i->t) {
  case SIM_INST:
    SIM_INST(i)->tmp_num = cn;
    break;
  case DUMMY_INST:
    D_INST(i)->tmp_num = cn;
    break;
  case MODEL_INST:
    MOD_INST(i)->tmp_num = cn;
    break;
  case REL_INST:
    RELN_INST(i)->tmp_num = cn;
    break;
  case LREL_INST:
    LRELN_INST(i)->tmp_num = cn;
    break;
  case WHEN_INST:
    W_INST(i)->tmp_num = cn;
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    ARY_INST(i)->tmp_num = cn;
    break;
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:
    break;		/* another change -- dont choke just return */
  default:
    ASC_PANIC("Incorrect type in SetTmpNum.\n");
  }
}

unsigned long IncrementTmpNum(struct Instance *i)
{
  if (i == NULL) {
    return LONG_MAX; /* baa */
  }
  if (IsAtomicInstance(i)) {
    CA_INST(i)->tmp_num += 1;
    return CA_INST(i)->tmp_num;
  }
  if (IsConstantInstance(i)) {
    CI_INST(i)->tmp_num += 1;
    return CI_INST(i)->tmp_num;
  }
  switch(i->t) {
  case SIM_INST:
    SIM_INST(i)->tmp_num += 1;
    return SIM_INST(i)->tmp_num;
  case DUMMY_INST:
    D_INST(i)->tmp_num += 1;
    return D_INST(i)->tmp_num;
  case MODEL_INST:
    MOD_INST(i)->tmp_num += 1;
    return MOD_INST(i)->tmp_num;
  case REL_INST:
    RELN_INST(i)->tmp_num += 1;
    return RELN_INST(i)->tmp_num;
  case LREL_INST:
    LRELN_INST(i)->tmp_num += 1;
    return LRELN_INST(i)->tmp_num;
  case WHEN_INST:
    W_INST(i)->tmp_num += 1;
    return W_INST(i)->tmp_num;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    ARY_INST(i)->tmp_num += 1;
    return ARY_INST(i)->tmp_num;
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:
    break;		/* another change -- dont choke just return */
  default:
    ASC_PANIC("Incorrect type in IncrementTmpNum.\n");
  }
  return 0;
}

unsigned long DecrementTmpNum(struct Instance *i)
{
  if (i == NULL) {
    return LONG_MAX; /* baa */
  }
  if (IsAtomicInstance(i)) {
    if (CA_INST(i)->tmp_num > 0) {
      CA_INST(i)->tmp_num -= 1;
    }
    return CA_INST(i)->tmp_num;
  }
  if (IsConstantInstance(i)) {
    if (CI_INST(i)->tmp_num > 0) {
      CI_INST(i)->tmp_num -= 1;
    }
    return CI_INST(i)->tmp_num;
  }
  switch(i->t) {
  case SIM_INST:
    if (SIM_INST(i)->tmp_num > 0) { 
      SIM_INST(i)->tmp_num -= 1;
    }
    return SIM_INST(i)->tmp_num;
  case DUMMY_INST:
    if (D_INST(i)->tmp_num > 0) {
      D_INST(i)->tmp_num -= 1;
    }
    return D_INST(i)->tmp_num;
  case MODEL_INST:
    if (MOD_INST(i)->tmp_num > 0) {
      MOD_INST(i)->tmp_num -= 1;
    }
    return MOD_INST(i)->tmp_num;
  case REL_INST:
    if (RELN_INST(i)->tmp_num > 0) {
      RELN_INST(i)->tmp_num -= 1;
    }
    return RELN_INST(i)->tmp_num;
  case LREL_INST:
    if (LRELN_INST(i)->tmp_num >0) {
      LRELN_INST(i)->tmp_num -= 1;
    }
    return LRELN_INST(i)->tmp_num;
  case WHEN_INST:
    if (W_INST(i)->tmp_num > 0) {
      W_INST(i)->tmp_num -= 1;
    }
    return W_INST(i)->tmp_num;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->tmp_num >0) {
      ARY_INST(i)->tmp_num -= 1;
    }
    return ARY_INST(i)->tmp_num;
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:
    break;		/* another change -- dont choke just return */
  default:
    ASC_PANIC("Incorrect type in DecrementTmpNum.\n");
  }
  return 0;
}

static void ResetNodes(struct Instance *i)
{
  SetTmpNum(i,0);
}

void ZeroTmpNums(struct Instance *i,int order)
{
    SilentVisitInstanceTree(i,ResetNodes,order,0);
}

