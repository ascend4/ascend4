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
#include <linear/mtx_csparse.h>

#include <assertimpl.h>


/*
	Test the rank of matrix 

	[ 1 0 2
	  2 3 4 
	  0 6 7 ]
*/
static void test_csparse(void){
#ifdef ASC_WITH_UFSPARSE
	mtx_matrix_t M, M2;
	mtx_coord_t C;
	mtx_range_t R;
	mtx_region_t G;
	cs *csm;

	/* build the matrix [[1,0],[1,1]] */
	M = mtx_create();
	mtx_set_order(M,3);
	mtx_set_value(M,mtx_coord(&C,0,0), 1.0);
	mtx_set_value(M,mtx_coord(&C,1,1), 3.0);
	mtx_set_value(M,mtx_coord(&C,2,2), 7.0);
	mtx_set_value(M,mtx_coord(&C,0,2), 2.0);
	mtx_set_value(M,mtx_coord(&C,1,0), 2.0);
	mtx_set_value(M,mtx_coord(&C,1,2), 4.0);
	mtx_set_value(M,mtx_coord(&C,2,1), 6.0);

	/* construct a 'whole matrix' region (can't use mtx_ENTIRE_MATRIX with linsolqr_set_region) */
	R.low = 0;
	R.high = mtx_order(M) - 1;
	G.row = R;
	G.col = R;

	mtx_write_region_mmio(stderr,M,mtx_ENTIRE_MATRIX);

	csm = mtx_to_cs(M);
	cs_print(csm,1);

	CU_ASSERT(csm->nz==7);
	CU_ASSERT(csm->m==3);
	CU_ASSERT(csm->n==3);

	M2 = mtx_from_cs(csm);
	mtx_write_region_mmio(stderr,M2,mtx_ENTIRE_MATRIX);

	/* @TODO compare these outputs with expected values */

#else
	CONSOLE_DEBUG("CSparse not present -- skipping test");
#endif
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo mtx_test_list[] = {
	{"csparse", test_csparse},
	CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"linear_mtx", NULL, NULL, mtx_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_linear_mtx(void){
	return CU_register_suites(suites);
}

