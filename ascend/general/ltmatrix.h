/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Light-Weight Hessian Matrix Library
	
	This module implements dense symmetric and full matrices. Symmetric
	matrices can be stored internally as either lower-triangular or 
	upper-triangular. Module includes routines to access values from a particular
	row of the matrix, but no additional routines are included. These routines
	so far are only used in relman.c (relman_hess) and solvers/ipopt.

	Created by: Mahesh Narayanamurthi
	Creation Date: Aug 2009
*/

#ifndef LT_MATRIX_H
#define LT_MATRIX_H

#include "panic.h"
#include "ascMalloc.h"
#include <ascend/general/mathmacros.h>

/**	@addtogroup general_ltmatrix Dense symmetric matrices
	@{
*/

/**
	Hessian Matrix Layout-Structure
*/

enum ltmatrix_layout_enum{
	LTMATRIX_UPPER, /*Upper Triangular Matrix*/
	LTMATRIX_LOWER, /*Lower Triangular Matrix*/
	LTMATRIX_FULL   /*Full Hessian Matrix*/
};

typedef enum ltmatrix_layout_enum ltmatrix_layout;


/**
	Hessian Matrix Data-Structure
*/

struct ltmatrix_struct {
	ltmatrix_layout access_type; /* Details about access type  */
	unsigned long dimension;  /* Dimension of Square Matrix */
	unsigned long len; /* n*n in maximum case*/
	double *h;
};

typedef struct ltmatrix_struct ltmatrix;


#define ltmatrix_set_element(matrix,row,col,value) \
do { \
	asc_assert(matrix!=NULL); \
	ASC_ASSERT_RANGE(row,0,matrix->dimension); \
	ASC_ASSERT_RANGE(col,0,matrix->dimension); \
	unsigned long index;\
	unsigned long r; \
	unsigned long c; \
	unsigned long d; \
	unsigned long l; \
	switch(matrix->access_type){ \
		case LTMATRIX_UPPER: \
			r = MIN(row,col); \
			c = MAX(row,col); \
			d = matrix->dimension; \
			l = matrix->len; \
			index = (l-1) - (((((d-1)-r) * (d-r)) / 2) + ((d-1)-c)); \
			asc_assert(index < l);\
			matrix->h[index] = value; \
			break; \
		case LTMATRIX_LOWER: \
			r = MAX(row,col); \
			c = MIN(row,col); \
			index = ((r)*(r+1))/2 + c; \
			asc_assert(index < (matrix->len));\
			matrix->h[index] = value; \
			break; \
		case LTMATRIX_FULL: \
			index = (row * (matrix->dimension)) + col; \
			asc_assert(index < (matrix->len));\
			matrix->h[index] = value; \
			break; \
	} \
} while (0)

#define HESSIAN_LT_SET_ELEMENT(matrix,row,col,val)  \
do { \
	asc_assert(matrix!=NULL); \
	asc_assert(matrix->access_type==LTMATRIX_LOWER); \
	ASC_ASSERT_RANGE(row,0,matrix->dimension); \
	ASC_ASSERT_RANGE(col,0,matrix->dimension); \
	unsigned long index;\
	unsigned long r; \
	unsigned long c; \
	r = MAX(row,col); \
	c = MIN(row,col); \
	index = ((r)*(r+1))/2 + c; \
	matrix->h[index] = value; \
} while (0)

#define HESSIAN_UT_SET_ELEMENT(matrix,row,col,val) \
do { \
	asc_assert(matrix!=NULL); \
	asc_assert(matrix->access_type==LTMATRIX_UPPER); \
	ASC_ASSERT_RANGE(row,0,matrix->dimension); \
	ASC_ASSERT_RANGE(col,0,matrix->dimension); \
	unsigned long index;\
	unsigned long r; \
	unsigned long c; \
	unsigned long d; \
	unsigned long l; \
	r = MIN(row,col); \
	c = MAX(row,col); \
	d = matrix->dimension; \
	l = matrix->len; \
	index = (l-1) - (((((d-1)-r) * (d-r)) / 2) + ((d-1)-c)); \
	matrix->h[index] = value; \
} while (0)


#define HESSIAN_FULL_SET_ELEMENT(matrix,row,col,val) \
do { \
	asc_assert(matrix!=NULL); \
	asc_assert(matrix->access_type==LTMATRIX_FULL); \
	ASC_ASSERT_RANGE(row,0,matrix->dimension); \
	ASC_ASSERT_RANGE(col,0,matrix->dimension); \
	unsigned long index;\
	unsigned long r; \
	unsigned long c; \
	index = (row * (matrix->dimension)) + col; \
	matrix->h[index] = value; \
} while (0)


#define ltmatrix_get_element(matrix,row,col) matrix->h[ltmatrix_access(matrix,row,col)]

ASC_DLLSPEC ltmatrix* ltmatrix_create(ltmatrix_layout access_type,
											unsigned long dimension
											);

ASC_DLLSPEC int ltmatrix_destroy(ltmatrix* matrix);

ASC_DLLSPEC int ltmatrix_compare(ltmatrix* matrix_a, ltmatrix* matrix_b);

ASC_DLLSPEC int ltmatrix_compare_array(ltmatrix* matrix_a, double* array_b);

ASC_DLLSPEC int ltmatrix_clear(ltmatrix* matrix);

ASC_DLLSPEC int ltmatrix_init(ltmatrix* matrix, double* new_vals);

ASC_DLLSPEC unsigned long ltmatrix_access(ltmatrix* matrix,
											unsigned long row,
											unsigned long col);

ASC_DLLSPEC double* ltmatrix_get_row_pointer(ltmatrix* matrix, unsigned long row);

ASC_DLLSPEC unsigned long ltmatrix_get_row_length(ltmatrix* matrix, unsigned long row);

ASC_DLLSPEC void ltmatrix_debug_print(ltmatrix* matrix);

ASC_DLLSPEC int ltmatrix_test_validity(ltmatrix *matrix);


/** TODO
	1) Modify print routines to print only the upper or
	   lower half of a UT or LT matrix respectively
	2) Include a method to construct a Full matrix with
		a lower and a upper matrix
	When the hessian is not symmetric, can this be used
	to speed up Hessian evalutations?
*/

/* @} */

#endif
