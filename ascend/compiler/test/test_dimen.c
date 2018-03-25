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
	// test setup and destruction of the global list

	gl_init_pool();
	gl_init();
	InitDimenList();

	DestroyDimenList();
	gl_destroy_pool();
}

#define FRAC_IS_ZERO(F) (0==Numerator(F) && 1==Denominator(F))
#define FRAC_IS_ZERO(F) (0==Numerator(F) && 1==Denominator(F))
#define FRAC_EQUALS(F,A,B) ((A)==Numerator(F) && (B)==Denominator(F))

static void test_test2(void){
	// these tests are done without using the global list

	dim_type D,E,F;

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

	// CmpDimen
	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	SetDimFraction(E,D_LENGTH,CreateFraction(1,1));
	CU_TEST(0==CmpDimen(&D,&E));
	SetWild(&D);
	CU_TEST(-1==CmpDimen(&D,&E));
	SetWild(&E);
	CU_TEST(0==CmpDimen(&D,&E));
	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	SetDimFraction(E,D_LENGTH,CreateFraction(1,1));
	CU_TEST(0==CmpDimen(&D,&E));
	SetWild(&E);
	CU_TEST(1==CmpDimen(&D,&E));

	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(D,D_MASS,CreateFraction(1,1));
	CU_TEST(1==CmpDimen(&D,&E));

	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(D,D_MASS,CreateFraction(1,1));
	SetDimFraction(E,D_MASS,CreateFraction(1,1));
	CU_TEST(0==CmpDimen(&D,&E));

	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(E,D_MASS,CreateFraction(1,1));
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	CU_TEST(-1==CmpDimen(&D,&E));

	ClearDimensions(&D);
	ClearDimensions(&E);
	SetDimFraction(D,D_MASS,CreateFraction(1,1));
	SetDimFraction(E,D_MASS,CreateFraction(1,1));
	SetDimFraction(E,D_LENGTH,CreateFraction(1,1));
	CU_TEST(-1==CmpDimen(&D,&E));

	// AddDimensions

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(3,1));
	SetDimFraction(D,D_MASS,CreateFraction(7,1));
	ClearDimensions(&E);
	SetDimFraction(E,D_LENGTH,CreateFraction(11,1));
	SetDimFraction(E,D_TEMPERATURE,CreateFraction(5,1));
	F = AddDimensions(&D,&E);
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_LENGTH),14,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_MASS),7,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_TEMPERATURE),5,1));

	SetWild(&E);
	F = AddDimensions(&D,&E);
	CU_TEST(1==IsWild(&F));

	// SubDimensions

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(3,1));
	SetDimFraction(D,D_MASS,CreateFraction(7,1));
	ClearDimensions(&E);
	SetDimFraction(E,D_LENGTH,CreateFraction(11,1));
	SetDimFraction(E,D_TEMPERATURE,CreateFraction(5,1));
	F = SubDimensions(&D,&E);
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_LENGTH),-8,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_MASS),7,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(F,D_TEMPERATURE),-5,1));

	SetWild(&E);
	F = AddDimensions(&D,&E);
	CU_TEST(1==IsWild(&F));
}

static void test_test3(void){
	// test global list and operations on dimensions
	gl_init_pool();
	gl_init();
	InitDimenList();
	int L0 = gl_length(g_dimen_list);

	CU_TEST(L0==gl_length(g_dimen_list));
	dim_type D,E;
	const dim_type *P;
	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	CU_TEST(!IsWild(&D));
	
	P = FindOrAddDimen(&D);
	CU_TEST(NULL!=P);
	CU_TEST(NULL!=CheckDimensionsMatch(P,&D));
	CU_TEST(!IsWild(P));
	CU_TEST(L0+1==gl_length(g_dimen_list));

	// SquareDimension
	P = SquareDimension(NULL,1);
	CU_TEST(NULL==P);

	ClearDimensions(&D);
	SetWild(&D);
	P = SquareDimension(&D,1);
	CU_TEST(1==IsWild(P));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	P = SquareDimension(&D,1);
	CU_TEST(NULL==P); // fractional dimension

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	SetDimFraction(D,D_TEMPERATURE,CreateFraction(2,1));
	P = SquareDimension(&D,1);
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(*P,D_MASS)));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),2,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_TEMPERATURE),4,1));

	// CubeDimension

	P = CubeDimension(NULL,1);
	CU_TEST(NULL==P);

	ClearDimensions(&D);
	SetWild(&D);
	P = CubeDimension(&D,1);
	CU_TEST(1==IsWild(P));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	P = CubeDimension(&D,1);
	CU_TEST(NULL==P); // fractional dimension

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	P = CubeDimension(&D,0);
	CU_TEST(NULL!=P); // fractional dimension

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	SetDimFraction(D,D_TEMPERATURE,CreateFraction(2,1));
	P = CubeDimension(&D,1);
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(*P,D_MASS)));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),3,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_TEMPERATURE),6,1));

	// PowDimension

	P = PowDimension(1,NULL,1);
	CU_TEST(NULL==P);

	ClearDimensions(&D);
	SetWild(&D);
	P = PowDimension(2,&D,1);
	CU_TEST(1==IsWild(P));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	P = PowDimension(2,&D,1);
	CU_TEST(NULL==P); // fractional dimension

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,2));
	P = PowDimension(2,&D,0);
	CU_TEST(NULL!=P); // fractional dimension
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),1,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_MASS),0,1));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(1,1));
	SetDimFraction(D,D_TEMPERATURE,CreateFraction(2,1));
	P = PowDimension(5,&D,1);
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(*P,D_MASS)));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),5,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_TEMPERATURE),10,1));

	// ThirdDimension

	P = ThirdDimension(NULL,1);
	CU_TEST(NULL==P);

	ClearDimensions(&D);
	SetWild(&D);
	P = ThirdDimension(&D,1);
	CU_TEST(1==IsWild(P));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(4,1));
	P = ThirdDimension(&D,1);
	CU_TEST(NULL==P); // non-cube dimension

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(4,5));
	P = ThirdDimension(&D,0);
	CU_TEST(NULL!=P); // fractional dimension
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),4,15));

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(3,1));
	SetDimFraction(D,D_TEMPERATURE,CreateFraction(6,1));
	P = ThirdDimension(&D,1);
	CU_TEST(FRAC_IS_ZERO(GetDimFraction(*P,D_MASS)));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),1,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_TEMPERATURE),2,1));

	// SumDimensions

	ClearDimensions(&D);
	SetDimFraction(D,D_LENGTH,CreateFraction(3,1));
	SetDimFraction(D,D_MASS,CreateFraction(7,1));
	ClearDimensions(&E);
	SetDimFraction(E,D_LENGTH,CreateFraction(11,1));
	SetDimFraction(E,D_TEMPERATURE,CreateFraction(5,1));
	P = SumDimensions(&D,&E,1);
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_LENGTH),14,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_MASS),7,1));
	CU_TEST(FRAC_EQUALS(GetDimFraction(*P,D_TEMPERATURE),5,1));

	// DimName

	CU_TEST(0==strcmp(DimName(D_MASS),"M"));
	CU_TEST(0==strcmp(DimName(D_QUANTITY),"Q"));
	CU_TEST(0==strcmp(DimName(D_LENGTH),"L"));
	CU_TEST(0==strcmp(DimName(D_TIME),"T"));
	CU_TEST(0==strcmp(DimName(D_TEMPERATURE),"TMP"));
	CU_TEST(0==strcmp(DimName(D_CURRENCY),"C"));
	CU_TEST(0==strcmp(DimName(D_ELECTRIC_CURRENT),"E"));
	CU_TEST(0==strcmp(DimName(D_LUMINOUS_INTENSITY),"LUM"));
	CU_TEST(0==strcmp(DimName(D_PLANE_ANGLE),"P"));
	CU_TEST(0==strcmp(DimName(D_SOLID_ANGLE),"S"));
	CU_TEST(NULL==DimName(-1));
	CU_TEST(NULL==DimName(NUM_DIMENS));

	DestroyDimenList();
	gl_destroy_pool();

}




/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2) \
	T(test3)

REGISTER_TESTS_SIMPLE(compiler_dimen, TESTS)

