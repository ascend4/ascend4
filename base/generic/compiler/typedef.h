/**< 
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

#ifndef __TYPEDEF_H_SEEN__
#define __TYPEDEF_H_SEEN__


/**< 
 *  When #including typedef.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *         #include "module.h"
 *         #include "list.h"
 *         #include "slist.h"
 *         #include "dimen.h"
 *         #include "child.h"
 *         #include "type_desc.h"
 */

extern void DestroyTypedefRecycle(void);
/**< 
 *  DestroyTypedefRecycle()
 *  To work efficiently the typedef module recycles internally
 *  certain pieces of memory.
 *  This function may be called any time (from outside this file)
 *  to clear out this recycled memory.
 *  The only time it makes _sense_ to do so is after deleting all types
 *  in the library or when shutting down the system, but it is safe
 *  regardless.
 */

extern struct TypeDescription *
CreateModelTypeDef(symchar *,              /**< model name */
                   symchar *,              /*refines name*/
                   struct module_t *,
                   int,                       /**< universal ? */
                   struct StatementList *,    /**< declarative statements*/
                   struct gl_list_t *,        /**< initialization procs */
                   struct StatementList *,    /**< parameter statements */
                   struct StatementList *,    /**< parameter reductions */
                   struct StatementList *,    /**< parameter wheres */
                   unsigned int
                   );
/**< 
 *  struct TypeDescription *CreateModelTypeDef(name,refines,mod,univ,sl,pl,
 *                                             psl,rsl,wsl,err)
 *  const char *name,		name of the type to define
 *             *refines,	name of the type it refines or NULL
 *  struct module_t *mod;	module where the type is defined
 *  int univ;			FALSE non-universal, TRUE universal type
 *  struct StatementList *sl;	declarative statements
 *  struct gl_list_t *pl;	list of procedures or NULL
 *  struct StatementList *psl;	declarative parameter statements
 *  struct StatementList *rsl;	parameter reductions
 *  struct StatementList *wsl;	conditions on WILL_BE parameter structure
 *  unsigned int err;		if !=0, abandon input.
 *  Note sl, psl and rsl have restrictions on the statements allowed:
 *  sl cannot contain WILL_BEs;
 *  psl cannot contain other than WILL_BEs/IS_As;
 *  rsl cannot contain other than constant assignments.
 *  wsl cannot contain other than WILL_BE_THE_SAMEs
 *  This creates a model type definition.
 *
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are:
 *  sl, pl, psl, rsl, wsl.
 */

extern struct TypeDescription *
CreateConstantTypeDef(symchar *,	  /**< constant name */
                      symchar *,	  /**< refines name*/
                      struct module_t *,  /**< module */
                      int,		  /**< universal ? */
                      int,		  /**< defaulted ? */
                      double,		  /**< default real value */
                      long,		  /**< default integer/boolean value */
                      symchar *,	  /**< default symbol value */
                      CONST dim_type *,   /**< default dimensions */
                      unsigned int
                      );
/**< 
 *  struct TypeDescription *CreateConstantTypeDef(
 *  name,refines,mod,univ,defaulted,rval,ival,sval,dim,err)
 *  const char *name;		name of the type
 *             *refines;	name of the type it refines
 *  struct module_t *mod;	module where the type is defined
 *  int univ;			FALSE non-universal, TRUE universal type
 *  int defaulted;		FALSE no default assigned, TRUE default
 *  double rval;		default value for reals only
 *  long ival;			default value for integers/booleans
 *  symchar * sval;		default for symbols
 *  CONST dim_type *dim;	dimensions of default real value
 *  unsigned int err;		if !=0, abandon input.
 *
 *  Creates a refinement of refines. refines cannot be NULL.
 *  If refines is already defaulted, defaulted must be be FALSE.
 *  Only the value (among rval,ival,sval) that matches refines is used.
 *  An appropriate dim should be supplied for all types.
 */

extern struct TypeDescription *
CreateAtomTypeDef(symchar *,            /**< atom name */
                  symchar *,            /**< refines name*/
                  enum type_kind,
                  struct module_t *,       /**< module */
                  int,                     /**< universal ? */
                  struct StatementList *,  /**< declarative statements */
                  struct gl_list_t *,      /**< initialization procedures*/
                  int,
                  double,                  /**< default value */
                  CONST dim_type *,        /**< default dimensions */
                  long,                    /**< default int/bool */
                  symchar *,            /**< default sym*/
                  unsigned int
                  );
/**< 
 *  struct TypeDescription *CreateAtomTypeDef(name,refines,t,mod,univ,sl,pl,
 *          defaulted,val,dim,ival,sval,err)
 *  const char *name,		name of the type
 *             *refines,	name of the type it refines OR NULL
 *  enum type_kind t;		ignored when refines!=NULL
 *  struct module_t *mod;	module where the type is defined
 *  int univ;			FALSE non-universal, TRUE universal type
 *  struct StatementList *sl;	list of declarative statements
 *  struct gl_list_t *pl;	list of initialization procedures OR NULL
 *  int defaulted;		FALSE no default assigned, TRUE default
 *  double val;			default value for reals only
 *  CONST dim_type *dim;	dimensions of default value
 *  ival;			default integer or boolean
 *  symchar *sval;		default symbol
 *  unsigned int err;		!=0 --> abandon input
 *  Note: val,ival,sval are digested according to the type_kind.
 *  Those which are irrelevant are ignored.
 *
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are:
 *  sl, pl.
 *
 *  We should very much like to require that pl be NULL.
 */

extern struct TypeDescription *CreateRelationTypeDef(struct module_t *,
                                                     symchar *,
                                                     struct StatementList *,
                                                     struct gl_list_t *
                                                     );
/**< 
 *  struct TypeDescription *CreateRelationTypeDef(mod,name,sl,pl)
 *  struct module_t *mod;	the module it is defined in
 *  const char *name;           the name to assign to the relation type
 *  struct StatementList *sl;	the list of declarative statements
 *  struct gl_list_t *pl;	the list of initialization procedures OR NULL
 */

extern struct TypeDescription *CreateLogRelTypeDef(struct module_t *,
                                                   symchar *,
                                                   struct StatementList *,
                                                   struct gl_list_t *
                                                   );
/**< 
 *  struct TypeDescription *CreateLogRelTypeDef(mod,name,sl,pl)
 *  struct module_t *mod;	the module it is defined in
 *  symchar *name;              the name to assign to the logrel type
 *  struct StatementList *sl;	the list of declarative statements
 *  struct gl_list_t *pl;	the list of initialization procedures OR NULL
 */


extern
struct TypeDescription *CreatePatchTypeDef(symchar *,
                                           symchar *,
                                           symchar *,
                                           struct module_t *,
                                           struct StatementList *,
                                           struct gl_list_t *,
                                           unsigned int);
/**< 
 *  struct TypeDescription *
 *  CreatePatchTypeDef(patch,original,orig_mod,sl,pl,err);
 *  const char *name;		name of the patch
 *  const char *original;	name of the original type being patched
 *  const char *orig_mode;	name of module for the orig type or NULL
 *  struct module_t *mod;	the module it is defined in
 *  struct StatementList *sl;	the list of declarative statements OR NULL
 *  struct gl_list_t *pl;	the list of initialization procedures OR NULL
 *  unsigned int err;		if !=0 abandon input
 *  Note that, per ascParse.y, this function destroys several of its
 *  args if it is going to return NULL. (OTHERWISE the args become
 *  parts of the returned object). These destroyed args are:
 *  sl, pl.
 */

extern void DefineFundamentalTypes(void);
/**< 
 *  Define the fundamental and constant basetypes used in ascend.
 *  They will be named following the defines in type_desc.h.
 *
 *  Bugs: doesn't specify the name "relation". Doing so is problematic
 *  wrt instantiate.c.
 */
#endif /**< __TYPEDEF_H_SEEN__ */
