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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

/**
	Light-Weight Hessian Matrix Library
	Created by: Mahesh Narayanamurthi
	Creation Date: Aug 2009
*/

#ifndef LT_MATRIX_H
#define LT_MATRIX_H

#include <ascend/utilities/ascPanic.h>
#include <ascend/utilities/ascMalloc.h>
#include <ascend/general/mathmacros.h>

/**	@addtogroup general_ltmatrix General Lightweight Matrix
	@{
*/

/**
	Hessian Matrix Layout-Structure
*/

enum hess_layout{
	Upper, /*Upper Triangular Matrix*/
	Lower, /*Lower Triangular Matrix*/
	Full   /*Full Hessian Matrix*/
};

typedef enum hess_layout layout;


/**
	Hessian Matrix Data-Structure
*/

struct rel_hessian_mtx {
	layout access_type; /* Details about access type  */
	unsigned long dimension;  /* Dimension of Square Matrix */
	unsigned long len; /* n*n in maximum case*/
	double *h;
};

typedef struct rel_hessian_mtx hessian_mtx;


#define Hessian_Mtx_set_element(matrix,row,col,value) \
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
		case Upper: \
			r = MIN(row,col); \
			c = MAX(row,col); \
			d = matrix->dimension; \
			l = matrix->len; \
			index = (l-1) - (((((d-1)-r) * (d-r)) / 2) + ((d-1)-c)); \
			asc_assert(index < l);\
			matrix->h[index] = value; \
			break; \
		case Lower: \
			r = MAX(row,col); \
			c = MIN(row,col); \
			index = ((r)*(r+1))/2 + c; \
			asc_assert(index < (matrix->len));\
			matrix->h[index] = value; \
			break; \
		case Full: \
			index = (row * (matrix->dimension)) + col; \
			asc_assert(index < (matrix->len));\
			matrix->h[index] = value; \
			break; \
	} \
} while (0)

#define HESSIAN_LT_SET_ELEMENT(matrix,row,col,val)  \
do { \
	asc_assert(matrix!=NULL); \
	asc_assert(matrix->access_type==Lower); \
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
	asc_assert(matrix->access_type==Upper); \
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
	asc_assert(matrix->access_type==Full); \
	ASC_ASSERT_RANGE(row,0,matrix->dimension); \
	ASC_ASSERT_RANGE(col,0,matrix->dimension); \
	unsigned long index;\
	unsigned long r; \
	unsigned long c; \
	index = (row * (matrix->dimension)) + col; \
	matrix->h[index] = value; \
} while (0)


#define Hessian_Mtx_get_element(matrix,row,col) matrix->h[Hessian_Mtx_access(matrix,row,col)]

ASC_DLLSPEC hessian_mtx* Hessian_Mtx_create(layout access_type,
											unsigned long dimension
											);

ASC_DLLSPEC int Hessian_Mtx_destroy(hessian_mtx* matrix);

ASC_DLLSPEC int Hessian_Mtx_compare(hessian_mtx* matrix_a, hessian_mtx* matrix_b);

ASC_DLLSPEC int Hessian_Mtx_compare_array(hessian_mtx* matrix_a, double* array_b);

ASC_DLLSPEC int Hessian_Mtx_clear(hessian_mtx* matrix);

ASC_DLLSPEC int Hessian_Mtx_init(hessian_mtx* matrix, double* new_vals);

ASC_DLLSPEC unsigned long Hessian_Mtx_access(hessian_mtx* matrix,
											unsigned long row,
											unsigned long col);

ASC_DLLSPEC double* Hessian_Mtx_get_row_pointer(hessian_mtx* matrix, unsigned long row);

ASC_DLLSPEC unsigned long Hessian_Mtx_get_row_length(hessian_mtx* matrix, unsigned long row);

ASC_DLLSPEC void Hessian_Mtx_debug_print(hessian_mtx* matrix);

ASC_DLLSPEC int Hessian_Mtx_test_validity(hessian_mtx *matrix);


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
