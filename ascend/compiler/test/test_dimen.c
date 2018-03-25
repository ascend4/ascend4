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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Unit test functions for compiler.
*/
#include <ascend/compiler/dimen.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <test/common.h>

static void test_test1(void){
	gl_init_pool();
	gl_init();
	InitDimenList();

	DestroyDimenList();
	gl_destroy_pool();
}

#define FRAC_IS_ZERO(F) (0==Numerator(F) && 1==Denominator(F))
#define FRAC_IS_ZERO(F) (0==Numerator(F) && 1==Denominator(F))

static void test_test2(void){

	dim_type D;

	// ClearDimensions

	ClearDimensions(&D);
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_MASS)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_QUANTITY)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_LENGTH)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_TIME)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_TEMPERATURE)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_CURRENCY)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_ELECTRIC_CURRENT)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_LUMINOUS_INTENSITY)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_PLANE_ANGLE)));
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(D,D_SOLID_ANGLE)));
	CU_TEST(!IsWild(&D));

	// IsWild

	SetWild(&D);
	CU_TEST(IsWild(&D));

	ClearDimensions(&D);
	CU_TEST(0==IsWild(&D));

	CU_TEST(1==IsWild(NULL));

	// SetDimFraction, IsOddDimension

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(2,1));
	SetDimFraction(D,D_LENGTH,CreateFraction(4,1));
	CU_TEST(0==OddDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(1,1));
	CU_TEST(1==OddDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(1,2));
	CU_TEST(1==OddDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(3,1));
	CU_TEST(1==OddDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(3,1));
	SetDimFraction(D,D_LENGTH,CreateFraction(4,1));
	CU_TEST(1==OddDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(-2,1));
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	CU_TEST(1==OddDimension(&D))

	// NonCubicDimension

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(3,1));
	SetDimFraction(D,D_LENGTH,CreateFraction(6,1));
	CU_TEST(0==NonCubicDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_TIME,CreateFraction(1,1));
	CU_TEST(1==NonCubicDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_SOLID_ANGLE,CreateFraction(4,1));
	CU_TEST(1==NonCubicDimension(&D))

	ClearDimensions(&D);
	SetDimFraction(D,D_SOLID_ANGLE,CreateFraction(3,1));
	SetWild(&D);
	CU_TEST(1==NonCubicDimension(&D))


}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2)

REGISTER_TESTS_SIMPLE(compiler_dimen, TESTS)

