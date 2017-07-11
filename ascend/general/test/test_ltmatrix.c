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

#include <CUnit/CUnit.h>
#include <ascend/general/platform.h>
#include <ascend/general/ltmatrix.h>

#include "test/common.h"

#ifndef MEMUSED
# define MEMUSED(N) CU_TEST(ascmeminuse()==(N))
#endif

static void ltmatrix_fill_pattern(ltmatrix* matrix, int pattern);
static int ltmatrix_check_pattern(ltmatrix* matrix, int pattern);

static void test_ltmatrix(void){
	double array[] = {0.1,-1.2,2.3,-3.4,4.5,-5.6,6.7,-7.8,8.9,-9.0};
	/** Performing CRUD Operations on LTMatrix library */
	ltmatrix *matrix_upper;
	ltmatrix *matrix_lower;
	ltmatrix *matrix_full;
	
	/** Upper Matrix Operations */
	/** Create Operations */
	matrix_upper = ltmatrix_create(LTMATRIX_UPPER,4);
	CU_ASSERT(matrix_upper->access_type == LTMATRIX_UPPER);
	CU_ASSERT(matrix_upper->dimension == 4);
	CU_ASSERT(matrix_upper->len == 10);
	CU_ASSERT(matrix_upper->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	ltmatrix_debug_print(matrix_upper); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	/** Update Operations */
	ltmatrix_init(matrix_upper,array);
	CU_ASSERT(ltmatrix_compare_array(matrix_upper,array)==1);
	ltmatrix_debug_print(matrix_upper);
	ltmatrix_clear(matrix_upper);
	ltmatrix_debug_print(matrix_upper);	

	/** Lower Matrix Operations */
	/** Create Operations */
	matrix_lower = ltmatrix_create(LTMATRIX_LOWER,4);
	CU_ASSERT(matrix_lower->access_type == LTMATRIX_LOWER);
	CU_ASSERT(matrix_lower->dimension == 4);
	CU_ASSERT(matrix_lower->len == 10);
	CU_ASSERT(matrix_lower->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	ltmatrix_debug_print(matrix_lower); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	/** Update Operations */
	ltmatrix_init(matrix_lower,array);
	CU_ASSERT(ltmatrix_compare_array(matrix_lower,array)==1);
	ltmatrix_debug_print(matrix_lower);
	ltmatrix_clear(matrix_lower);
	ltmatrix_debug_print(matrix_lower);

	/** Full Matrix Operations */
	/** Create Operations */
	matrix_full = ltmatrix_create(LTMATRIX_FULL,4);
	CU_ASSERT(matrix_full->access_type == LTMATRIX_FULL);
	CU_ASSERT(matrix_full->dimension == 4);
	CU_ASSERT(matrix_full->len == 16);
	CU_ASSERT(matrix_full->h!=NULL);
	/** Read Operations */
	/** Done with Range checks in the Access Method. */
	ltmatrix_debug_print(matrix_full); /** Success Prints Matrix of 5x5 with all elements 0.0 */
	

	/** Pattern all the Hessian Matrices */
	ltmatrix_fill_pattern(matrix_upper,0);
	CU_ASSERT(ltmatrix_check_pattern(matrix_upper,0)==1);

	ltmatrix_fill_pattern(matrix_lower,0);
	CU_ASSERT(ltmatrix_check_pattern(matrix_lower,0)==1);

	ltmatrix_fill_pattern(matrix_full,0);
	CU_ASSERT(ltmatrix_check_pattern(matrix_full,0)==1);

	/** Print for one last time the Matrices */

	ltmatrix_debug_print(matrix_lower); 
	ltmatrix_debug_print(matrix_upper); 
	ltmatrix_debug_print(matrix_full); 


	/** Delete Operations */
	ltmatrix_destroy(matrix_upper);
	ltmatrix_destroy(matrix_lower);
	ltmatrix_destroy(matrix_full);
}

/** 
	Fills patterns in the given matrix
	@param matrix is the matrix in which a given pattern is stored
	@param pattern is the type of pattern one wants to store.
			0 - matrix[row][col] = ((row+1)*(col+1)+1)
*/
static void ltmatrix_fill_pattern(ltmatrix* matrix, int pattern){
	ltmatrix_test_validity(matrix);
	unsigned long i, j;
	switch(pattern){
		case 0:
			for (i = 0; i < matrix->dimension; i++)
			{
				for (j = 0 ; j < matrix->dimension; j++){
					ltmatrix_set_element(matrix,i,j,((i+1)*(j+1)+1));
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
static int ltmatrix_check_pattern(ltmatrix* matrix, int pattern){
	ltmatrix_test_validity(matrix);
	unsigned long i, j;
	switch(pattern){
		case 0:
			for (i = 0; i < matrix->dimension; i++)
			{
				for (j = 0 ; j < matrix->dimension; j++){
					if(ltmatrix_get_element(matrix,i,j)!=((i+1)*(j+1)+1))
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

	//-------------------------

static void test_compare(void){
	ltmatrix *m1 = ltmatrix_create(LTMATRIX_UPPER,3);
	double *r = ltmatrix_get_row_pointer(m1,0);
	r[0] = 1.;
	r[1] = 2.;
	r[2] = 3.;
	ltmatrix_set_element(m1,1,1,4.);
	ltmatrix_set_element(m1,2,1,5.);
	ltmatrix_set_element(m1,2,2,6.);

	CU_TEST(ltmatrix_get_element(m1,0,0)==1.);
	CU_TEST(ltmatrix_get_element(m1,1,0)==2.);
	CU_TEST(ltmatrix_get_element(m1,0,1)==2.);
	CU_TEST(ltmatrix_get_element(m1,2,0)==3.);
	CU_TEST(ltmatrix_get_element(m1,0,2)==3.);
	CU_TEST(ltmatrix_get_element(m1,1,1)==4.);
	CU_TEST(ltmatrix_get_element(m1,1,2)==5.);
	CU_TEST(ltmatrix_get_element(m1,2,2)==6.);

	ltmatrix *m2 = ltmatrix_create(LTMATRIX_UPPER,3);
	ltmatrix_init(m2,(double[]){1,2,3,4,5,6});
	CU_TEST(ltmatrix_compare(m1,m2)==1);
	ltmatrix_debug_print(m1);
	ltmatrix_destroy(m1);
	ltmatrix_destroy(m2);
	MEMUSED(0);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
  T(ltmatrix) \
  T(compare)

REGISTER_TESTS_SIMPLE(general_ltmatrix, TESTS);

