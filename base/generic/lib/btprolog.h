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

/*
 * prolog for C to be compiled and dynamically loaded to provide
 * residuals and gradients to token relations. This defines the
 * math functions ascend will provide after loading the
 * necessary system headers.
 * Don't put anything in this prolog which requires access to
 * other ascend sources. This header must be shipped with the
 * binary distribution in some lib directory.
 *
 * bintoken.c also includes this so we maintain 1 definition
 * of our structs. bintoken.h must include it AFTER func.h
 * in bintoken.c.
 */
#ifndef __BTPROLOG_H_SEEN__
#define __BTPROLOG_H_SEEN__

#ifndef _ASCCONFIG_H /* then this is being used to build a dynamic library,
                      * so we reverse the import/export definitions.
                      */
#ifdef WIN32
/* two for use in this file */
#define DLEXPORT __declspec(dllimport)
#define DLIMPORT __declspec(dllexport)
/* two for use in the generated file */
#define IMPORT __declspec(dllimport)
#define EXPORT __declspec(dllexport)
#else /* not win32 */
/* four for use in either file when the operating system is not brain dead */
#define DLEXPORT
#define DLIMPORT
#define EXPORT
#define IMPORT
#endif /* WIN32 */
#endif /* _ASCCONFIG_H */

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

/*
 * residual evaluation function pointer.
 * F(vars,resid);
 * gradient evaluation function pointer.
 * G(vars,grad,resid);
 * F77 style interface code (if and big goto required inside)
 * S(vars,grad,resid,ForG,bindex,status);
 */
#ifdef __STDC__
typedef void (*BinTokenFPtr)(double *, double *);
typedef void (*BinTokenGPtr)(double *, double *, double *);
typedef void (*BinTokenSPtr)(double *, double *, double *, int *, int *, int *);
#else
typedef void (*BinTokenFPtr)();
typedef void (*BinTokenGPtr)();
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
extern int DLEXPORT ExportBinTokenCTable(struct TableC *, int);
#else
extern int DLEXPORT ExportBinTokenCTable();
#endif /* __STDC__ */

#ifndef __FUNC_H_SEEN__
/* The following is stripped from compiler func.h.
 * If a new function is introduced into ASCEND and func.h, fix this.
 */

#ifdef __STDC__
#if __STDC__
/*
 * stdc==1 --> erf, cbrt not defined in headers. user should link
 * against a library that does provide them. ASCEND is research
 * code: we aren't going to waste time reimplementing these basic
 * functions.
 */
extern double DLEXPORT cbrt(double);
#ifdef HAVE_ERF
extern double DLEXPORT erf(double);
#endif /* HAVE_ERF */
#endif /* __STDC__ == 1 */
/*
 * in the case where __STDC__ is defined but == 0, system headers
 * should provide cbrt, erf.
 */
extern int DLEXPORT ascnintF(double);
extern double DLEXPORT dln(double);
extern double DLEXPORT dln2(double);
extern double DLEXPORT dlog10(double);
extern double DLEXPORT dlog102(double);
extern double DLEXPORT lnm(double);
extern double DLEXPORT dlnm(double);
extern double DLEXPORT dlnm2(double);
extern double DLEXPORT dtanh(double);
extern double DLEXPORT dtanh2(double);
extern double DLEXPORT arcsinh(double);
extern double DLEXPORT arccosh(double);
extern double DLEXPORT arctanh(double);
extern double DLEXPORT darcsinh(double);
extern double DLEXPORT darcsinh2(double);
extern double DLEXPORT darccosh(double);
extern double DLEXPORT darccosh2(double);
extern double DLEXPORT darctanh(double);
extern double DLEXPORT darctanh2(double);
extern double DLEXPORT sqr(double);
extern double DLEXPORT dsqr(double);
extern double DLEXPORT dsqr2(double);
extern double DLEXPORT cube(double);
extern double DLEXPORT dcube(double);
extern double DLEXPORT dcube2(double);
extern double DLEXPORT asc_ipow(double,int);
extern double DLEXPORT asc_d1ipow(double,int);
extern double DLEXPORT asc_d2ipow(double,int);
extern double DLEXPORT hold(double);
extern double DLEXPORT dsqrt(double);
extern double DLEXPORT dsqrt2(double);
extern double DLEXPORT dcbrt(double);
extern double DLEXPORT dcbrt2(double);
extern double DLEXPORT dfabs(double);
extern double DLEXPORT dfabs2(double);
extern double DLEXPORT dhold(double);
extern double DLEXPORT dasin(double);
extern double DLEXPORT dasin2(double);
extern double DLEXPORT dcos(double);
extern double DLEXPORT dcos2(double);
extern double DLEXPORT dacos(double);
extern double DLEXPORT dacos2(double);
extern double DLEXPORT dtan(double);
extern double DLEXPORT dtan2(double);
extern double DLEXPORT datan(double);
extern double DLEXPORT datan2(double);
extern double DLEXPORT derf(double);
extern double DLEXPORT derf2(double);

#else /* no stdc */

extern double DLEXPORT cbrt();
#ifdef HAVE_ERF
extern double DLEXPORT erf();
#endif /* HAVE_ERF */
extern int DLEXPORT ascnintF();
extern double DLEXPORT dln();
extern double DLEXPORT dln2();
extern double DLEXPORT dlog10();
extern double DLEXPORT dlog102();
extern double DLEXPORT lnm();
extern double DLEXPORT dlnm();
extern double DLEXPORT dlnm2();
extern double DLEXPORT dtanh();
extern double DLEXPORT dtanh2();
extern double DLEXPORT arcsinh();
extern double DLEXPORT arccosh();
extern double DLEXPORT arctanh();
extern double DLEXPORT darcsinh();
extern double DLEXPORT darcsinh2();
extern double DLEXPORT darccosh();
extern double DLEXPORT darccosh2();
extern double DLEXPORT darctanh();
extern double DLEXPORT darctanh2();
extern double DLEXPORT sqr();
extern double DLEXPORT dsqr();
extern double DLEXPORT dsqr2();
extern double DLEXPORT cube();
extern double DLEXPORT dcube();
extern double DLEXPORT dcube2();
extern double DLEXPORT asc_ipow();
extern double DLEXPORT asc_d1ipow();
extern double DLEXPORT asc_d2ipow();
extern double DLEXPORT hold();
extern double DLEXPORT dsqrt();
extern double DLEXPORT dsqrt2();
extern double DLEXPORT dcbrt();
extern double DLEXPORT dcbrt2();
extern double DLEXPORT dfabs();
extern double DLEXPORT dfabs2();
extern double DLEXPORT dhold();
extern double DLEXPORT dasin();
extern double DLEXPORT dasin2();
extern double DLEXPORT dcos();
extern double DLEXPORT dcos2();
extern double DLEXPORT dacos();
extern double DLEXPORT dacos2();
extern double DLEXPORT dtan();
extern double DLEXPORT dtan2();
extern double DLEXPORT datan();
extern double DLEXPORT datan2();
extern double DLEXPORT derf();
extern double DLEXPORT derf2();

#endif /* no stdc */
#endif /* fake__FUNC_H_SEEN__ */
#endif /* __BTPROLOG_H_SEEN__ */
