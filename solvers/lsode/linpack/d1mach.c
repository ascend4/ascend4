/*********************************************************************\
                        d1mach: Ascend Replacement for d1mach.f
                        by Ben Allan
                        Created: September, 1994
                        Version: $Revision: 1.3 $
                        Date last modified: $Date: 1998/07/06 10:56:12 $

This file is part of the Ascend fortran subroutine collection.

Copyright (C) Benjamin Andrew Allan

The ascend fortran subroutine collection is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.
Most of the sources in the ascend fortran subroutine collection are public
domain and available from NETLIB. See newsgroup sci.math.numerical-analysis.
Sources from netlib are not restricted by the GNU license and are marked as
such.

The Ascend fortran subroutine collection is distributed in hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with
the program; if not, write to the Free Software Foundation, Inc., 675
Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
COPYING is found in ../compiler.
\*********************************************************************/

/* d1mach.c. Ben Allan
   C replacement for d1mach.f in terms of ANSI constants.
   The LINPACK d1mach.f is not such that f77 compilers pick
   the right set of constants automatically.

Observed equivalences:
F  D1MACH( 1) = B**(EMIN-1), THE SMALLEST POSITIVE MAGNITUDE.
C  DBL_MIN
F  D1MACH( 2) = B**EMAX*(1 - B**(-T)), THE LARGEST MAGNITUDE.
C  DBL_MAX
F  D1MACH( 3) = B**(-T), THE SMALLEST RELATIVE SPACING.
C  DBL_EPSILON/2
F  D1MACH( 4) = B**(1-T), THE LARGEST RELATIVE SPACING.
C  DBL_EPSILON
F  D1MACH( 5) = LOG10(B)
C  NONE
   B:      FLT_RADIX
   EMIN:   DBL_MIN_EXP
   EMAX:   DBL_MAX_EXP
   T:      DBL_MANT_DIG

   On alphas d1mach(3)=DBL_EPSILON/2 for some reason. Returning
   DBL_EPSILON may result in 1 bit of conservatism in some codes, but
   this is the price of portability.

*/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
 

/* Commentary and Apology:
 * We used to have a bunch of #ifdef's here trying to figure out which
 * platform we were on and then blessing our d1mach function with the
 * proper number of underbars (d1mach vs d1mach_) so that the linker
 * would not whine about missing symbols due to the insanity of
 * whether or not the f77 compiler puts an underbar on the symbols it
 * generates.  Of course, just to make life fun, it's not strictly
 * platform dependent, since some f77 compilers accept flags that turn
 * the underbars on or off.  Given this lunacy and the wasted time of
 * trying to tack down this bug every time it occurs, we've decided to
 * just duplicate the function and be finished with the underbar
 * madness.  We realize this sucks and we apologize for it, but at
 * this point,``Frankly, my dears, we don't give a damn.''
 */

double d1mach_(int *i) {
  switch (*i) {
  case 1:
    return DBL_MIN;
  case 2:
    return DBL_MAX;
  case 3:
    return DBL_EPSILON;
  case 4:
    return DBL_EPSILON;
  case 5:
    return log10((double)FLT_RADIX);
  default:
    fprintf(stderr," D1MACH - I OUT OF BOUNDS %d",*i);
    abort();
  }
}

double d1mach(int *i) {
  return d1mach_(i);
}

double D1MACH(int *i) {
  return d1mach_(i);
}



