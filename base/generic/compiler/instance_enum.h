/**< 
 *  Ascend Instance Tree Type Definitions
 *  by Ben Allan
 *  6/02/96
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: instance_enum.h,v $
 *  Date last modified: $Date: 1997/07/18 12:30:11 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1996 Benjamin Andrew Allan
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

#ifndef __INSTANCE_ENUM_H_SEEN__
#define __INSTANCE_ENUM_H_SEEN__

/**< 
 *  When #including instance_enum.h, make sure these files are #included first:
 *         NO INCLUDES NEEDED
 */


/**< 
 *  Notes on the instance type scheme.
 *      Ben Allan, Feb. 1996.
 *  We have three types of 'scalar data' instances:
 *  *_INST, *_ATOM_INST, and *_CONSTANT_INST,
 *  where * is REAL, INTEGER, BOOLEAN, SYMBOL and SET.
 *  (Sets are a little weird and should perhaps not be grouped in *.)
 *  The word Atom is used a little loosely in naming of functions in
 *  this file. Many places it implies any of these types, but in some
 *  it implies exclusively *_ATOM_INST types.
 *     READ YOUR HEADERS CAREFULLY.
 *  Atoms are the 'variables' in the ASCEND language.
 *  Constants are the 'structural variables' and 'real constants'.
 *  *_INST are the children of Atoms.
 *  Constants do not have children and cannot be children of Atoms.
 *  Constants may be refined up until they are actually assigned
 *  a value (and in the case of reals a dimensionality).
 *  Constants in a clique (bad style, imho) all have the same value/dims:
 *  assigning one assigns them all. ATS is usually a better style.
 *
 *  Future work:
 *  LIST replaces SET in the Atom group; the atomic SET becoming the
 *  constant SET.
 *  REAL_ATOM_INST becomes the parametric reals of a model and
 *  a new SOLVER_ATOM_INST becomes the real variables of a model.
 */
#define IREAL	0x1
#define IINT	0x2
#define IBOOL	0x4
#define ISYM	0x8
#define ISET	0x10
#define IARR	0x20
#define IENUM	0x40
#define IFUND	0x80
#define ICONS	0x100
#define IATOM	0x200
#define ICOMP	0x400
#define IMOD 	0x800
#define IRELN	0x1000
#define ILRELN  0x2000
#define IWHEN   0x4000
#define IAUTO   0x8000
#define IDUMB   0x10000
/**< future work */
#define ICHILDLESS (IFUND | ICONS | IWHEN | IDUMB)
/**< constants and fundamental types have no child list */
/**< when's have no children.  Better way of doing this?  */
#define IERRINST ~(IREAL|IINT|IBOOL|ISYM|ISET|IARR|IENUM|IFUND|ICONS|IATOM| \
 ICOMP|IMOD|IRELN|ILRELN|IWHEN|IAUTO|IDUMB)
/**< any of these bits on is an error */
enum inst_t {
  ERROR_INST =		0,		/**< deleted instances get this type*/
  SIM_INST =		ICOMP,		/**< a simulation instance */
  MODEL_INST =		ICOMP | IMOD,	/**< model instance */
  /**< AUTO_INST = ICOMP | IMOD | IAUTO, */ /**< future stack instance */
  REL_INST =		IRELN,		/**< relation(equality or inequality) */
  LREL_INST =		ILRELN,		/**< logical relation( == || != ) */
  WHEN_INST =           IWHEN,          /**< WHEN instance  */
  ARRAY_INT_INST =	ICOMP | IARR  | IINT,	/**< an array instance integer */
  ARRAY_ENUM_INST = 	ICOMP | IARR  | ISYM,	/**< an array instance enumed */
  /**< fundamental instances */
  REAL_INST =		IFUND | IREAL,
  INTEGER_INST =	IFUND | IINT,
  BOOLEAN_INST =	IFUND | IBOOL,
  SYMBOL_INST =		IFUND | ISYM,
  SET_INST =		IFUND | ISET,
  /**< nonfundamental atomic instances */
  REAL_ATOM_INST =	IATOM | IREAL,
  INTEGER_ATOM_INST =	IATOM | IINT,
  BOOLEAN_ATOM_INST =	IATOM | IBOOL,
  SYMBOL_ATOM_INST =	IATOM | ISYM,
  SET_ATOM_INST =	IATOM | ISET,
  /**< nonfundamental constant instances */
  REAL_CONSTANT_INST =		ICONS | IREAL,
  BOOLEAN_CONSTANT_INST =	ICONS | IINT,
  INTEGER_CONSTANT_INST =	ICONS | IBOOL,
  SYMBOL_CONSTANT_INST =	ICONS | ISYM,
  /**< dummy instance - unselected IS_A children. */
  DUMMY_INST =		IDUMB
};

/**< Never, ever, allocate either one of these types */
struct Instance {
  enum inst_t t;
};

#endif /**< __INSTANCE_ENUM_H_SEEN__ */
