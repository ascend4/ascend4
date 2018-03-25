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
#include <ascend/compiler/fractions.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <test/common.h>

static void test_test1(void){

	struct fraction f,g,h;
	
	f = CreateFraction(1,2);
	CU_TEST(1==Numerator(f));
	CU_TEST(2==Denominator(f));	

	f = CreateFraction(2,6);
	CU_TEST(1==Numerator(f));
	CU_TEST(3==Denominator(f));	

	f = CreateFraction(15,-60);
	CU_TEST(-1==Numerator(f));
	CU_TEST(4==Denominator(f));	

	f = CreateFraction(-64,72);
	CU_TEST(-8==Numerator(f));
	CU_TEST(9==Denominator(f));	

	f = CreateFraction(1,0);
	CU_TEST(1==Numerator(f));
	CU_TEST(0==Denominator(f));	

	f = CreateFraction(2,0); // (2/0) is 'factored' to (1/0).
	CU_TEST(1==Numerator(f));
	CU_TEST(0==Denominator(f));	

	f = CreateFraction(0,1);
	CU_TEST(0==Numerator(f));
	CU_TEST(1==Denominator(f));	

	f = CreateFraction(0,2); // (0/2) is 'factored' to (0/1).
	CU_TEST(0==Numerator(f));
	CU_TEST(1==Denominator(f));	

	// adding

	f = CreateFraction(1,5);
	g = CreateFraction(2,5);
	h = AddF(f,g);
	CU_TEST(3==Numerator(h));
	CU_TEST(5==Denominator(h));	

	f = CreateFraction(1,10);
	g = CreateFraction(1,5);
	h = AddF(f,g);
	CU_TEST(3==Numerator(h));
	CU_TEST(10==Denominator(h));	

	f = CreateFraction(1,10);
	g = CreateFraction(2,5);
	h = AddF(f,g);
	CU_TEST(1==Numerator(h));
	CU_TEST(2==Denominator(h));	

	f = CreateFraction(3,10);
	g = CreateFraction(4,5);
	h = AddF(f,g);
	CU_TEST(11==Numerator(h));
	CU_TEST(10==Denominator(h));	

	//subtract

	f = CreateFraction(1,5);
	g = CreateFraction(2,5);
	h = SubF(f,g);
	CU_TEST(-1==Numerator(h));
	CU_TEST(5==Denominator(h));	

	f = CreateFraction(7,10);
	g = CreateFraction(1,5);
	h = SubF(f,g);
	CU_TEST(1==Numerator(h));
	CU_TEST(2==Denominator(h));	

	f = CreateFraction(11,10);
	g = CreateFraction(2,5);
	h = SubF(f,g);
	CU_TEST(7==Numerator(h));
	CU_TEST(10==Denominator(h));	

	f = CreateFraction(11,12);
	g = CreateFraction(3,4);
	h = SubF(f,g);
	CU_TEST(1==Numerator(h));
	CU_TEST(6==Denominator(h));	

	// multiply

	f = CreateFraction(1,3);
	g = CreateFraction(1,4);
	h = MultF(f,g);
	CU_TEST(1==Numerator(h));
	CU_TEST(12==Denominator(h));	

	f = CreateFraction(1,5);
	g = CreateFraction(15,-3);
	h = MultF(f,g);
	CU_TEST(-1==Numerator(h));
	CU_TEST(1==Denominator(h));	

	f = CreateFraction(1,0);
	g = CreateFraction(0,1);
	h = MultF(f,g);
	CU_TEST(0==Numerator(h));
	CU_TEST(0==Denominator(h));	

	// divide

	f = CreateFraction(1,3);
	g = CreateFraction(4,1);
	h = DivF(f,g);
	CU_TEST(1==Numerator(h));
	CU_TEST(12==Denominator(h));	

	f = CreateFraction(1,3);
	g = CreateFraction(1,2);
	h = DivF(f,g);
	CU_TEST(2==Numerator(h));
	CU_TEST(3==Denominator(h));	
	

}



/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_fractions, TESTS)

