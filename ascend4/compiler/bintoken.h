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

/* Note that this header and btprolog.h are a pair.
 * btprolog exists to make the C compiler job simpler -
 * we don't want it to know about struct Instance when
 * building code because we may not have the compiler
 * directory available. btprolog.h is installed with
 * the ascend binaries or scripts.
 */
/*
 * this header requires:
 * #include "compiler/instance_enum.h"
 */
#ifndef __BINTOKEN_H_SEEN__
#define __BINTOKEN_H_SEEN__

enum bintoken_kind {
  BT_error,
  BT_C,
  BT_F77,	/* ansi f77, unimplemented */
  BT_SunJAVA,	/* Sun JAVA, unimplemented */
  BT_MsJAVA	/* Microsoft(tm) JAVA(hah!), unimplemented */
};

/*
 * Set the configurations for building code.
 * The string arguments given are kept.
 * They are freed on the next call which specifies a new string or NULL.
 * Strings given should not be allocated from Tcl.
 * This function must be called before Create is.
 *
 * err = BinTokenSetOptions(srcname, objname, libname, buildcommand,
 *                          unlinkcommand, maxreln, verbose, housekeep);
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
extern int BinTokenSetOptions(char *, char *, char *, char *, char *,
                              unsigned long, int, int);

/*
 * BinTokenClearTables();
 * Frees global data allocated during loading.
 * Do not call any previously loaded functions after this is
 * executed. Generally, this should only be called at shutdown.
 */
extern void BinTokenClearTables(void);

/*
 * BinTokenDeleteReference(btable);
 * When all the references expire, we might unload the library.
 * Note there is no AddReference since all the references
 * are made 1 per share at load time.
 * This should be called each time a share that references
 * btable is destroyed, not each time a relation is destroyed.
 */
extern void BinTokenDeleteReference(int);

/*
 * BinTokensCreate(root,method);
 * Searches for unbinary equations in the tree of root and
 * compiles them to source, then object, then dynamically loaded
 * library. Then associates the compiled code to the equations.
 * The language and compiler tools are determined from method.
 */
extern void BinTokensCreate(struct Instance *, enum bintoken_kind);

/*
 * err = BinTokenCalcResidual(btable,bindex,vars,residual);
 * Returns 1 if can't evaluate function.
 * Vars is assumed already filled with values.
 * Calculates residual of relation indicated by btable and bindex
 * using the data in vars and putting the result in residual.
 * This function is SIGFPE safe.
 * Returns 0 if ok, 1 if not.
 * May be safely called on token relation instances only.
 */
extern int BinTokenCalcResidual(int, int, double *, double *);

/*
 * err = BinTokenCalcGradient(int,int,vars,residual,gradient);
 * Returns nonzero if can't evaluate gradient using binary
 * form of token relation.
 * Vars is assumed already filled with values.
 * Residual is free anyway, so we calc it, too.
 * May be safely called on token relation instances only.
 */
extern int BinTokenCalcGradient(int, int, double *,double *,double *);
#endif
