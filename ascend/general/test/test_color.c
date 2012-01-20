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

#include <ascend/general/color.h>
#include <test/common.h>

void test_rainbow(void){

	int c[] = {0,ASC_FG_RED,ASC_FG_GREEN,ASC_FG_BROWN,ASC_FG_BLUE,ASC_FG_MAGENTA,ASC_FG_CYAN,ASC_FG_GREY
		,ASC_FG_DARKGREY,ASC_FG_BRIGHTRED,ASC_FG_BRIGHTGREEN,ASC_FG_YELLOW,ASC_FG_BRIGHTBLUE,ASC_FG_PINK,ASC_FG_BRIGHTCYAN,ASC_FG_WHITE};
	char *s[] = {"black","red","green","brown","blue","magenta","cyan","grey","darkgrey","red","brightgreen","yellow","brightblue","pink","brightcyan","white"};

	int i,n=16;
	FPRINTF(stdout,"\nCOLOR TEST\n");
	for(i=0;i<n;++i){
		color_on(stdout,c[i]);
		FPRINTF(stdout,"%s",s[i]);
		color_off(stdout);
		FPRINTF(stdout,"%c",i%8==7?'\n':' ');
	}
	CU_TEST(1);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(rainbow) \

REGISTER_TESTS_SIMPLE(general_color, TESTS);

