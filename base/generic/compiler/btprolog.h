/*
 *  btprolog.h
 *  By Benjamin A. Allan
 *  Jan 7, 1998.
 *  Part of ASCEND
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: btprolog.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:40 $
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
 *  Prolog for C to be compiled and dynamically loaded to provide
 *  residuals and gradients to token relations. This defines the
 *  math functions ascend will provide after loading the
 *  necessary system headers.<br><br>
 *
 *  Don't put anything in this prolog which requires access to
 *  other ASCEND sources. This header must be shipped with the
 *  binary distribution in some lib directory.<br><br>
 *
 *  bintoken.c also includes this so we maintain 1 definition
 *  of our structs. bintoken.h must include it AFTER func.h
 *  in bintoken.c.
 *  <pre>
 *  When #including btprolog.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  <pre>
 *
 *  @todo Complete documentation of btprolog.h.
 */

#ifndef ASC_BTPROLOG_H
#define ASC_BTPROLOG_H

#ifdef ASC_BINTOKEN_H
# include "utilities/ascConfig.h"
# include "compiler/instance_enum.h"
#else
# define IS_BINTOKEN_COMPILE
# include <ascConfig.h>
# include <instance_enum.h>
#endif

#include <math.h>

#ifndef NULL
#ifdef __alpha
#define NULL 0L
#else
#define NULL 0
#endif
#endif

#define BinTokenGRADIENT 0
#define BinTokenRESIDUAL 1

#ifdef __STDC__
/**  Residual evaluation function pointer.  F(vars,resid); */
typedef void (*BinTokenFPtr)(double *, double *);
/**  Gradient evaluation function pointer.  G(vars,grad,resid); */
typedef void (*BinTokenGPtr)(double *, double *, double *);
/**
 * F77 style interface code (if and big goto required inside).
 * S(vars,grad,resid,ForG,bindex,status);
 */
typedef void (*BinTokenSPtr)(double *, double *, double *, int *, int *, int *);
#else
/**  Residual evaluation function pointer.  F(vars,resid); */
typedef void (*BinTokenFPtr)();
/**  Gradient evaluation function pointer.  G(vars,grad,resid); */
typedef void (*BinTokenGPtr)();
/**
 * F77 style interface code (if and big goto required inside).
 * S(vars,grad,resid,ForG,bindex,status);
 */
typedef void (*BinTokenSPtr)();
#endif /* __STDC__ */

struct TableC {
  BinTokenFPtr F;
  BinTokenGPtr G;
};

struct TableF {
  BinTokenSPtr S;
};

union TableUnion {
  struct TableC c;
  struct TableF f;
};

#ifdef __STDC__
extern int ASC_DLLSPEC ExportBinTokenCTable(struct TableC *t, int size);
#else
extern int ASC_DLLSPEC ExportBinTokenCTable();
#endif /* __STDC__ */

/*------------------------------
  The following is stripped from compiler func.h.
  If a new function is introduced into ASCEND and func.h, fix this.
*/
#ifndef ASC_FUNC_H

#  ifdef __STDC__
#   if __STDC__
/*
 * stdc==1 --> erf, cbrt not defined in headers. user should link
 * against a library that does provide them. ASCEND is research
 * code: we aren't going to waste time reimplementing these basic
 * functions.
 */

extern double ASC_DLLSPEC cbrt(double x);
#    ifdef HAVE_ERF
extern double ASC_DLLSPEC erf(double x);
#    endif /* HAVE_ERF */
#   endif /* __STDC__ == 1 */
/*
 * in the case where __STDC__ is defined but == 0, system headers
 * should provide cbrt, erf.
 */
extern int ASC_DLLSPEC ascnintF(double x);
extern double ASC_DLLSPEC dln(double x);
extern double ASC_DLLSPEC dln2(double x);
extern double ASC_DLLSPEC dlog10(double x);
extern double ASC_DLLSPEC dlog102(double x);
extern double ASC_DLLSPEC lnm(double x);
extern double ASC_DLLSPEC dlnm(double x);
extern double ASC_DLLSPEC dlnm2(double x);
extern double ASC_DLLSPEC dtanh(double x);
extern double ASC_DLLSPEC dtanh2(double x);
extern double ASC_DLLSPEC arcsinh(double x);
extern double ASC_DLLSPEC arccosh(double x);
extern double ASC_DLLSPEC arctanh(double x);
extern double ASC_DLLSPEC darcsinh(double x);
extern double ASC_DLLSPEC darcsinh2(double x);
extern double ASC_DLLSPEC darccosh(double x);
extern double ASC_DLLSPEC darccosh2(double x);
extern double ASC_DLLSPEC darctanh(double x);
extern double ASC_DLLSPEC darctanh2(double x);
extern double ASC_DLLSPEC sqr(double x);
extern double ASC_DLLSPEC dsqr(double x);
extern double ASC_DLLSPEC dsqr2(double x);
extern double ASC_DLLSPEC cube(double x);
extern double ASC_DLLSPEC dcube(double x);
extern double ASC_DLLSPEC dcube2(double x);
extern double ASC_DLLSPEC asc_ipow(double x, int y);
extern double ASC_DLLSPEC asc_d1ipow(double x, int y);
extern double ASC_DLLSPEC asc_d2ipow(double x, int y);
extern double ASC_DLLSPEC hold(double x);
extern double ASC_DLLSPEC dsqrt(double x);
extern double ASC_DLLSPEC dsqrt2(double x);
extern double ASC_DLLSPEC dcbrt(double x);
extern double ASC_DLLSPEC dcbrt2(double x);
extern double ASC_DLLSPEC dfabs(double x);
extern double ASC_DLLSPEC dfabs2(double x);
extern double ASC_DLLSPEC dhold(double x);
extern double ASC_DLLSPEC dasin(double x);
extern double ASC_DLLSPEC dasin2(double x);
extern double ASC_DLLSPEC dcos(double x);
extern double ASC_DLLSPEC dcos2(double x);
extern double ASC_DLLSPEC dacos(double x);
extern double ASC_DLLSPEC dacos2(double x);
extern double ASC_DLLSPEC dtan(double x);
extern double ASC_DLLSPEC dtan2(double x);
extern double ASC_DLLSPEC datan(double x);
extern double ASC_DLLSPEC datan2(double x);
extern double ASC_DLLSPEC derf(double x);
extern double ASC_DLLSPEC derf2(double x);

#  else /* no stdc */

extern double ASC_DLLSPEC cbrt();
#   ifdef HAVE_ERF
extern double ASC_DLLSPEC erf();
#   endif /* HAVE_ERF */
extern int ASC_DLLSPEC ascnintF();
extern double ASC_DLLSPEC dln();
extern double ASC_DLLSPEC dln2();
extern double ASC_DLLSPEC dlog();
extern double ASC_DLLSPEC dlog2();
extern double ASC_DLLSPEC lnm();
extern double ASC_DLLSPEC dlnm();
extern double ASC_DLLSPEC dlnm2();
extern double ASC_DLLSPEC dtanh();
extern double ASC_DLLSPEC dtanh2();
extern double ASC_DLLSPEC arcsinh();
extern double ASC_DLLSPEC arccosh();
extern double ASC_DLLSPEC arctanh();
extern double ASC_DLLSPEC darcsinh();
extern double ASC_DLLSPEC darcsinh2();
extern double ASC_DLLSPEC darccosh();
extern double ASC_DLLSPEC darccosh2();
extern double ASC_DLLSPEC darctanh();
extern double ASC_DLLSPEC darctanh2();
extern double ASC_DLLSPEC sqr();
extern double ASC_DLLSPEC dsqr();
extern double ASC_DLLSPEC dsqr2();
extern double ASC_DLLSPEC cube();
extern double ASC_DLLSPEC dcube();
extern double ASC_DLLSPEC dcube2();
extern double ASC_DLLSPEC asc_ipow();
extern double ASC_DLLSPEC asc_d1ipow();
extern double ASC_DLLSPEC asc_d2ipow();
extern double ASC_DLLSPEC hold();
extern double ASC_DLLSPEC dsqrt();
extern double ASC_DLLSPEC dsqrt2();
extern double ASC_DLLSPEC dcbrt();
extern double ASC_DLLSPEC dcbrt2();
extern double ASC_DLLSPEC dfabs();
extern double ASC_DLLSPEC dfabs2();
extern double ASC_DLLSPEC dhold();
extern double ASC_DLLSPEC dasin();
extern double ASC_DLLSPEC dasin2();
extern double ASC_DLLSPEC dcos();
extern double ASC_DLLSPEC dcos2();
extern double ASC_DLLSPEC dacos();
extern double ASC_DLLSPEC dacos2();
extern double ASC_DLLSPEC dtan();
extern double ASC_DLLSPEC dtan2();
extern double ASC_DLLSPEC datan();
extern double ASC_DLLSPEC datan2();
extern double ASC_DLLSPEC derf();
extern double ASC_DLLSPEC derf2();

#  endif  /* no stdc */
# endif  /* fake ASC_FUNC_H */
#endif  /* ASC_BT_PROLOG */

