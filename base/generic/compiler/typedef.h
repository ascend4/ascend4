/*
 *  Ascend Type Definition Module
 *  by Tom Epperly
 *  Created: 1/11/90
 *  Version: $Revision: 1.15 $
 *  Version control file: $RCSfile: typedef.h,v $
 *  Date last modified: $Date: 1998/03/25 06:45:50 $
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
 */

/** @file
 *  Ascend Type Definition Module.
 *  This module provides functions for creating new types.
 *  <pre>
 *  When #including typedef.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler/fractions.h"
 *         #include "compiler/instance_enum.h"
 *         #include "compiler/compiler.h"
 *         #include "compiler/module.h"
 *         #include "compiler/list.h"
 *         #include "compiler/slist.h"
 *         #include "compiler/dimen.h"
 *         #include "compiler/child.h"
 *         #include "compiler/type_desc.h"
 *  </pre>
 */

#ifndef typedef_h_seen__
#define typedef_h_seen__

extern void DestroyTypedefRecycle(void);
/**< 
 *  Clears recycled memory.
 *  To work efficiently the typedef module recycles certain pieces 
 *  of memory internally.  This function may be called any time (from 
 *  outside this file) to clear out this recycled memory.  The only 
 *  time it makes _sense_ to do so is after deleting all types in the
 *  library or when shutting down the system, but it is safe regardless.
 */

extern struct TypeDescription
*CreateModelTypeDef(symchar *name,             /* model name */
                    symchar *refines,          /* refines name*/
                    struct module_t *mod,
                    int univ,                  /* universal ? */
                    struct StatementList *sl,  /* declarative statements*/
                    struct gl_list_t *pl,      /* initialization procs */
                    struct StatementList *psl, /* parameter statements */
                    struct StatementList *rsl, /* parameter reductions */
                    struct StatementList *wsl, /* parameter wheres */
                    unsigned int err);
/**<
 *  Creates a model type definition.
 *
 *  Note sl, psl and rsl have restrictions on the statements allowed:
 *  - sl cannot contain WILL_BEs
 *  - psl cannot contain other than WILL_BEs/IS_As
 *  - rsl cannot contain other than constant assignments
 *  - wsl cannot contain other than WILL_BE_THE_SAMEs
 *
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are:
 *  sl, pl, psl, rsl, wsl.
 *
 *  @param name     Name of the type to define.
 *  @param refines  Name of the type it refines or NULL.
 *  @param mod      Module where the type is defined.
 *  @param univ     FALSE non-universal, TRUE universal type.
 *  @param sl       Declarative statements.
 *  @param pl       List of procedures or NULL.
 *  @param psl      Declarative parameter statements.
 *  @param rsl      Parameter reductions.
 *  @param wsl      Conditions on WILL_BE parameter structure.
 *  @param err      If !=0, abandon input.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateConstantTypeDef(symchar *name,        /* constant name */
                       symchar *refines,     /* refines name*/
                       struct module_t *mod, /* module */
                       int univ,             /* universal ? */
                       int defaulted,        /* defaulted ? */
                       double rval,          /* default real value */
                       long ival,            /* default integer/boolean value */
                       symchar *sval,        /* default symbol value */
                       CONST dim_type *dim,  /* default dimensions */
                       unsigned int err);
/**<
 *  Creates a refinement of refines. refines cannot be NULL.
 *  If refines is already defaulted, defaulted must be FALSE.
 *  Only the value (among rval,ival,sval) that matches refines is used.
 *  An appropriate dim should be supplied for all types.
 *
 *  @param name       Name of the type.
 *  @param refines    Name of the type it refines.
 *  @param mod        Module where the type is defined.
 *  @param univ       FALSE non-universal, TRUE universal type.
 *  @param defaulted  FALSE no default assigned, TRUE default.
 *  @param rval       Default value for reals only.
 *  @param ival       Default value for integers/booleans.
 *  @param sval       Default for symbols.
 *  @param dim        Dimensions of default real value.
 *  @param err        If !=0, abandon input.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateAtomTypeDef(symchar *name,            /* atom name */
                   symchar *refines,         /* refines name*/
                   enum type_kind t,
                   struct module_t *mod,     /* module */
                   int univ,                 /* universal ? */
                   struct StatementList *sl, /* declarative statements */
                   struct gl_list_t *pl,     /* initialization procedures*/
                   int defaulted,
                   double val,               /* default value */
                   CONST dim_type *dim,      /* default dimensions */
                   long ival,                /* default int/bool */
                   symchar *sval,            /* default sym*/
                   unsigned int err);
/**<
 *  Creates an atom type definition.
 *  Note: val,ival,sval are digested according to the type_kind.
 *  Those which are irrelevant are ignored.<br><br>
 *
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are:
 *  sl, pl.<br><br>
 *
 *  We should very much like to require that pl be NULL.
 *
 *  @param name       Name of the type.
 *  @param refines    Name of the type it refines OR NULL.
 *  @param t          Ignored when refines!=NULL.
 *  @param mod        Module where the type is defined.
 *  @param univ;      FALSE non-universal, TRUE universal type.
 *  @param sl         List of declarative statements.
 *  @param pl         List of initialization procedures OR NULL.
 *  @param defaulted  FALSE no default assigned, TRUE default.
 *  @param val        Default value for reals only.
 *  @param dim        Dimensions of default value.
 *  @param ival       Default integer or boolean.
 *  @param sval       Default symbol.
 *  @param err        If !=0 --> abandon input.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateRelationTypeDef(struct module_t *mod,
                       symchar *name,
                       struct StatementList *sl,
                       struct gl_list_t *pl);
/**<
 *  Creates a relation type definition.
 *
 *  @param mod    The module the type is defined in.
 *  @param name   The name to assign to the relation type.
 *  @param sl     The list of declarative statements.
 *  @param pl     The list of initialization procedures OR NULL.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateLogRelTypeDef(struct module_t *mod,
                     symchar *name,
                     struct StatementList *sl,
                     struct gl_list_t *pl);
/**<
 *  Creates a logical relation type definition.
 *
 *  @param mod    The module the type is defined in.
 *  @param name   The name to assign to the logical relation type.
 *  @param sl     The list of declarative statements.
 *  @param pl     The list of initialization procedures OR NULL.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreatePatchTypeDef(symchar *patch,
                    symchar *original,
                    symchar *orig_mod,
                    struct module_t *mod,
                    struct StatementList *sl,
                    struct gl_list_t *pl,
                    unsigned int err);
/**<
 *  Creates a patch type definition.
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are: sl, pl.
 *
 *  @param patch      Name of the patch.
 *  @param original   Name of the original type being patched.
 *  @param orig_mod   Name of module for the orig type or NULL.
 *  @param mod        The module it is defined in.
 *  @param sl         The list of declarative statements OR NULL.
 *  @param pl         The list of initialization procedures OR NULL.
 *  @param err        If !=0 abandon input.
 *  @return A pointer to the new TypeDescription structure.
 */

extern void DefineFundamentalTypes(void);
/**< 
 *  Define the fundamental and constant basetypes used in ascend.
 *  They will be named following the defines in type_desc.h.
 *
 *  @bug compiler/typedef:DefineFundamentalTypes() doesn't specify the 
 *       name "relation". Doing so is problematic wrt instantiate.c.
 */

#endif /* typedef_h_seen__ */

