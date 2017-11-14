/*	ASCEND modelling environment
	Copyright (C) 2017 John Pye

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
	Unit test functions for freestore.
*/
#include <ascend/compiler/freestore.h>
#include <test/common.h>

static void test_simple(void){

  union RelationTermUnion *term = NULL;
  union RelationTermUnion **list = NULL;
  int n_buffers = 1;
  int buffer_length = 5;
  int len = 10;
  int i;

  CU_ASSERT(NULL == FreeStore_GetFreeStore());
  FreeStore_SetFreeStore(FreeStore_Create(n_buffers,buffer_length));
  CU_ASSERT(NULL != FreeStore_GetFreeStore());

  list = ASC_NEW_ARRAY(union RelationTermUnion *,len);
  CU_ASSERT(NULL != list);
  for (i=0;i<len;i++) {
    term = FreeStore_GetMem();
    if(term){
      V_TERM(term)->varnum = i;
      V_TERM(term)->t = e_var;
      list[i] = term;
    }
  }
  CU_ASSERT(0 == FreeStore_FreeMem(list[4]));
  CU_ASSERT(0 == FreeStore_FreeMem(list[5]));
  CU_ASSERT(0 == FreeStore_FreeMem(list[6]));

  term = (union RelationTermUnion *)malloc(sizeof(union RelationTermUnion));
  V_TERM(term)->varnum = 6;
  CU_ASSERT(1 == FreeStore_FreeMem(term));
  FreeStore__Statistics(stdout,FreeStore_GetFreeStore());
  FreeStore__BlastMem(FreeStore_GetFreeStore());
  ASC_FREE(term);
  ASC_FREE(list);
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(simple)

REGISTER_TESTS_SIMPLE(compiler_freestore, TESTS)

