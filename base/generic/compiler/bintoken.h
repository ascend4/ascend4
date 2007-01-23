/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1998 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
 *  Binary tokens implementation for real relation instances.
 *
 *  Note that this header and btprolog.h are a pair.
 *  btprolog exists to make the C compiler job simpler -
 *  we don't want it to know about struct Instance when
 *  building code because we may not have the compiler
 *  directory available. btprolog.h is installed with
 *  the ascend binaries or scripts.
*//*
	By Benjamin A. Allan
	Jan 7, 1998.
	Last in CVS: $Revision: 1.2 $ $Date: 1998/06/16 16:38:37 $ $Author: mthomas $
*/

#ifndef ASC_BINTOKEN_H
#define ASC_BINTOKEN_H

#include <utilities/ascConfig.h>
#include "compiler.h"

/**	@addtogroup compiler Compiler
	@{
*/

enum bintoken_kind {
  BT_error,
  BT_C,
  BT_F77 /**< ansi f77, unimplemented */
};

/**
 * Set the configurations for building code.
 * This function must be called before Create is.<br><br>
 *
 * srcname, objname, libname, buildcommand, unlinkcommand
 * are all OS/compiler specific.
 * maxreln is the largest number of relations to be allowed in a
 * single generated C file.
 * verbose, if nonzero, causes human-edible comments in generated
 * code. housekeep, if given, will cause limited OS housekeeping
 * of unneeded files; specifically $srcname, objname will be deleted
 * after a successful link.
 */
ASC_DLLSPEC int BinTokenSetOptions(
	CONST char *srcname, CONST char *objname, CONST char *libname
	,CONST char *buildcommand, CONST char *unlinkcommand
	,unsigned long maxreln
	,int verbose, int housekeep
);

/**
 * Frees global data allocated during loading.
 * Do not call any previously loaded functions after this is
 * executed. Generally, this should only be called at shutdown.
 */
extern void BinTokenClearTables(void);

/**
 * When all the references expire, we might unload the library.
 * Note there is no AddReference since all the references
 * are made 1 per share at load time.
 * This should be called each time a share that references
 * btable is destroyed, not each time a relation is destroyed.
 */
extern void BinTokenDeleteReference(int btable);

/**
 * Searches for unbinary equations in the tree of root and
 * compiles them to source, then object, then dynamically loaded
 * library. Then associates the compiled code to the equations.
 * The language and compiler tools are determined from method.
 */
extern void BinTokensCreate(struct Instance *root, enum bintoken_kind method);

/**
 * Calculates residual of relation indicated by btable and bindex
 * using the data in vars and putting the result in residual.
 * Vars is assumed already filled with values.
 * This function is SIGFPE safe.
 * Returns an error code (0 if ok, 1 if couldn't evaluate function).
 * May be safely called on token relation instances only.
 */
extern int BinTokenCalcResidual(int btable, int bindex, double *vars, double *residual);

/**
 * Calculates gradient of relation indicated by btable and bindex
 * using the data in vars and putting the result in gradient.
 * Returns nonzero if can't evaluate gradient using binary
 * form of token relation.
 * Vars is assumed already filled with values.
 * Residual is free anyway, so we calculate it, too.
 * May be safely called on token relation instances only.
 */
extern int BinTokenCalcGradient(int btable, int bindex, double *vars,
                                double *residual, double *gradient);

/* @} */

#endif  /* ASC_BINTOKEN_H */

