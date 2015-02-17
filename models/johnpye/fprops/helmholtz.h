/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FPROPS_HELMHOLTZ_H
#define FPROPS_HELMHOLTZ_H

#define FPROPS_CHAR int

#include "rundata.h"
#include "ideal.h"

PureFluid *helmholtz_prepare(const EosData *data, const ReferenceState *ref);

void helmholtz_destroy(PureFluid *data);

#endif

