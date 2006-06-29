/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//**
	@file
	Ascend Instance Tree Type Definitions

	We have three types of 'scalar data' instances:

	  - *_INST,
	  - *_ATOM_INST, and
      - *_CONSTANT_INST,

	where * is REAL, INTEGER, BOOLEAN, SYMBOL and SET.

	(Sets are a little weird and should perhaps not be grouped under this
	wildcard)

	The word 'atom' is used a little loosely in naming of functions in
	this file. Many places it implies any of these types, but in some
	it implies exclusively *_ATOM_INST types. So read your headers carefully.

	Atoms are the 'variables' in the ASCEND language.

	*_INST are the children of Atoms.

	Constants are the 'structural variables' and 'real constants'.
	Constants do not have children and cannot be children of Atoms.
	Constants may be refined up until they are actually assigned
	a value (and in the case of reals a dimensionality).
	Constants in a clique (bad style, IMHO) all have the same value/dims:
	assigning one assigns them all. ARE_THE_SAME is usually a better style.
	
	@TODO Future work:
	LIST replaces SET in the Atom group; the atomic SET becoming the
	constant SET.
	REAL_ATOM_INST becomes the parametric reals of a model and
	a new SOLVER_ATOM_INST becomes the real variables of a model.

	-- BAA.

	Requires:
	none
*//*
	by Ben Allan
	6/02/96
	Last in CVS: $Revision: 1.8 $ $Date: 1997/07/18 12:30:11 $ $Author: mthomas $
*/

#ifndef ASC_INSTANCE_ENUM_H
#define ASC_INSTANCE_ENUM_H

/**
	@TODO add some macro wizardry here to enable simple reporting of instance
	types as string values.
*/

#define IREAL   0x1
#define IINT    0x2
#define IBOOL   0x4
#define ISYM    0x8
#define ISET    0x10
#define IARR    0x20
#define IENUM   0x40
#define IFUND   0x80
#define ICONS   0x100
#define IATOM   0x200
#define ICOMP   0x400
#define IMOD    0x800
#define IRELN   0x1000
#define ILRELN  0x2000
#define IWHEN   0x4000
#define IAUTO   0x8000
#define IDUMB   0x10000

/** @todo future work */

#define ICHILDLESS (IFUND | ICONS | IWHEN | IDUMB)
/* constants and fundamental types have no child list */
/* when's have no children.  Better way of doing this?  */

#define IERRINST ~(IREAL|IINT|IBOOL|ISYM|ISET|IARR|IENUM|IFUND|ICONS|IATOM| \
 ICOMP|IMOD|IRELN|ILRELN|IWHEN|IAUTO|IDUMB)
/**< any of these bits on is an error */

/** Instance types. */
enum inst_t {
  ERROR_INST =        0,                    /**< Deleted instances get this type. */
  SIM_INST =          ICOMP,                /**< A simulation instance. */
  MODEL_INST =        ICOMP | IMOD,         /**< Model instance. */
  /* AUTO_INST = ICOMP | IMOD | IAUTO, */ /* future stack instance */
  REL_INST =          IRELN,                /**< Relation(equality or inequality). */
  LREL_INST =         ILRELN,               /**< Logical relation( == || != ). */
  WHEN_INST =         IWHEN,                /**< WHEN instance  */
  ARRAY_INT_INST =    ICOMP | IARR | IINT,  /**< Array instance integer */
  ARRAY_ENUM_INST =   ICOMP | IARR | ISYM,  /**< Array instance enumed */
  /* fundamental instances */
  REAL_INST =         IFUND | IREAL,        /**< Real instance. */
  INTEGER_INST =      IFUND | IINT,         /**< Int instance. */
  BOOLEAN_INST =      IFUND | IBOOL,        /**< Boolean instance. */
  SYMBOL_INST =       IFUND | ISYM,         /**< Symbol instance. */
  SET_INST =          IFUND | ISET,         /**< Set instance. */
  /* nonfundamental atomic instances */
  REAL_ATOM_INST =    IATOM | IREAL,        /**< Real atomic instance. */
  INTEGER_ATOM_INST = IATOM | IINT,         /**< Int atomic instance. */
  BOOLEAN_ATOM_INST = IATOM | IBOOL,        /**< Boolean atomic instance. */
  SYMBOL_ATOM_INST =  IATOM | ISYM,         /**< Symbol atomic instance. */
  SET_ATOM_INST =     IATOM | ISET,         /**< Set atomic instance. */
  /* nonfundamental constant instances */
  REAL_CONSTANT_INST =    ICONS | IREAL,    /**< Real constant instance. */
  BOOLEAN_CONSTANT_INST = ICONS | IINT,     /**< Boolean constant instance. */
  INTEGER_CONSTANT_INST = ICONS | IBOOL,    /**< Int constant instance. */
  SYMBOL_CONSTANT_INST =  ICONS | ISYM,     /**< Symbol constant instance. */
  /* dummy instance - unselected IS_A children. */
  DUMMY_INST =  IDUMB                       /**< Dummy instance - unselected IS_A children. */
};

/** Never, ever, allocate either one of these types. */
struct Instance {
  enum inst_t t;
};

#endif /* ASC_INSTANCE_ENUM_H */
