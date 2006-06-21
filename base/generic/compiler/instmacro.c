/*
 *  Ascend Instance Tree Type Implementation Macros
 *  by Ben Allan
 *  9/16/96
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: instmacro.c,v $
 *  Date last modified: $Date: 1997/07/18 12:30:43 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright 1996 Bejamin Allan
 *  based on instance.c
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
#include <utilities/ascPanic.h>
#include "instance_enum.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "instance_types.h"
#include "instmacro.h"

#ifndef lint
static CONST char InstanceMacroModuleID[] = "$Id: instmacro.c,v 1.6 1997/07/18 12:30:43 mthomas Exp $";
#endif

/* temporary global variables used for debugging only */
#ifndef NDEBUG
/* fundies */
struct RealInstance *g_r_inst;
struct IntegerInstance *g_i_inst;
struct BooleanInstance *g_b_inst;
struct SetInstance *g_s_inst;
struct SymbolInstance *g_sym_inst;
/* constants */
struct RealConstantInstance *g_rc_inst;
struct IntegerConstantInstance *g_ic_inst;
struct BooleanConstantInstance *g_bc_inst;
struct SetConstantInstance *g_sc_inst;
struct SymbolConstantInstance *g_symc_inst;
/* atoms */
struct RealAtomInstance *g_ra_inst;
struct IntegerAtomInstance *g_ia_inst;
struct BooleanAtomInstance *g_ba_inst;
struct SetAtomInstance *g_sa_inst;
struct SymbolAtomInstance *g_syma_inst;
/* other */
struct ModelInstance *g_mod_inst;
struct RelationInstance *g_rel_inst;
struct LogRelInstance *g_lrel_inst;
struct WhenInstance *g_when_inst;
#endif

int NotAtomF(struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return 1;
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST: /* well, it's fair to say the dummy is not anything */
    return 0;
  default:
    Asc_Panic(2, NULL, "VisitTree: Illegal instance type.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}
