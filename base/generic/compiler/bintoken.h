/* 
 *  bintoken.h
 *  By Benjamin A. Allan
 *  Jan 7, 1998.
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: bintoken.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:37 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

/** @file
 *  Binary tokens implementation for real relation instances.
 *
 *  Note that this header and btprolog.h are a pair.
 *  btprolog exists to make the C compiler job simpler -
 *  we don't want it to know about struct Instance when
 *  building code because we may not have the compiler
 *  directory available. btprolog.h is installed with
 *  the ascend binaries or scripts.
 *  <pre>
 *  When #including bintoken.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef __BINTOKEN_H_SEEN__
#define __BINTOKEN_H_SEEN__

enum bintoken_kind {
  BT_error,
  BT_C,
  BT_F77,     /**< ansi f77, unimplemented */
  BT_SunJAVA, /**< Sun JAVA, unimplemented */
  BT_MsJAVA   /**< Microsoft(tm) JAVA(hah!), unimplemented */
};

/** 
 * Set the configurations for building code.
 * The string arguments given are kept.
 * They are freed on the next call which specifies a new string or NULL.
 * Strings given should not be allocated from tcl.
 * This function must be called before Create is.<br><br>
 *
 * err = BinTokenSetOptions(srcname, objname, libname, buildcommand,
 *                          unlinkcommand, maxreln, verbose, housekeep);<br><br>
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
extern int BinTokenSetOptions(char *srcname,
                              char *objname,
                              char *libname,
                              char *buildcommand,
                              char *unlinkcommand,
                              unsigned long maxreln,
                              int verbose,
                              int housekeep);

/**
 * <!--  BinTokenClearTables();                                        -->
 * Frees global data allocated during loading.
 * Do not call any previously loaded functions after this is
 * executed. Generally, this should only be called at shutdown.
 */
extern void BinTokenClearTables(void);

/**
 * <!--  BinTokenDeleteReference(btable);                              -->
 * When all the references expire, we might unload the library.
 * Note there is no AddReference since all the references
 * are made 1 per share at load time.
 * This should be called each time a share that references
 * btable is destroyed, not each time a relation is destroyed.
 */
extern void BinTokenDeleteReference(int btable);

/**
 * <!--  BinTokensCreate(root,method);                                 -->
 * Searches for unbinary equations in the tree of root and
 * compiles them to source, then object, then dynamically loaded
 * library. Then associates the compiled code to the equations.
 * The language and compiler tools are determined from method.
 */
extern void BinTokensCreate(struct Instance *root, enum bintoken_kind method);

/**
 * <!--  err = BinTokenCalcResidual(btable,bindex,vars,residual);      -->
 * Calculates residual of relation indicated by btable and bindex
 * using the data in vars and putting the result in residual.
 * Vars is assumed already filled with values.
 * This function is SIGFPE safe.
 * Returns an error code (0 if ok, 1 if couldn't evaluate function).
 * May be safely called on token relation instances only.
 */
extern int BinTokenCalcResidual(int btable, int bindex, double *vars, double *residual);

/**
 * <!--  err = BinTokenCalcGradient(int,int,vars,residual,gradient);   -->
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

#endif  /* __BINTOKEN_H_SEEN__ */

