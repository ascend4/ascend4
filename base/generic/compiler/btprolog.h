/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	Prolog for C to be compiled and dynamically loaded to provide
	residuals and gradients to token relations. This defines the
	math functions ascend will provide after loading the
	necessary system headers.

	Don't put anything in this prolog which requires access to
	other ASCEND sources. This header must be shipped with the
	binary distribution in some lib directory.

	bintoken.c also includes this so we maintain 1 definition
	of our structs. bintoken.h must include it AFTER func.h
	in bintoken.c.

	@TODO Complete documentation of btprolog.h.

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
*//*
	By Benjamin A. Allan
	Jan 7, 1998.
	Part of ASCEND
	Version: $Revision: 1.3 $
	Version control file: $RCSfile: btprolog.h,v $
	Date last modified: $Date: 1998/06/16 16:38:40 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_BTPROLOG_H
#define ASC_BTPROLOG_H

#ifdef ASC_BINTOKEN_H
# include <utilities/ascConfig.h>
# include <compiler/instance_enum.h>
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
ASC_DLLSPEC int ExportBinTokenCTable(struct TableC *t, int size);
#else
ASC_DLLSPEC int ExportBinTokenCTable();
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

ASC_DLLSPEC double cbrt(double x);
#    ifdef HAVE_ERF
ASC_DLLSPEC double erf(double x);
#    endif /* HAVE_ERF */
#   endif /* __STDC__ == 1 */
/*
 * in the case where __STDC__ is defined but == 0, system headers
 * should provide cbrt, erf.
 */
ASC_DLLSPEC int ascnintF(double x);
ASC_DLLSPEC double dln(double x);
ASC_DLLSPEC double dln2(double x);
ASC_DLLSPEC double dlog10(double x);
ASC_DLLSPEC double dlog102(double x);
ASC_DLLSPEC double lnm(double x);
ASC_DLLSPEC double dlnm(double x);
ASC_DLLSPEC double dlnm2(double x);
ASC_DLLSPEC double dtanh(double x);
ASC_DLLSPEC double dtanh2(double x);
ASC_DLLSPEC double arcsinh(double x);
ASC_DLLSPEC double arccosh(double x);
ASC_DLLSPEC double arctanh(double x);
ASC_DLLSPEC double darcsinh(double x);
ASC_DLLSPEC double darcsinh2(double x);
ASC_DLLSPEC double darccosh(double x);
ASC_DLLSPEC double darccosh2(double x);
ASC_DLLSPEC double darctanh(double x);
ASC_DLLSPEC double darctanh2(double x);
ASC_DLLSPEC double sqr(double x);
ASC_DLLSPEC double dsqr(double x);
ASC_DLLSPEC double dsqr2(double x);
ASC_DLLSPEC double cube(double x);
ASC_DLLSPEC double dcube(double x);
ASC_DLLSPEC double dcube2(double x);
ASC_DLLSPEC double asc_ipow(double x, int y);
ASC_DLLSPEC double asc_d1ipow(double x, int y);
ASC_DLLSPEC double asc_d2ipow(double x, int y);
ASC_DLLSPEC double hold(double x);
ASC_DLLSPEC double dsqrt(double x);
ASC_DLLSPEC double dsqrt2(double x);
ASC_DLLSPEC double dcbrt(double x);
ASC_DLLSPEC double dcbrt2(double x);
ASC_DLLSPEC double dfabs(double x);
ASC_DLLSPEC double dfabs2(double x);
ASC_DLLSPEC double dhold(double x);
ASC_DLLSPEC double dasin(double x);
ASC_DLLSPEC double dasin2(double x);
ASC_DLLSPEC double dcos(double x);
ASC_DLLSPEC double dcos2(double x);
ASC_DLLSPEC double dacos(double x);
ASC_DLLSPEC double dacos2(double x);
ASC_DLLSPEC double dtan(double x);
ASC_DLLSPEC double dtan2(double x);
ASC_DLLSPEC double datan(double x);
ASC_DLLSPEC double datan2(double x);
ASC_DLLSPEC double derf(double x);
ASC_DLLSPEC double derf2(double x);

#  else /* no stdc */

ASC_DLLSPEC double cbrt();
#   ifdef HAVE_ERF
ASC_DLLSPEC double erf();
#   endif /* HAVE_ERF */
ASC_DLLSPEC int ascnintF();
ASC_DLLSPEC double dln();
ASC_DLLSPEC double dln2();
ASC_DLLSPEC double dlog();
ASC_DLLSPEC double dlog2();
ASC_DLLSPEC double lnm();
ASC_DLLSPEC double dlnm();
ASC_DLLSPEC double dlnm2();
ASC_DLLSPEC double dtanh();
ASC_DLLSPEC double dtanh2();
ASC_DLLSPEC double arcsinh();
ASC_DLLSPEC double arccosh();
ASC_DLLSPEC double arctanh();
ASC_DLLSPEC double darcsinh();
ASC_DLLSPEC double darcsinh2();
ASC_DLLSPEC double darccosh();
ASC_DLLSPEC double darccosh2();
ASC_DLLSPEC double darctanh();
ASC_DLLSPEC double darctanh2();
ASC_DLLSPEC double sqr();
ASC_DLLSPEC double dsqr();
ASC_DLLSPEC double dsqr2();
ASC_DLLSPEC double cube();
ASC_DLLSPEC double dcube();
ASC_DLLSPEC double dcube2();
ASC_DLLSPEC double asc_ipow();
ASC_DLLSPEC double asc_d1ipow();
ASC_DLLSPEC double asc_d2ipow();
ASC_DLLSPEC double hold();
ASC_DLLSPEC double dsqrt();
ASC_DLLSPEC double dsqrt2();
ASC_DLLSPEC double dcbrt();
ASC_DLLSPEC double dcbrt2();
ASC_DLLSPEC double dfabs();
ASC_DLLSPEC double dfabs2();
ASC_DLLSPEC double dhold();
ASC_DLLSPEC double dasin();
ASC_DLLSPEC double dasin2();
ASC_DLLSPEC double dcos();
ASC_DLLSPEC double dcos2();
ASC_DLLSPEC double dacos();
ASC_DLLSPEC double dacos2();
ASC_DLLSPEC double dtan();
ASC_DLLSPEC double dtan2();
ASC_DLLSPEC double datan();
ASC_DLLSPEC double datan2();
ASC_DLLSPEC double derf();
ASC_DLLSPEC double derf2();

#  endif  /* no stdc */
# endif  /* fake ASC_FUNC_H */
#endif  /* ASC_BT_PROLOG */

