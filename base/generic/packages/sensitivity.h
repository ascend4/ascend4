/*===================================================================*\
 THIS HEADER IS CRAP AND SHOULD BE FIXED UP PROPERLY
 			Created: May 16, 1996
			Version: $Revision: 1.1 $
			Date last modified: $Date: 1996/05/17 16:56:32 $

This file is part of the Ascend Language Interpreter.

Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Abbott.

The Ascend Language Interpreter is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Ascend Language Interpreter is distributed in hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with
the program; if not, write to the Free Software Foundation, Inc., 675
Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
\*===================================================================*/

/** @file
 *  Sensitivity analysis routines.
 *  <pre>
 *  When #including sensitivity.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler/instance_enum.h"
 *         #include "compiler/compiler.h"
 *         #include "general/list.h"
 *         #include "compiler/extfunc.h"
 *  </pre>
 *  @todo Document sensitivity.h functions.
 *  @todo Do we really need 2 files called [Ss]ensitivity.[ch]?  Other one is in tcltk98.
 */

#ifndef __SENSITIVITY_H_SEEN__
#define __SENSITIVITY_H_SEEN__

extern int do_solve_eval(struct Slv_Interp *slv_interp,
                         struct Instance *i,
                         struct gl_list_t *arglist,
                         unsigned long whichvar);

extern int do_finite_diff_eval(struct Slv_Interp *slv_interp,
                               struct Instance *i,
                               struct gl_list_t *arglist,
                               unsigned long whichvar);

extern char sensitivity_help[];

extern int do_sensitivity_eval_all(struct Slv_Interp *slv_interp,
                                   struct Instance *i,
                                   struct gl_list_t *arglist);

extern int do_sensitivity_eval(struct Slv_Interp *slv_interp,
                               struct Instance *i,
                               struct gl_list_t *arglist);

#endif  /* __SENSITIVITY_H_SEEN__ */

