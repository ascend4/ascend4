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
  struct BitList *bit1, *bit2, *bit3;
  unsigned long prior_meminuse = ascmeminuse();

#ifndef MALLOC_DEBUG
  CU_FAIL("test_set() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  // CreateBList, SetBit, ClearBit, ReadBit, BLength, BitListEmpty, BitListBytes

  bit1 = CreateBList(10);

  SetBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==1);
  ClearBit(bit1,0);
  CU_TEST(ReadBit(bit1,0)==0);

  CU_TEST(BitListBytes(bit1) == 2 + SIZEOF_ULONG);
  CU_TEST(BitListEmpty(bit1));
  CU_TEST(BLength(bit1)==10);

  DestroyBList(bit1);

  // CreateFBList, ExpandBList

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

  CU_ASSERT(ReadBit(bit1,9));
  ClearBit(bit1,9);
  CU_ASSERT(!ReadBit(bit1,9));
  SetBit(bit1,9);
  CU_ASSERT(ReadBit(bit1,9));

  CU_ASSERT(ReadBit(bit1,2));
  ClearBit(bit1,2);
  CU_ASSERT(!ReadBit(bit1,2));
  SetBit(bit1,2);
  CU_ASSERT(ReadBit(bit1,2));

  CU_ASSERT(ReadBit(bit1,7));
  ClearBit(bit1,7);
  CU_ASSERT(!ReadBit(bit1,7));
  SetBit(bit1,7);
  CU_ASSERT(ReadBit(bit1,7));

  CU_TEST(BLength(bit1)==100);
  bit1 = ExpandBList(bit1,150);
  CU_TEST(BLength(bit1)==150);

  DestroyBList(bit1);

  // BitListEmpty, CompBList

  bit1 = CreateBList(10);
  CU_TEST(BitListEmpty(bit1));
  SetBit(bit1,9);
  CU_TEST(!BitListEmpty(bit1));
  ClearBit(bit1,9);
  CU_TEST(BitListEmpty(bit1));
  SetBit(bit1,0);
  CU_TEST(!BitListEmpty(bit1));
  ClearBit(bit1,0);
  CU_TEST(BitListEmpty(bit1));
  DestroyBList(bit1);

  bit1 = CreateFBList(10);
  bit2 = CreateFBList(10);
  //SetBit(bit1,11); // naughty, past end of list, causes panic
  CU_TEST(CompBList(bit1,bit2));
  DestroyBList(bit1);

  // ExpandFBList

  bit1 = CreateFBList(5);
  bit1 = ExpandFBList(bit1,10);
  CU_TEST(CompBList(bit1,bit2));

  DestroyBList(bit1);
  DestroyBList(bit2);

  // ExpandBList

  bit1 = CreateBList(10);
  SetBit(bit1,3);
  SetBit(bit1,8);
  bit2 = CopyBList(bit1);
  CU_TEST(CompBList(bit1,bit2));
  DestroyBList(bit2);
  bit1 = ExpandBList(bit1,20);
  SetBit(bit1,14);
  CU_TEST(ReadBit(bit1,3));
  CU_TEST(ReadBit(bit1,8));
  CU_TEST(!ReadBit(bit1,2));
  CU_TEST(!ReadBit(bit1,9));
  bit2 = CreateBList(20);
  SetBit(bit2,3);
  SetBit(bit2,8);
  SetBit(bit2,14);
  CU_TEST(CompBList(bit1,bit2));

  DestroyBList(bit1);
  DestroyBList(bit2);
  
  // CondSetBit

  bit1 = CreateBList(1000);
  bit2 = CreateBList(1000);
  CondSetBit(bit1,768,84);
  SetBit(bit2,768);
  CU_TEST(CompBList(bit1,bit2));
  CondSetBit(bit1,21,5);
  CU_TEST(!CompBList(bit1,bit2));
  CondSetBit(bit1,21,0);
  CU_TEST(CompBList(bit1,bit2));
  
  DestroyBList(bit1);
  DestroyBList(bit2);

  // IntersectBLists

  bit1 = CreateBList(1000);
  bit2 = CreateBList(1000);
  SetBit(bit1,10);
  SetBit(bit2,999);
  SetBit(bit2,10);
  SetBit(bit1,22);
  IntersectBLists(bit1,bit2);
  CU_TEST(ReadBit(bit1,10));
  CU_TEST(!ReadBit(bit1,999));
  CU_TEST(!ReadBit(bit1,22));
  bit3 = CreateBList(1000);
  SetBit(bit3,10);
  CU_TEST(CompBList(bit1,bit3));
  DestroyBList(bit1);
  DestroyBList(bit2);
  DestroyBList(bit3);

  // UnionBLists

  bit1 = CreateBList(1000);
  bit2 = CreateBList(1000);
  SetBit(bit1,19);
  SetBit(bit2,888);
  SetBit(bit2,19);
  SetBit(bit1,105);
  UnionBLists(bit1,bit2);
  CU_TEST(ReadBit(bit1,19));
  CU_TEST(ReadBit(bit1,888));
  CU_TEST(ReadBit(bit1,105));
  bit3 = CreateBList(1000);
  SetBit(bit3,19);
  SetBit(bit3,888);
  SetBit(bit3,105);
  CU_TEST(CompBList(bit1,bit3));
  DestroyBList(bit1);
  DestroyBList(bit2);
  DestroyBList(bit3);

  // FirstNonZeroBit

  bit1 = CreateBList(19);
  SetBit(bit1,0);
  CU_TEST(FirstNonZeroBit(bit1)==0);
  ClearBit(bit1,0);
  SetBit(bit1,1);
  CU_TEST(FirstNonZeroBit(bit1)==1);
  ClearBit(bit1,1);
  SetBit(bit1,2);
  CU_TEST(FirstNonZeroBit(bit1)==2);
  ClearBit(bit1,2);
  SetBit(bit1,7);
  CU_TEST(FirstNonZeroBit(bit1)==7);
  ClearBit(bit1,7);
  SetBit(bit1,8);
  CU_TEST(FirstNonZeroBit(bit1)==8);
  ClearBit(bit1,8);
  SetBit(bit1,15);
  CU_TEST(FirstNonZeroBit(bit1)==15);
  ClearBit(bit1,15);
  SetBit(bit1,16);
  CU_TEST(FirstNonZeroBit(bit1)==16);
  ClearBit(bit1,16);
  SetBit(bit1,18);
  CU_TEST(FirstNonZeroBit(bit1)==18);
  SetBit(bit1,17);
  CU_TEST(FirstNonZeroBit(bit1)==17);
  SetBit(bit1,16);
  CU_TEST(FirstNonZeroBit(bit1)==16);
  SetBit(bit1,15);
  CU_TEST(FirstNonZeroBit(bit1)==15);
  SetBit(bit1,14);
  CU_TEST(FirstNonZeroBit(bit1)==14);
  SetBit(bit1,13);
  CU_TEST(FirstNonZeroBit(bit1)==13);
  DestroyBList(bit1);

  bit1 = CreateBList(128);
  SetBit(bit1,127);
  CU_TEST(FirstNonZeroBit(bit1)==127);
  SetBit(bit1,126);
  CU_TEST(FirstNonZeroBit(bit1)==126);
  SetBit(bit1,8);
  CU_TEST(FirstNonZeroBit(bit1)==8);
  SetBit(bit1,7);
  CU_TEST(FirstNonZeroBit(bit1)==7);
  SetBit(bit1,0);
  CU_TEST(FirstNonZeroBit(bit1)==0);
  CU_TEST(!BitListEmpty(bit1));
  bit2 = CreateBList(128);
  CU_TEST(BitListEmpty(bit2));
  CU_TEST(!CompBList(bit1,bit2));
  CU_TEST(FirstNonZeroBit(bit2) >= 128);
  OverwriteBList(bit2,bit1);
  CU_TEST(FirstNonZeroBit(bit1) >= 128);
  CU_TEST(BitListEmpty(bit1));
  DestroyBList(bit1);
  DestroyBList(bit2);

  CU_TEST(prior_meminuse == ascmeminuse());// back to original memory used
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(bit)

REGISTER_TESTS_SIMPLE(utilities_bit, TESTS)

