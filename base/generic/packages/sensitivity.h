
/*********************************************************************\
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
\*********************************************************************/

 extern int do_solve_eval(struct Slv_Interp *,
		  struct Instance *,
		  struct gl_list_t *,
		  unsigned long);
     
extern int do_finite_diff_eval(struct Slv_Interp *,
			 struct Instance *,
			 struct gl_list_t *,
			 unsigned long);
     
extern char sensitivity_help[];

extern int do_sensitivity_eval_all(struct Slv_Interp *,
			    struct Instance *,
			    struct gl_list_t *);
     
extern int do_sensitivity_eval(struct Slv_Interp *,
			 struct Instance *,
			 struct gl_list_t *);
