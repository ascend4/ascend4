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

/**	addtogroup compiler Compiler
	@{
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
/**<
	any of these bits on is an error 
	@TODO explain this
*/

/**
	Values for use in declaration of enum inst_t.

	@NOTE these enum declarations are done using the '#define list trick' so that
	ASC_ENUM_DECLS can be re-purposed to give an string enum lookup table in
	instance_io.h.
*/
#define ASC_ENUM_DECLS(D,X) \
	D( ERROR_INST,           0) X                  /**< Deleted instance (error). */ \
	D( SIM_INST,             ICOMP) X              /**< Simulation instance. */ \
	D( MODEL_INST,           ICOMP | IMOD) X       /**< Model instance. */ \
	D( REL_INST,             IRELN) X              /**< Relation(equality or inequality). */ \
	D( LREL_INST,            ILRELN) X             /**< Logical relation( == || != ). */ \
	D( WHEN_INST,            IWHEN) X              /**< WHEN instance  */ \
	D( ARRAY_INT_INST,       ICOMP | IARR | IINT) X/**< Array instance integer */ \
	D( ARRAY_ENUM_INST,      ICOMP | IARR | ISYM) X/**< Array instance enumed */ \
	D( REAL_INST,            IFUND | IREAL) X      /**< Real instance. */ \
	D( INTEGER_INST,         IFUND | IINT) X       /**< Int instance. */ \
	D( BOOLEAN_INST,         IFUND | IBOOL) X      /**< Boolean instance. */ \
	D( SYMBOL_INST,          IFUND | ISYM) X       /**< Symbol instance. */ \
	D( SET_INST,             IFUND | ISET) X       /**< Set instance. */ \
	D( REAL_ATOM_INST,       IATOM | IREAL) X      /**< Real atomic instance. */ \
	D( INTEGER_ATOM_INST,    IATOM | IINT) X       /**< Int atomic instance. */ \
	D( BOOLEAN_ATOM_INST,    IATOM | IBOOL) X      /**< Boolean atomic instance. */ \
	D( SYMBOL_ATOM_INST,     IATOM | ISYM) X       /**< Symbol atomic instance. */ \
	D( SET_ATOM_INST,        IATOM | ISET) X       /**< Set atomic instance. */ \
	D( REAL_CONSTANT_INST,   ICONS | IREAL) X      /**< Real constant instance. */ \
	D( BOOLEAN_CONSTANT_INST,ICONS | IINT) X       /**< Boolean constant instance. */ \
	D( INTEGER_CONSTANT_INST,ICONS | IBOOL) X      /**< Int constant instance. */ \
	D( SYMBOL_CONSTANT_INST, ICONS | ISYM) X       /**< Symbol constant instance. */ \
	D( DUMMY_INST,           IDUMB)                /**< Dummy instance - unselected IS_A children. */
    /* AUTO_INST = ICOMP | IMOD | IAUTO, */ /* future stack instance, would come after 'MODEL_INST' */

/**
	Instance types, see also comments at head of instance_enum.h.

	There are 'fundamental instances' which include REAL, INTEGER, BOOLEAN,
	SYMBOL and SET (suffixed with _INST).

	There are nonfundamental 'atomic' instances, REAL, INTEGER, BOOLEAR, SYMBOL
	and SET, (this time suffixed with _ATOM_INST).

	There are nonfundamental constant instances, REAL, BOOLEAR, INTEGER and
	SYMBOL (suffixed with _CONSTANT_INST).

	Then there are some special instances:
	  - ERROR_INST -- a deleted instance
	  - SIM_INST -- a simulation
	  - MODEL_INST -- a model instance (used where...)
	  - REL_INST -- a 'relation' i.e. equality or inequality (real-valued LHS and RHS)
	  - LREL_INST -- a logical relation (true or false) (An '==' or '!=' between LHS and RHS).
	  - WHEN_INST -- ...
	  - ARRAY_INT_INST -- 'array instance integer'...
	  - ARRAY_ENUM_INST -- 'array instance enumed'...
	  - DUMMY_INST -- dummy instance - unselected 'IS_A' children.

	@see InstanceEnumLookup in instance_io.c
*/
enum inst_t {
#define ENUM_D(NAME,VALUE) NAME = VALUE
#define ENUM_X ,
	ASC_ENUM_DECLS(ENUM_D,ENUM_X)
#undef ENUM_D
#undef ENUM_X
};

/** 
	@NOTE *NEVER* allocate either one of these types! See instead the file
	instance_types.h. Note that all 'struct Instance *' types share the
	leading 't'	member, but other than that they are not unionised so they
	are not all of the same length. So you need to be careful when making
	assumptions about the type of Instance (eg RealAtomInstance, etc) that you 
	have.
*/
struct Instance {
  enum inst_t t;
};

/* @} */

#endif /* ASC_INSTANCE_ENUM_H */
