/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
*/
#ifndef PENGROB_H
#define PENGROB_H
#include "rundata.h"

PureFluid *pengrob_prepare(const EosData *data, const ReferenceState *ref);

/* these are the 'raw' functions, they don't do phase equilibrium. */
PropEvalFn pengrob_p;
PropEvalFn pengrob_u;
PropEvalFn pengrob_h;
PropEvalFn pengrob_s;
PropEvalFn pengrob_a;
PropEvalFn pengrob_g;
PropEvalFn pengrob_cp;
PropEvalFn pengrob_cv;
PropEvalFn pengrob_w;
PropEvalFn pengrob_dpdrho_T;
PropEvalFn pengrob_alphap;
PropEvalFn pengrob_betap;


#endif //PENGROB_H
