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
#include "test_register_integrator.h"

#define TESTS(T) \
	T(ida) \
	T(lsode)

#define PROTO_INTEG(NAME) PROTO(integrator,NAME)
TESTS(PROTO_INTEG)
#undef PROTO_INTEG

#define REGISTER_TEST(NAME) \
	result = TESTREGISTER(integrator,NAME); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(integrator,TESTS)

