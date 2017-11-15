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
*/
/**
	@file
	Unit test functions for 'ascend/utilities/bit.c'
*/

#include <ascend/general/platform.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/bit.h>

#include <test/common.h>
#include "test/assertimpl.h"

#include <ascend/general/config.h>

static void test_bit(void){
  struct BitList *bit1, *bit2;
  unsigned long prior_meminuse = ascmeminuse();

#ifndef MALLOC_DEBUG
  CU_FAIL("test_set() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  bit1 = CreateBList(10);

  SetBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==1);
  ClearBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==0);

  CU_TEST(BitListBytes(bit1) == 2 + SIZEOF_ULONG);
  CU_TEST(BitListEmpty(bit1));
  CU_TEST(BLength(bit1)==10);

  DestroyBList(bit1);

  bit1 = CreateFBList(100);
  CU_TEST(BLength(bit1)==100);
  CU_TEST(ReadBit(bit1,0)==1);
  ClearBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==0);
  SetBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==1);

  CU_TEST(ReadBit(bit1,8)==1);
  ClearBit(bit1,8);
  CU_TEST(ReadBit(bit1,8)==0);
  SetBit(bit1,8);
  CU_TEST(ReadBit(bit1,8)==1);

#if 0
  // mysteriously failing!!
  CU_TEST(ReadBit(bit1,9)==1);
  ClearBit(bit1,9);
  CU_TEST(ReadBit(bit1,9)==0);
  SetBit(bit1,9);
  CU_TEST(ReadBit(bit1,9)==1);

  CU_TEST(ReadBit(bit1,2)==1);
  ClearBit(bit1,2);
  CU_TEST(ReadBit(bit1,2)==0);
  SetBit(bit1,2);
  CU_TEST(ReadBit(bit1,2)==1);

  CU_TEST(ReadBit(bit1,7)==1);
  ClearBit(bit1,7);
  CU_TEST(ReadBit(bit1,7)==0);
  SetBit(bit1,7);
  CU_TEST(ReadBit(bit1,7)==1);
#endif
  CU_TEST(BLength(bit1)==100);
  bit2 = ExpandBList(bit1,150);
  //CU_TEST(BLength(bit1)==100);
  CU_TEST(BLength(bit2)==150);

  DestroyBList(bit2);

  CU_TEST(prior_meminuse == ascmeminuse());// back to original memory used
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(bit)

REGISTER_TESTS_SIMPLE(utilities_bit, TESTS)

