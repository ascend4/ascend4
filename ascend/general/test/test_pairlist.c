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
	Test functions for general/pairlist.c
	by John Pye.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ascend/general/pairlist.h>
#include <ascend/general/list.h>

#include "test/common.h"
#include "test/assertimpl.h"

#ifndef MEMUSED
# define MEMUSED(N) CU_TEST(ascmeminuse()==(N))
#endif
#ifndef LISTUSESPOOL
# error "LISTUSESPOOL must be defined!"
#endif

static void setup(void){
#if LISTUSESPOOL
	gl_init_pool();
#endif
	gl_init();
}

static void teardown(void){
#if LISTUSESPOOL
	gl_destroy_pool();
#endif
}	

static void test_createdestroy(void){
	setup();
	struct pairlist_t *pl1 = pairlist_create(10);
	pairlist_destroy(pl1);
	teardown();
	MEMUSED(0);
}
	//------------------------

static void test_setappend(void){
	setup();
	struct pairlist_t *pl1 = pairlist_create(10);
	pairlist_append(pl1,"A","value1");
	pairlist_append(pl1,"B","value2");
	pairlist_append(pl1,"C","value3");
	int l = pairlist_length(pl1);
	CU_TEST(l==3);
	CU_TEST(3==pairlist_append_unique(pl1,"C","value4"));
	void *old;
	old = pairlist_set(pl1,"C","value5");
	CU_TEST(strcmp(old,"value3")==0);
	old = pairlist_set(pl1,"D","value6");
	CU_TEST(old==NULL);
	unsigned long i;
	i = pairlist_contains(pl1,"B");
	CU_TEST(i==2);
	CU_TEST(strcmp(pairlist_keyAt(pl1,i),"B")==0);
	CU_TEST(strcmp(pairlist_valueAt(pl1,i),"value2")==0);
	CU_TEST(strcmp(pairlist_valueAt(pl1,pairlist_contains(pl1,"C")),"value5")==0);
	pairlist_destroy(pl1);
	teardown();
	MEMUSED(0);
}
	//------------------------

/*===========================================================================*/
/* Registration information */


#define TESTS(T) \
	T(createdestroy) \
	T(setappend)

REGISTER_TESTS_SIMPLE(general_pairlist, TESTS);

