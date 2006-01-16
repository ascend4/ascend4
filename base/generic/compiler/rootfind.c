/*
 *  SLV: Ascend Nonlinear Solver
 *  by Kirk Andre' Abbott
 *  Created: 10/06/95
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: rootfind.c,v $
 *  Date last modified: $Date: 1997/07/18 12:33:55 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
    Copyright (C) 2005 Carnegie-Mellon University
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/** @file
	This is the first pass implemenation of some rootfinding codes.
	@see page 360 of NR in C.

	@TODO FIXME Use NETLIB code instead of this to avoid copyright
	'problems' with the NR in C boys
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "compiler/extfunc.h"
#include "compiler/rootfind.h"

#define ITMAX 100
#define EPS 1.0e-08
#define ZBIGNUM 1.0e08

/**
	We have maintained a consistent calling protocol between
	the (possibly) different versions of the rootfinding code.
	In this version, m is not used; n is the index into to
	the x-vector, of the variable that we are solving for.
	Our residuals are always written to f[0].
*/
double zbrent(ExtEvalFunc *func,	/* the evaluation function */
	      double *lowbound,		/* low bound */
	      double *upbound,		/* up bound */
	      int *mode,		/* to pass to the eval func */
	      int *m,			/* the relation index */
	      int *n,			/* the variable index */
	      double *x,	/* the x vector -- needed by eval func */
	      double *u,	/* the u vector -- needed by eval func */
	      double *f,		/* vector of residuals */
	      double *g,		/* vector of gradients */
	      double *tolerance,	/* accuracy of solution */
	      int *status)		/* success or failure */
{
  int iter, result;
  double x1, x2;
  double a, b, c;
  double fa, fb;
  double d, e, min1, min2;
  double fc, p, q, r, s, tol1, xm;

  x1 = *lowbound;		/* initialization */
  x2 = *upbound;
  a = x1, b = x2, c = x2;

  x[*n] = a;
  result = (*func)(mode,m,n,x,u,f,g);
  fa = f[*m];
  if (result) {
    *status = -1;
    return ZBIGNUM;
  }

  x[*n] = b;
  result = (*func)(mode,m,n,x,u,f,g);
  fb = f[*m];
  if (result) {
    *status = -1;
    return ZBIGNUM;
  }

  if ((fa > 0.0 && fb > 0.0) || (fa < 0.0 && fb < 0.0)) {
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Compiler: zbrent: Root must be bracketed.");
    *status = -1;		/* cannot invert */
    return ZBIGNUM;
  }
  fc = fb;
  for (iter=1;iter<=ITMAX;iter++) {
    if ((fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0)) {
      c = a;		/* rename a, b, c and adjust bounding interval d. */
      fc = fa;
      e = d = b - a;
    }
    if (fabs(fc) < fabs(fb)) {
      a = b;
      b = c;
      c = a;
      fa = fb;
      fb = fc;
      fc = fa;
    }
    tol1 = 2.0*EPS*fabs(b) + 0.5 * (*tolerance); /* Convergence check */
    xm = 0.5*(c-b);
    if (fabs(xm) <= tol1 || fb == 0.0) {
      x[*n] = b;
      *status = 0;
      return b;
    }
    if (fabs(e) >= tol1 && fabs(fa) > fabs(fb)) {
      s = fb/fa;	/* Attempt inverse quadratic interpolation. */
      if (a == c) {
	p = 2.0*xm*s;
	q = 1.0 - s;
      }
      else{
	q = fa/fc;
	r = fb/fc;
	p = s*(2.0*xm*q*(q-r) - (b-a)*(r-1.0));
	q = (q-1.0)*(r-1.0)*(s-1.0);
      }
      if (p > 0.0) q = -q;	/* check whether in bounds */
      p = fabs(p);
      min1 = 3.0*xm*q - fabs(tol1*q);
      min2 = fabs(e*q);
      if (2.0*p < (min1 < min2 ? min1 : min2)) {
	e = d;			/* accept interpolation */
	d = p/q;
      }
      else{
	d = xm;
	e = d;		/* interpolation failed; use bisection */
      }
    }
    else{		/* bounds decreasing too slowly; use bisection */
      d = xm;
      e = d;
    }
    a = b;			/* move last best quess to a. */
    fa = fb;
    if (fabs(d) > tol1)		/* evaluate new trial root */
      b += d;
    else
      b += (xm > 0.0 ? fabs(tol1) : -fabs(tol1));

    x[*n] = b;
    result = (*func)(mode,m,n,x,u,f,g);
    fb = f[*m]	;
    if (result) {
      *status = -1;
      return ZBIGNUM;
    }
  }
  ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Compiler: zbrent: Maximum number of iterations exceeded.");
  *status = -1;		/* cannot invert */
  return ZBIGNUM;	/* NOTREACHED */
}
