/*
 *  Ascend Instance Tree Type Definitions
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: instance.h,v $
 *  Date last modified: $Date: 1997/07/18 12:30:09 $
 *  Last modified by: $Author: mthomas $
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

#ifndef __INSTANCE_H_SEEN__
#define __INSTANCE_H_SEEN__


/*
 *  When #including instance.h, make sure these files are #included first:
 *         NO INCLUDES NEEDED
 */

/*
 * Been gutted (pun intended) because it was too big.
 * We need an API header for instances (for noncompiler clients)
 * in this location.
 *
 * For the moment:
 * THOU SHALT NOT PUT ANY INCLUDE STATEMENTS IN THIS FILE!
 * probably we will want atomvalue.h, instance_enum.h,
 * instance_name.h and instance_io.h bundled.
 */

/*
 *  Notes on the instance type scheme.
 *      Ben Allan, Feb. 1996.
 *  We have three types of 'scalar data' instances:
 *  *_INST, *_ATOM_INST, and *_CONSTANT_INST,
 *  where * is REAL, INTEGER, BOOLEAN, SYMBOL and SET.
 *  (Sets are a little weird and should perhaps not be grouped in *.)
 *  The word Atom is used a little loosely in naming of functions in
 *  the compiler. Many places it implies any of these types, but in some
 *  it implies only *_ATOM_INST types.
 *     READ YOUR HEADERS CAREFULLY to distinguish.
 *  Atoms are the 'variables' in the ASCEND language.
 *  Constants are the 'structural variables' and 'real constants'.
 *  *_INST are the children of Atoms.
 *  Constants do not have children and cannot be children of Atoms.
 *  Constants may be refined up until they are actually assigned
 *  a value (and in the case of reals a dimensionality).
 *  Constants in a clique (very bad style, imho) all have the same value/dims:
 *  assigning one assigns them all. ATS is usually a better style.
 *
 *  Future work:
 *  LIST replaces SET in the Atom group; the atomic SET becoming the
 *  constant SET.
 *  REAL_ATOM_INST becomes the parametric reals of a model and
 *  a new SOLVER_ATOM_INST becomes the real variables of a model.
 */

#endif /* __INSTANCE_H_SEEN__ */
