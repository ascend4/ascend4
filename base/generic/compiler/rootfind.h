/**< 
 *  SLV: Ascend Nonlinear Solver
 *  by Kirk Andre' Abbott
 *  Created: 10/06/95
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: rootfind.h,v $
 *  Date last modified: $Date: 1997/07/18 12:33:57 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#ifndef rootfind_already_included
#define rootfind_already_included

/**< needed only for the definition of
 * ExtEvalFunc. This definition should probably
 * be moved to types.h. or compiler.h */
/**< requires
 *# #ifndef STAND_ALONE
 *# #include "extfunc.h"
 *# #else
 *# #include "codegen_support.h"
 *# #endif
 */


extern double zbrent(ExtEvalFunc *func,	/**< the evaluation function */
       double *lowbound,		/**< low bound */
       double *upbound,		/**< up bound */
       int *mode,		/**< to pass to the eval func */
       int *m,			/**< the relation index */
       int *n,			/**< the variable index */
       double *x,	/**< the x vector -- needed by eval func */
       double *u,	/**< the u vector -- needed by eval func */
       double *f,		/**< vector of residuals */
       double *g,		/**< vector of gradients */
       double *tolerance,	/**< accuracy of solution */
       int *status);		/**< success or failure */
/**< 
 *  Using Brents method, find the root of a function known to lie
 *  between x1 and x2. The root, returned as zbrent, will be refined
 *  until its accuracy is tol. The result of status must be monitored
 *  to see if we were successful. A nonzero code means that the result
 *  returned is erroneous. n is the index into the x vector of the variable
 *  that is to be solved for.
 */

#endif /**< rootfind_already_included */




