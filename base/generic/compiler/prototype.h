/*
 *  This Module Store A Copy of Atom Instances
 *  by Tom Epperly
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: prototype.h,v $
 *  Date last modified: $Date: 1998/02/05 16:37:30 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/*
 *  When #including prototype.h, make sure these files are #included first:
 *         #include "compiler.h"
 */


#ifndef __PROTOTYPE_H_SEEN__
#define __PROTOTYPE_H_SEEN__
/* requires
# #include"compiler.h"
# #include"instance_enum.h"
*/


/*
 *  Given the amount of time we spend looking for prototypes, we need
 *  to be running distinct prototype libraries for models, atoms, and
 *  possibly constants.
 */

extern void InitializePrototype(void);
/*
 *  Must be called before any other prototype procedure.
 */

extern struct Instance *LookupPrototype(symchar *);
/*
 *  struct Instance *LookupPrototype(t)
 *  symchar *t;
 *
 *  Check if an instance of type "t" is in the prototype library.  If
 *  no instance of that type exists, NULL is returned.
 *  t is from symbol table.
 */

extern void DeletePrototype(symchar *);
/*
 *  void DeletePrototype(t)
 *  const char *t;
 *
 *  Delete the type t from the prototype library.  This should be done
 *  when the definition of type "t" is changed or when the definition of
 *  an ancestor of type "t" is change.
 */

extern void AddPrototype(struct Instance *);
/*
 *  void AddPrototype(i)
 *  struct Instance *i;
 *
 *  This will add instance i to the prototype library.  If another definition
 *  of type "i" exists, it is deleted and replace with then new one.
 */

extern void DestroyPrototype(void);
/*
 *  void DestroyPrototype()
 *  This deletes all the instances in the prototype library.  This should
 *  be done before the program exits.
 */
#endif /* __PROTOTYPE_H_SEEN__ */
