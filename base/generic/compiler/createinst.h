/*
 *  Ascend Instance Allocation Functions
 *  by Tom Epperly & Ben Allan
 *  8/16/89
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: createinst.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:48 $
 *  Last modified by: $Author: ballan $
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
 */

/** @file
 *  Ascend Instance Allocation Functions.
 *  <pre>
 *  When #including createinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef ASC_CREATEINST_H
#define ASC_CREATEINST_H

extern void ZeroNewChildrenEntries(struct Instance **ary, unsigned long int num);
/**<
 *  Sets num entries of ary to NULL.
 */

extern struct Instance *CreateSimulationInstance(struct TypeDescription *type,
                                                 symchar *name);
/**<
 *  Create an instance of the type described by "type".  The memory of "type"
 *  is used by the instance structure, so do not dispose or modify the memory.
 *  This initializes the instance with no parents, no clique membership.
 *  Being a simulation instance, it will have only 1 child and will make
 *  a separate copy of its name. The type should be the type of the root
 *  instance of the simulation. A simulation instance will have no parents.
 *  It will respond cleanly to all general instance query commands.<br><br>
 *
 *  The reference count of type is incremented.
 */

extern struct Instance *CreateModelInstance(struct TypeDescription *type);
/**<
 *  Create an instance of the type described by "type".  The memory of "type"
 *  is used by the instance structure, so do not dispose or modify the memory.
 *  This initializes the instance with no parents, no clique membership,
 *  and no statements executed.<br><br>
 *
 *  The reference count of type is incremented.
 */

extern struct Instance *CreateDummyInstance(struct TypeDescription *type);
/**<
 *  Returns a DummyInstance using the type given, which must be
 *  the dummy_type named BASE_UNSELECTED.
 *  The instance returned is UNIVERSAL.<br><br>
 *
 *  The reference count of type is incremented.
 */

extern struct Instance *CreateRealInstance(struct TypeDescription *type);
/**<
 *  Create a real instance whose type is described by "type".  The instance
 *  struct will use the memory pointed to by type, so don't destroy this
 *  structure.  It is cheaper to copy an instance of a real, than to create
 *  one.
 *  The value of the instance is initialized to def.<br><br>
 *
 *  Type may be either real_type or real_constant_type or refinements thereof.
 *  User is expected to know what to do with the result.
 *  The reference count of type and units are incremented.
 */

extern struct Instance *CreateIntegerInstance(struct TypeDescription *type);
/**<
 *  Create an integer instance whose type is described by "type".  The memory
 *  pointed to by type is used by the instance, so don't destroy it or modify
 *  it.
 *  Type may be either integer_type or integer_constant_type or
 *  refinements thereof.
 *  User is expected to know what to do with the result.<br><br>
 *
 *  It is cheaper to copy an instance of an integer, than to create one.
 *  FALSE:  Integer instances don't have default values.
 */

extern struct Instance *CreateBooleanInstance(struct TypeDescription *type);
/**<
 *  Create a boolean instance whose type is described by "type".  The
 *  instance structure will use the memory pointed to by type, so don't
 *  modify it or free it.
 *  FALSE: Booleans don't yet have defaults.
 *  Type may be either boolean_type or boolean_constant_type or
 *  refinements thereof.
 *  User is expected to know what to do with the result.
 */

extern struct Instance *CreateSetInstance(struct TypeDescription *type, int intset);
/**<
 *  Create a set instance whose type is described by "type".  The instance
 *  will use the memory pointed to by type, so don't modify it or free it.
 *  Set instances don't have defaults.<br><br>
 *
 *  intset determines the "type" of set.  True means it is a set of integers,
 *  FALSE means a set of symbols.
 */

extern struct Instance *CreateSymbolInstance(struct TypeDescription *type);
/**<
 *  Create a symbol instance whose type is described by "type".  The
 *  instance will use the memory pointed by type type, so don't modify
 *  it or free it.
 *  FALSE: Symbol instances don't yet have defaults.
 *  Type may be either symbol_type or symbol_constant_type or
 *  refinements thereof.
 */

extern struct Instance *CreateArrayInstance(struct TypeDescription *type,
                                            unsigned long index);
/**<
 *  Create an array instance whose type is described by "set".  The instance
 *  will use the memory pointed to by type, so don't modify it or free it.<br><br>
 *
 *  index indicates the number in the index list which this array is.
 *  Normally this should be 1.
 */

extern struct Instance *CreateRelationInstance(struct TypeDescription *type,
                                               enum Expr_enum reltype);
/**<
 *  Create a relation instance who type is described by "type".  The instance
 *  will use the memory pointed to by type, so don't modify it or free it.
 *  reltype is one of e_glassbox, e_blackbox, e_opcode, e_tokens and must
 *  be provided.
 */

extern struct Instance *CreateLogRelInstance(struct TypeDescription *type);
/**<
 *  Create a logical relation instance. The instance will use the memory
 *  pointed to by type, so don't modify it or free it.
 *  the type is always e_bol_token.
 */

extern struct Instance *CreateWhenInstance(struct TypeDescription *type);
/**<
 *  Create a when instance. The instance will use the memory
 *  pointed to by type, so don't modify it or free it.
 */

#endif  /* ASC_CREATEINST_H */

