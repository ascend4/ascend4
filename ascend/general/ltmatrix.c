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

#include "ltmatrix.h"

#include <ascend/utilities/ascPanic.h>
#include <ascend/utilities/ascMalloc.h>

#include <stdio.h>

/** Static functions */
static int Hessian_Mtx_compare_high_level(hessian_mtx *matrix_a, hessian_mtx *matrix_b);
static int Hessian_Mtx_compare_element_wise(hessian_mtx *matrix_a, hessian_mtx *matrix_b);
static int Hessian_Mtx_compare_with_array(hessian_mtx *matrix_a, double *array_b);

/**
	Creates a Hessian Matrix
	@param access_type can be one of Lower, Upper and Full
	
	(using row-majorness)
	
	Lower Triangular would appear something like this for a dimension = 3 
	0 X X
	1 2 X
	3 4 5
	
	Upper Triangular would appear something like this for a dimension = 3
	0 1 2
	X 3 4
	X X 5

	The references 0,1,2,3,4,5 are the indices of the corresponding elements
	in the linear array.

	A full Matrix is arranged as
	0 1 2
	3 4 5
	6 7 8

	@param dimension is the dimension of the matrix
	@param len is the length of the linear array with nxn the maximum value
	@return matrix is the pointer to the new Matrix
*/
hessian_mtx* Hessian_Mtx_create(layout access_type,
								unsigned long dimension
								){
	hessian_mtx* matrix;
	unsigned long i;
	unsigned long len;
	asc_assert(dimension > 0);
	if (access_type == Full)
	{
		len = ((dimension)*(dimension));
	}
	else 
	{
		len = (dimension*(dimension+1))/2; 
	}
	matrix = (hessian_mtx*) ASC_NEW(hessian_mtx);
	matrix->h = ASC_NEW_ARRAY_CLEAR(double,len);
	asc_assert(matrix!=NULL);
	matrix->access_type = access_type;
	matrix->dimension = dimension;
	matrix->len = len;
	for(i=0;i<len;i++){
		matrix->h[i]=0.0;
	}
	return matrix;
}


/**
	Kills a Hessian Matrix
	@param matrix is the matrix that is to be destroyed
	@return 0 when successful
*/
int Hessian_Mtx_destroy(hessian_mtx* matrix){
	Hessian_Mtx_test_validity(matrix);
	Hessian_Mtx_clear(matrix);
	if (matrix!=NULL)
	{
		if (matrix->h!=NULL)
		{ 
			ascfree(matrix->h);
		}
		ascfree(matrix);
	}
	return 0;
}


/**
	Returns the index within a single-dimensional array calculated from row,col
	@param matrix is the matrix whose array-index is returned.
	
	(using row-majorness)
	
	Lower Triangular would appear something like this for a dimension = 3 
	0 X X
	1 2 X
	3 4 5
	
	Upper Triangular would appear something like this for a dimension = 3
	0 1 2
	X 3 4
	X X 5

	The references 0,1,2,3,4,5 are the indices of the corresponding elements
	in the linear array.

	A full Matrix is arranged as
	0 1 2
	3 4 5
	6 7 8

	@param row is the row of the matrix
	@param col is the col of the matrix
	@return the index within the array accessed by row,col
*/
unsigned long Hessian_Mtx_access(hessian_mtx* matrix, unsigned long row, unsigned long col){
	asc_assert(matrix!=NULL); 
	ASC_ASSERT_RANGE(row,0,matrix->dimension); 
	ASC_ASSERT_RANGE(col,0,matrix->dimension); 
	unsigned long index;
	unsigned long r; 
	unsigned long c; 
	unsigned long d; 
	unsigned long l; 
	switch(matrix->access_type){ 
		case Upper: 
			r = MIN(row,col);
			c = MAX(row,col);
			d = matrix->dimension; 
			l = matrix->len; 
			index = (l-1) - (((((d-1)-r) * (d-r)) / 2) + ((d-1)-c)); 
			break; 
		case Lower: 
			r = MAX(row,col); 
			c = MIN(row,col); 
			index = ((r)*(r+1))/2 + c; 
			break; 
		case Full: 
			index = (row * (matrix->dimension)) + col; 
			break; 
	} 
	return index;
}

/**
	Initializes the elements of the matrix with 0.0
	@param matrix is the matrix whose elements are to be set to 0.0
	@return 0 if successful
*/
int Hessian_Mtx_clear(hessian_mtx* matrix){
	unsigned long i;
	for(i=0;i<matrix->len;i++){
		matrix->h[i]=0.0;
	}
	return 0;
}

/**
	Initializes the matrix with the new set of values from new_vals
	@param matrix is the matrix whose elements are to be set from new_vals
	@param new_vals is the array from which the new values are read
	@return 0 if successful

	@note Care is not taken to ensure len(new_vals)&lt;matrix-&gt;len 
	FIXME How to ensure that length of the double is not shorter than
          matrix->len
*/
int Hessian_Mtx_init(hessian_mtx* matrix,double* new_vals){
	asc_assert(new_vals!=NULL);
	unsigned long i;
	for(i=0;i<matrix->len;i++){
		matrix->h[i]= new_vals[i];
	}
	return 0;
}

/**
	Print out the contents of the Matrix for debugging
	@param matrix is the matrix whose content is to be printed
	FIXME Do I need to make this static?
*/
void Hessian_Mtx_debug_print(hessian_mtx* matrix){
	Hessian_Mtx_test_validity(matrix);
	unsigned long i, j;
	printf("\n");
	for (i=0; i<matrix->dimension ; i++){
		for (j=0; j<matrix->dimension; j++){
			printf("%10.4g\t",Hessian_Mtx_get_element(matrix,i,j));
		}
		printf("\n");
	}
}


/**
	Test that the matrix is valid
	@param matrix is the matrix which is to be tested
*/
int Hessian_Mtx_test_validity(hessian_mtx *matrix){
	asc_assert(matrix!=NULL);
	asc_assert(matrix->h!=NULL);
	asc_assert(matrix->dimension > 0);
	if (matrix->access_type == Full)
	{
		asc_assert((matrix->len) == ((matrix->dimension)*(matrix->dimension)));
	}
	else 
	{
		asc_assert((matrix->len) == ((matrix->dimension)*((matrix->dimension)+1))/2); 
	}
	return 0; /* when successful */
}

/**
	Compares the two matrices
	@param matrix_a is the 1st matrix
	@param matrix_b is the 2nd matrix
	@return 0 when no match
*/
int Hessian_Mtx_compare(hessian_mtx* matrix_a, hessian_mtx* matrix_b){
	if (Hessian_Mtx_compare_high_level(matrix_a,matrix_b))
	{
		return Hessian_Mtx_compare_element_wise(matrix_a,matrix_b);
	}
	return 0;
}



/**
	Does a high level comparison of the matrices. Does not compare the elements
	@param matrix_a is the 1st matrix
	@param matrix_b is the 2nd matrix
	@return 0 when no match
*/
int Hessian_Mtx_compare_high_level(hessian_mtx *matrix_a,hessian_mtx *matrix_b){
	Hessian_Mtx_test_validity(matrix_a);
	Hessian_Mtx_test_validity(matrix_b);
	if (matrix_a->access_type == Full)
	{
		if(matrix_b->access_type != Full)
		{
			return 0;	
		}
	}
	else if (matrix_b->access_type == Full)
	{
		if(matrix_a->access_type != Full)
		{
			return 0;
		}
	}
	if(matrix_a->dimension != matrix_b->dimension)
	{
		return 0;
	}
	if(matrix_a->len != matrix_b->len)
	{
		return 0;
	}
	return 1;
}

/**
	Does a element wise comparison of the matrices. 
	@param matrix_a is the 1st matrix
	@param matrix_b is the 2nd matrix
	@return 0 when no match
*/
int Hessian_Mtx_compare_element_wise(hessian_mtx *matrix_a,hessian_mtx *matrix_b){
	unsigned i,j;
	for (i=0;i<matrix_a->dimension;i++)
	{
		for (j=0;j<matrix_a->dimension;j++)
		{
			if(Hessian_Mtx_get_element(matrix_a,i,j)!=Hessian_Mtx_get_element(matrix_b,i,j))
			{
				return 0;
			}
		}
	}
	return 1;
}

/**
	Compares the a matrix against an array.
	@param matrix_a is the matrix
	@param array_b is the array 
	@return 0 when no match
*/
int Hessian_Mtx_compare_array(hessian_mtx* matrix_a, double* array_b)
{
	Hessian_Mtx_test_validity(matrix_a);
	asc_assert(array_b!=NULL);
	return Hessian_Mtx_compare_with_array(matrix_a,array_b);
}

/**
	Does element wise comparison of matrix with array.
	@param matrix_a is the matrix
	@param array_b is the array 
	@return 0 when no match

	@Note care is not taken to ensure array length is lesser than
          matrix length
*/
int Hessian_Mtx_compare_with_array(hessian_mtx* matrix_a, double* array_b)
{
	unsigned long i;
	for (i=0;i<matrix_a->len;i++)
	{
		if(matrix_a->h[i]!=array_b[i]){
			return 0;
		}
	}	
	return 1;
}

/**
	Returns the Head pointer of a row of the matrix
	@param matrix is the matrix 
	@param row is the row whos head pointer is needed
	@return the pointer to the head of the row
*/
double* Hessian_Mtx_get_row_pointer(hessian_mtx* matrix, unsigned long row)
{
	Hessian_Mtx_test_validity(matrix);
	ASC_ASSERT_RANGE(row,0,matrix->dimension);
	switch(matrix->access_type)
	{
		case Lower:
		case Full:
					return ((matrix->h) + Hessian_Mtx_access(matrix,row,0));
					break;
		case Upper:
					return ((matrix->h) + Hessian_Mtx_access(matrix,row,row));
					break;
		default:
					return NULL;
					break;
	}
	return NULL;
}

/**
	Returns the length of a row of the matrix
	@param matrix is the matrix 
	@param row is the row whos length is needed
	@return the length of the row
*/
unsigned long Hessian_Mtx_get_row_length(hessian_mtx* matrix, unsigned long row)
{
	Hessian_Mtx_test_validity(matrix);
	ASC_ASSERT_RANGE(row,0,matrix->dimension);
	switch(matrix->access_type)
	{
		case Lower:
					return (row+1);
					break;
		case Upper:
					return (matrix->dimension - row);
					break;
		case Full:
					return (matrix->dimension);
					break;
	
	}
	return 0;
}
	
	

