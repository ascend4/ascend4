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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Unit test functions for linear/linsolqr.c
*/
#include <string.h>
#include <CUnit/CUnit.h>

#include <utilities/ascConfig.h>
#include <linear/linsolqr.h>

#include <assertimpl.h>

/*
	Test the rank of matrix 

	[ 1 ]
*/
static void test_qr1x1(void){
	linsolqr_system_t L;
	mtx_matrix_t M;
	mtx_coord_t C;
	mtx_range_t R;
	mtx_region_t G;
	int r;

	/* build the matrix [[1,0],[1,1]] */
	M = mtx_create();
	mtx_set_order(M,1);
	mtx_set_value(M,mtx_coord(&C,0,0), 1.0);

	/* construct a 'whole matrix' region (can't use mtx_ENTIRE_MATRIX with linsolqr_set_region) */
	R.low = 0;
	R.high = mtx_order(M) - 1;
	G.row = R;
	G.col = R;

	mtx_write_region_mmio(stderr,M,&G);

	L = linsolqr_create_default();
	linsolqr_set_matrix(L,M);
	linsolqr_set_region(L,G);
	linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
	linsolqr_reorder(L, &G, linsolqr_rmethod(L));
	linsolqr_factor(L,linsolqr_fmethod(L));
	r = linsolqr_rank(L);

	CU_ASSERT(r==1);
}

/*
	Test the rank of matrix 

	[ 1 0
	  1 1 ]
*/
static void test_qr2x2(void){
	linsolqr_system_t L;
	mtx_matrix_t M;
	mtx_coord_t C;
	mtx_range_t R;
	mtx_region_t G;
	int r;

	/* build the matrix [[1,0],[1,1]] */
	M = mtx_create();
	mtx_set_order(M,2);
	mtx_set_value(M,mtx_coord(&C,0,0), 1.0);
	mtx_set_value(M,mtx_coord(&C,1,0), 1.0);
	mtx_set_value(M,mtx_coord(&C,1,1), 1.0);

	/* construct a 'whole matrix' region (can't use mtx_ENTIRE_MATRIX with linsolqr_set_region) */
	R.low = 0;
	R.high = mtx_order(M) - 1;
	G.row = R;
	G.col = R;

	mtx_write_region_mmio(stderr,M,&G);

	L = linsolqr_create_default();
	linsolqr_set_matrix(L,M);
	linsolqr_set_region(L,G);
	linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
	linsolqr_reorder(L, &G, linsolqr_rmethod(L));
	linsolqr_factor(L,linsolqr_fmethod(L));
	r = linsolqr_rank(L);

	CU_ASSERT(r==2);
}

/*
	Test the rank of matrix 

	[ 2 0 1
	  1 1 1 
	  0 1 1 ]
*/
static void test_qr3x3(void){
	linsolqr_system_t L;
	mtx_matrix_t M;
	mtx_coord_t C;
	mtx_range_t R;
	mtx_region_t G;
	int r;

	/* build the matrix [[1,0],[1,1]] */
	M = mtx_create();
	mtx_set_order(M,3);
	mtx_set_value(M,mtx_coord(&C,0,0), 2.0);
	mtx_set_value(M,mtx_coord(&C,1,1), 1.0);
	mtx_set_value(M,mtx_coord(&C,2,2), 1.0);
	mtx_set_value(M,mtx_coord(&C,0,2), 1.0);
	mtx_set_value(M,mtx_coord(&C,1,0), 1.0);
	mtx_set_value(M,mtx_coord(&C,1,2), 1.0);
	mtx_set_value(M,mtx_coord(&C,2,1), 1.0);

	/* construct a 'whole matrix' region (can't use mtx_ENTIRE_MATRIX with linsolqr_set_region) */
	R.low = 0;
	R.high = mtx_order(M) - 1;
	G.row = R;
	G.col = R;

	mtx_write_region_mmio(stderr,M,&G);

	L = linsolqr_create_default();
	linsolqr_set_matrix(L,M);
	linsolqr_set_region(L,G);
	linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
	linsolqr_reorder(L, &G, linsolqr_rmethod(L));
	linsolqr_factor(L,linsolqr_fmethod(L));
	r = linsolqr_rank(L);

	CU_ASSERT(r==3);
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo qr_test_list[] = {
	{"test_qr1x1", test_qr1x1},
	{"test_qr2x2", test_qr2x2},
	{"test_qr3x3", test_qr3x3},
	CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"test_linear_qr", NULL, NULL, qr_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_linear_qr2x2(void){
	return CU_register_suites(suites);
}
