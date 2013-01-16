/*	ASCEND modelling environment
	Copyright (C) 2010 Carnegie Mellon University

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

#include <ascend/general/platform.h>
#include "test_register_solver.h"

#define SUITE solver

#define TESTS(T) \
	T(slv_common) \
	T(slvreq) \
	T(ipopt) \
	T(conopt) \
	T(qrslv) \
	T(fprops) \
	T(lrslv)

#define PROTO_SOLVER(NAME) PROTO(solver,NAME)
TESTS(PROTO_SOLVER)
#undef PROTO_SOLVER

#define REGISTER_TEST(NAME) \
	result = TESTREGISTER(solver,NAME); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(solver,TESTS)

