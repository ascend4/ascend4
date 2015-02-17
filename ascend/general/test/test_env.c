/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	CUnit tests for the ospath.c module.
	by John Pye.
*/

#include <ascend/general/env.h>
#include <test/common.h>

/*--------------------------------
	some simple test routines...
*/

# define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG);fflush(stderr)

/* 
	return NULL for unfound env vars, else point to a string that must not be
	modified by the caller, and may later be changed by a later call to getenv.
*/
static char *my_getenv(const char *name){
	if(strcmp(name,"MYHOME")==0){
		return "/home/john";
	}else if(strcmp(name,"MYBIN")==0){
		return "/usr/local/bin";
	}
	return NULL;
}

void test_subst(void){
	char s1[]="$MYHOME/bitmaps";
	char *r;

	M(s1);

	r = env_subst(s1,my_getenv);
	M(r);

	CU_TEST(strcmp(r,"/home/john/bitmaps")==0);
	ASC_FREE(r);

	/* TODO add lots more tests in here... */

	/*assert(strcmp(r,"C:/msys/1.0/share/ascend/share")==0);*/
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(subst) \

REGISTER_TESTS_SIMPLE(general_env, TESTS);

