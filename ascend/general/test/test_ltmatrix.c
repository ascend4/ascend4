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
*/

#include <CUnit/CUnit.h>
#include <ascend/general/platform.h>
#include <ascend/general/ltmatrix.h>

static void Hessian_Mtx_fill_pattern(hessian_mtx* matrix, int pattern);
static int Hessian_Mtx_check_pattern(hessian_mtx* matrix, int pattern);

static void test_ltmatrix(void){
	double array[] = {0.1,-1.2,2.3,-3.4,4.5,-5.6,6.7,-7.8,8.9,-9.0};
	/** Performing CRUD Operations on LTMatrix library */
	hessian_mtx *matrix_upper;
	hessian_mtx *matrix_lower;
	hessian_mtx *matrix_full;
	
	/** Upper Matrix Operations */
	/** Create Operations */
	matrix_upper = Hessian_Mtx_create(Upper,4);
	CU_ASSERT(matrix_upper->access_type == Upper);
	CU_ASSERT(matrix_upper->dimension == 4);
	CU_ASSERT(matrix_upper->len == 10);
	CU_ASSERT(matrix_upper->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	Hessian_Mtx_debug_print(matrix_upper); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	/** Update Operations */
	Hessian_Mtx_init(matrix_upper,array);
	CU_ASSERT(Hessian_Mtx_compare_array(matrix_upper,array)==1);
	Hessian_Mtx_debug_print(matrix_upper);
	Hessian_Mtx_clear(matrix_upper);
	Hessian_Mtx_debug_print(matrix_upper);	

	/** Lower Matrix Operations */
	/** Create Operations */
	matrix_lower = Hessian_Mtx_create(Lower,4);
	CU_ASSERT(matrix_lower->access_type == Lower);
	CU_ASSERT(matrix_lower->dimension == 4);
	CU_ASSERT(matrix_lower->len == 10);
	CU_ASSERT(matrix_lower->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	Hessian_Mtx_debug_print(matrix_lower); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	/** Update Operations */
	Hessian_Mtx_init(matrix_lower,array);
	CU_ASSERT(Hessian_Mtx_compare_array(matrix_lower,array)==1);
	Hessian_Mtx_debug_print(matrix_lower);
	Hessian_Mtx_clear(matrix_lower);
	Hessian_Mtx_debug_print(matrix_lower);

	/** Full Matrix Operations */
	/** Create Operations */
	matrix_full = Hessian_Mtx_create(Full,4);
	CU_ASSERT(matrix_full->access_type == Full);
	CU_ASSERT(matrix_full->dimension == 4);
	CU_ASSERT(matrix_full->len == 16);
	CU_ASSERT(matrix_full->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	Hessian_Mtx_debug_print(matrix_full); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	

	/** Pattern all the Hessian Matrices */
	Hessian_Mtx_fill_pattern(matrix_upper,0);
	CU_ASSERT(Hessian_Mtx_check_pattern(matrix_upper,0)==1);

	Hessian_Mtx_fill_pattern(matrix_lower,0);
	CU_ASSERT(Hessian_Mtx_check_pattern(matrix_lower,0)==1);

	Hessian_Mtx_fill_pattern(matrix_full,0);
	CU_ASSERT(Hessian_Mtx_check_pattern(matrix_full,0)==1);

	/** Print for one last time the Matrices */

	Hessian_Mtx_debug_print(matrix_lower); 
	Hessian_Mtx_debug_print(matrix_upper); 
	Hessian_Mtx_debug_print(matrix_full); 


	/** Delete Operations */
	Hessian_Mtx_destroy(matrix_upper);
	Hessian_Mtx_destroy(matrix_lower);
	Hessian_Mtx_destroy(matrix_full);
}

/** 
	Fills patterns in the given matrix
	@param matrix is the matrix in which a given pattern is stored
	@param pattern is the type of pattern one wants to store.
			0 - matrix[row][col] = ((row+1)*(col+1)+1)
*/
void Hessian_Mtx_fill_pattern(hessian_mtx* matrix, int pattern){
	Hessian_Mtx_test_validity(matrix);
	unsigned long i, j;
	switch(pattern){
		case 0:
			for (i = 0; i < matrix->dimension; i++)
			{
				for (j = 0 ; j < matrix->dimension; j++){
					Hessian_Mtx_set_element(matrix,i,j,((i+1)*(j+1)+1));
				}	
			}
			break;
	}
}

/** 
	Checks the pattern in the given matrix
	@param matrix is the matrix in which a given pattern is stored
	@param pattern is the type of pattern one wants to check.
			0 - matrix[row][col] = ((row+1)*(col+1)+1)
*/
int Hessian_Mtx_check_pattern(hessian_mtx* matrix, int pattern){
	Hessian_Mtx_test_validity(matrix);
	unsigned long i, j;
	switch(pattern){
		case 0:
			for (i = 0; i < matrix->dimension; i++)
			{
				for (j = 0 ; j < matrix->dimension; j++){
					if(Hessian_Mtx_get_element(matrix,i,j)!=((i+1)*(j+1)+1))
					{
						return 0;
					}
				}	
			}
			break;
		default:
				return 0;
	}
	return 1;
}
/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T,X) \
  T(ltmatrix)

/* you shouldn't need to change the following */

#define TESTDECL(TESTFN) {#TESTFN,test_##TESTFN}

#define X ,

static CU_TestInfo ltmatrix_test_list[] = {
	TESTS(TESTDECL,X)
	X CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"general_ltmatrix", NULL, NULL, ltmatrix_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_ltmatrix(void){
	return CU_register_suites(suites);
}
