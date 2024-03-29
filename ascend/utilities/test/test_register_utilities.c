/*	ASCEND modelling environment
	Copyright (C) 2005 Jerry St.Clair

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
#include "test_register_utilities.h"

#ifdef ASC_WITH_DMALLOC
# error "Need to disable the ascMalloc test in this case"
#endif

#ifdef TEST_ASCPANIC
# error "Need to reenable the ascPanic test"
#endif

#define TESTS(T) \
	T(ascDynaLoad) \
	T(ascEnvVar) \
	/*T(ascPanic)*/ \
	T(error) \
	T(ascPrint) \
	T(ascSignal) \
	T(readln) \
	T(set) \
	T(bit)

#define PROTO_UTILS(NAME) PROTO(utilities,NAME)
TESTS(PROTO_UTILS)
#undef PROTO_UTILS

#define REGISTER_TEST(NAME) \
	result = TESTREGISTER(utilities,NAME); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(utilities,TESTS)

