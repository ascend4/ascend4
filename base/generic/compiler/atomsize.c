/*
 *  Ascend Instance Atom Size Functions
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: atomsize.c,v $
 *  Date last modified: $Date: 1997/07/18 12:28:01 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Ben Allan
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
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "childinfo.h"
#include "instance_name.h"
#include "instance_io.h"
#include "instance_types.h"
#include "instmacro.h"
#include "atomsize.h"

#ifndef lint
static CONST char AtomSizeModuleID[] = "$Id: atomsize.c,v 1.6 1997/07/18 12:28:01 mthomas Exp $";
#endif

static unsigned long ChildMemory(unsigned long int count,
                                 CONST struct ChildDesc *cdesc)
{
  register unsigned long result;
  result = NextHighestEven(count)*sizeof(struct Instance *);
  while(count) {
    switch(ChildDescType(GetChildArrayElement(cdesc,count))) {
    case real_child:
      result += sizeof(struct RealInstance);
      break;
    case integer_child:
      result += sizeof(struct IntegerInstance);
      break;
    case boolean_child:
      result += sizeof(struct BooleanInstance);
      break;
    case set_child:
      result += sizeof(struct SetInstance);
      break;
    case symbol_child:
      result += sizeof(struct SymbolInstance);
      break;
    default:
      /* the bad_child type should never! have count > 0 */
      Asc_Panic(2, "ChildMemory",
                "Bad ChildDesc struct passed to ChildMemory.\n");
      /*NOTREACHED*/
    }
    count--;
  }
  return result;
}

unsigned long ConstantByteLength(enum inst_t it)
{
  switch (it) {
  case REAL_CONSTANT_INST:
    return sizeof(struct RealConstantInstance);
  case BOOLEAN_CONSTANT_INST:
    return sizeof(struct BooleanConstantInstance);
  case INTEGER_CONSTANT_INST:
    return sizeof(struct IntegerConstantInstance);
  case SYMBOL_CONSTANT_INST:
    return sizeof(struct SymbolConstantInstance);
  default:
    ASC_PANIC("Bytelength of unknown constant type requested.");
    
    break;
  }
  /* not reached */
}

unsigned long RealAtomByteLength(unsigned long int num_children,
				 CONST struct ChildDesc *c)
{
  return sizeof(struct RealAtomInstance)+ChildMemory(num_children,c);
}

unsigned long IntegerAtomByteLength(unsigned long int num_children,
				    CONST struct ChildDesc *c)
{
  return sizeof(struct IntegerAtomInstance)+ChildMemory(num_children,c);
}

unsigned long BooleanAtomByteLength(unsigned long int num_children,
				    CONST struct ChildDesc *c)
{
  return sizeof(struct BooleanAtomInstance)+ChildMemory(num_children,c);
}

unsigned long SetAtomByteLength(unsigned long int num_children,
				CONST struct ChildDesc *c)
{
  return sizeof(struct SetAtomInstance)+ChildMemory(num_children,c);
}

unsigned long SymbolAtomByteLength(unsigned long int num_children,
				   CONST struct ChildDesc *c)
{
  return sizeof(struct SymbolAtomInstance)+ChildMemory(num_children,c);
}

unsigned long RelationAtomByteLength(unsigned long int num_children,
				     CONST struct ChildDesc *c)
{
  return sizeof(struct RelationInstance)+ChildMemory(num_children,c);
}


unsigned long LogRelAtomByteLength(unsigned long int num_children,
				     CONST struct ChildDesc *c)
{
  return sizeof(struct LogRelInstance)+ChildMemory(num_children,c);
}

