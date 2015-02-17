/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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

#include <ascend/general/platform.h>
#include "test_register_compiler.h"

#define SUITE compiler

#define TESTS(T) \
	T(basics) \
	T(autodiff) \
	T(expr) \
	T(bintok) \
	T(fixfree) \
	T(blackbox) \
	T(fixassign)


#define PROTO_TEST(NAME) PROTO(compiler,NAME)
TESTS(PROTO_TEST)
#undef PROTO_TEST

#define REGISTER_TEST(NAME) \
	result = TESTREGISTER(compiler,NAME); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(compiler,TESTS)
