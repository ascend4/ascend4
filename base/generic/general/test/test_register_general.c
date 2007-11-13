/*
 *  Registration function for the ASCEND general component.
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include <utilities/ascConfig.h>
#include "test_register_general.h"

#define TESTS(T) \
	T(dstring) \
	T(hashpjw) \
	T(list) \
	T(listio) \
	T(pool) \
	T(pretty) \
	T(stack) \
	T(table) \
	T(tm_time) \
	T(ospath)
/* 	T(qsort1) */

#define PROTO_GENERAL(NAME) PROTO(general,NAME)
TESTS(PROTO_GENERAL)
#undef PROTO_GENERAL

#define REGISTER_TEST(NAME) \
	result = test_register_general_##NAME(); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(general,TESTS)

