/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
*/
/**
	@file
	API tests for linear/linsolqr.c
*/
#include <math.h>
#include <string.h>

#include <ascend/general/platform.h>
#include <ascend/linear/linsolqr.h>
#include <ascend/system/system.h>
#include <ascend/utilities/set.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void build_matrix_2x2(mtx_matrix_t *m, mtx_region_t *g){
	mtx_coord_t c;
	mtx_range_t r;

	*m = mtx_create();
	mtx_set_order(*m,2);

	/* [4 1; 2 3] */
	mtx_set_value(*m,mtx_coord(&c,0,0),4.0);
	mtx_set_value(*m,mtx_coord(&c,0,1),1.0);
	mtx_set_value(*m,mtx_coord(&c,1,0),2.0);
	mtx_set_value(*m,mtx_coord(&c,1,1),3.0);

	r.low = 0;
	r.high = 1;
	g->row = r;
	g->col = r;
}

static void test_method_maps(void){
	const char *s;

	s = linsolqr_fmethods();
	CU_ASSERT(NULL != s);
	CU_ASSERT(NULL != strstr(s,"Fastest-SPK1/MR-RANKI"));
	CU_ASSERT(NULL != strstr(s,"CPQR"));

	CU_ASSERT(0 == strcmp("SPK1",linsolqr_enum_to_rmethod(spk1)));
	CU_ASSERT(NULL != strstr(linsolqr_enum_to_rmethod(unknown_r),"unknown"));

	CU_ASSERT(ranki == linsolqr_fmethod_to_fclass(ranki_ba2));
	CU_ASSERT(s_qr == linsolqr_fmethod_to_fclass(plain_qr));
	CU_ASSERT(unknown_c == linsolqr_fmethod_to_fclass(cond_qr));

	CU_ASSERT(NULL != strstr(linsolqr_rmethod_description(spk1),"SPK1"));
	CU_ASSERT(NULL != strstr(linsolqr_fmethod_description(ranki_ba2),"fast"));

	system_free_reused_mem();
}

static void test_ranki_ba2_api_workflow(void){
	linsolqr_system_t l;
	mtx_matrix_t m;
	mtx_region_t g;
	mtx_sparse_t *ur;
	mtx_sparse_t *uc;
	mtx_sparse_t *dr;
	mtx_sparse_t *dc;
	real64 rhs[2];
	real64 rhst[2];
	real64 sol[2];
	real64 solt[2];
	real64 x0, x1;
	real64 tol;
	unsigned *rowset;
	unsigned *colset;

	tol = 1e-10;
	rhs[0] = 9.0; rhs[1] = 8.0;   /* A*x = rhs -> x = [1.9, 1.4] */
	rhst[0] = 6.0; rhst[1] = 7.0; /* A^T*x = rhst -> x = [0.4, 2.2] */
	sol[0] = sol[1] = 0.0;
	solt[0] = solt[1] = 0.0;

	build_matrix_2x2(&m,&g);

	l = linsolqr_create_default();
	CU_ASSERT(ranki_ba2 == linsolqr_fmethod(l));
	CU_ASSERT(spk1 == linsolqr_rmethod(l));

	linsolqr_set_pivot_zero(l,1e-13);
	linsolqr_set_pivot_tolerance(l,0.25);
	linsolqr_set_condition_tolerance(l,0.5);
	linsolqr_set_drop_tolerance(l,1e-14);

	linsolqr_set_matrix(l,m);
	linsolqr_set_region(l,g);
	CU_ASSERT(m == linsolqr_get_matrix(l));

	linsolqr_add_rhs(l,rhs,FALSE);
	linsolqr_add_rhs(l,rhst,TRUE);
	linsolqr_add_rhs(l,rhs,FALSE); /* duplicate ignored */
	CU_ASSERT(2 == linsolqr_number_of_rhs(l));
	CU_ASSERT(rhs == linsolqr_get_rhs(l,0));
	CU_ASSERT(rhst == linsolqr_get_rhs(l,1));

	CU_ASSERT(0 == linsolqr_prep(l,linsolqr_fmethod_to_fclass(linsolqr_fmethod(l))));
	CU_ASSERT(0 == linsolqr_reorder(l,&g,linsolqr_rmethod(l)));
	CU_ASSERT(0 == linsolqr_factor(l,linsolqr_fmethod(l)));
	CU_ASSERT(2 == linsolqr_rank(l));
	CU_ASSERT(linsolqr_smallest_pivot(l) > 0.0);
	CU_ASSERT(NULL != linsolqr_get_factors(l));

	CU_ASSERT(0 == linsolqr_solve(l,rhs));
	CU_ASSERT(0 == linsolqr_solve(l,rhs)); /* solved path */
	CU_ASSERT(0 == linsolqr_solve(l,rhst));

	x0 = linsolqr_var_value(l,rhs,0);
	x1 = linsolqr_var_value(l,rhs,1);
	CU_ASSERT(fabs(x0 - 1.9) < 1e-9);
	CU_ASSERT(fabs(x1 - 1.4) < 1e-9);

	x0 = linsolqr_var_value(l,rhst,0);
	x1 = linsolqr_var_value(l,rhst,1);
	CU_ASSERT(fabs(x0 - 0.4) < 1e-9);
	CU_ASSERT(fabs(x1 - 2.2) < 1e-9);

	CU_ASSERT(FALSE == linsolqr_copy_solution(l,rhs,sol));
	CU_ASSERT(FALSE == linsolqr_copy_solution(l,rhst,solt));
	CU_ASSERT(fabs(sol[0] - 1.9) < 1e-9);
	CU_ASSERT(fabs(sol[1] - 1.4) < 1e-9);
	CU_ASSERT(fabs(solt[0] - 0.4) < 1e-9);
	CU_ASSERT(fabs(solt[1] - 2.2) < 1e-9);

	rowset = set_create(2);
	colset = set_create(2);
	CU_ASSERT(NULL != rowset);
	CU_ASSERT(NULL != colset);
	if (rowset && colset) {
		set_null(rowset,2);
		set_null(colset,2);
		CU_ASSERT(1 == linsolqr_get_pivot_sets(l,rowset,colset));
		CU_ASSERT(TRUE == set_is_member(rowset,0));
		CU_ASSERT(TRUE == set_is_member(rowset,1));
		CU_ASSERT(TRUE == set_is_member(colset,0));
		CU_ASSERT(TRUE == set_is_member(colset,1));
	}
	if (rowset) set_destroy(rowset);
	if (colset) set_destroy(colset);

	ur = linsolqr_unpivoted_rows(l);
	uc = linsolqr_unpivoted_cols(l);
	CU_ASSERT(NULL == ur);
	CU_ASSERT(NULL == uc);
	CU_ASSERT(fabs(linsolqr_org_col_dependency(l,0,0) - 1.0) < tol);

	linsolqr_calc_row_dependencies(l);
	linsolqr_calc_col_dependencies(l);
	dr = linsolqr_row_dependence_coefs(l,0);
	dc = linsolqr_col_dependence_coefs(l,0);
	CU_ASSERT(NULL == dr);
	CU_ASSERT(NULL == dc);

	linsolqr_rhs_was_changed(l,rhs);
	CU_ASSERT(0 == linsolqr_solve(l,rhs));

	linsolqr_matrix_was_changed(l);
	CU_ASSERT(0 == linsolqr_reorder(l,&g,natural));
	CU_ASSERT(0 == linsolqr_factor(l,linsolqr_fmethod(l)));

	linsolqr_remove_rhs(l,rhs);
	linsolqr_remove_rhs(l,rhst);
	CU_ASSERT(0 == linsolqr_number_of_rhs(l));

	linsolqr_destroy(l);
	mtx_destroy(m);
	system_free_reused_mem();
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(method_maps) \
	T(ranki_ba2_api_workflow)

REGISTER_TESTS_SIMPLE(linear_linsolqr_api, TESTS)
