/*	ASCEND modelling environment
	Copyright (C) 2009 Mahesh Narayanamurthi

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
	Light-Weight Hessian Matrix module 

	Created by: Mahesh Narayanamurthi
	Creation Date: Aug 2009
*/

#include "ltmatrix.h"

#include "panic.h"
#include "ascMalloc.h"

#include <stdio.h>

/** Static functions */
static int ltmatrix_compare_high_level(ltmatrix *matrix_a, ltmatrix *matrix_b);
static int ltmatrix_compare_element_wise(ltmatrix *matrix_a, ltmatrix *matrix_b);
static int ltmatrix_compare_with_array(ltmatrix *matrix_a, double *array_b);

/**
	Creates a Hessian Matrix
	@param access_type can be one of LTMATRIX_LOWER, LTMATRIX_UPPER and LTMATRIX_FULL
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
ltmatrix* ltmatrix_create(ltmatrix_layout access_type,
								unsigned long dimension
								){
	ltmatrix* matrix;
	unsigned long i;
	unsigned long len;
	asc_assert(dimension > 0);
	if (access_type == LTMATRIX_FULL){
		len = ((dimension)*(dimension));
	}else{
		len = (dimension*(dimension+1))/2; 
	}
	matrix = ASC_NEW(ltmatrix);
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
	Destroy Hessian Matrix
	@param matrix is the matrix that is to be destroyed
	@return 0 when successful
*/
int ltmatrix_destroy(ltmatrix* matrix){
	ltmatrix_test_validity(matrix);
	ltmatrix_clear(matrix);
	if (matrix!=NULL){
		if (matrix->h!=NULL){ 
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

	@param row is the row of the matrix (starting at zero)
	@param col is the col of the matrix (starting at zero)
	@return the index within the array accessed by row,col
*/
unsigned long ltmatrix_access(ltmatrix* matrix, unsigned long row, unsigned long col){
	asc_assert(matrix!=NULL); 
	ASC_ASSERT_RANGE(row,0,matrix->dimension); 
	ASC_ASSERT_RANGE(col,0,matrix->dimension); 
	unsigned long index = 99999;
	unsigned long r; 
	unsigned long c; 
	unsigned long d; 
	unsigned long l; 
	switch(matrix->access_type){ 
	case LTMATRIX_UPPER: 
		r = MIN(row,col);
		c = MAX(row,col);
		d = matrix->dimension; 
		l = matrix->len; 
		index = (l-1) - (((((d-1)-r) * (d-r)) / 2) + ((d-1)-c)); 
		break; 
	case LTMATRIX_LOWER: 
		r = MAX(row,col); 
		c = MIN(row,col); 
		index = ((r)*(r+1))/2 + c; 
		break; 
	case LTMATRIX_FULL: 
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
int ltmatrix_clear(ltmatrix* matrix){
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

	@note Care is not taken to ensure len(new_vals) &lt; matrix-&gt;len 
	FIXME How to ensure that length of the double is not shorter than
          matrix->len
*/
int ltmatrix_init(ltmatrix* matrix,double* new_vals){
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
void ltmatrix_debug_print(FILE *fp, ltmatrix* matrix){
	ltmatrix_test_validity(matrix);
	unsigned long i, j;
	fprintf(fp,"\n");
	for (i=0; i<matrix->dimension ; i++){
		for (j=0; j<matrix->dimension; j++){
			fprintf(fp,"%10.4g\t",ltmatrix_get_element(matrix,i,j));
		}
		fprintf(fp,"\n");
	}
}


/**
	Test that the matrix is valid
	@param matrix is the matrix which is to be tested
*/
int ltmatrix_test_validity(ltmatrix *matrix){
	asc_assert(matrix!=NULL);
	asc_assert(matrix->h!=NULL);
	asc_assert(matrix->dimension > 0);
	if (matrix->access_type == LTMATRIX_FULL){
		asc_assert((matrix->len) == ((matrix->dimension)*(matrix->dimension)));
	}else{
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
int ltmatrix_compare(ltmatrix* matrix_a, ltmatrix* matrix_b){
	if (ltmatrix_compare_high_level(matrix_a,matrix_b)){
		return ltmatrix_compare_element_wise(matrix_a,matrix_b);
	}
	return 0;
}


/**
	Does a high level comparison of the matrices. Does not compare the elements
	@param matrix_a is the 1st matrix
	@param matrix_b is the 2nd matrix
	@return 0 when no match
*/
int ltmatrix_compare_high_level(ltmatrix *matrix_a,ltmatrix *matrix_b){
	ltmatrix_test_validity(matrix_a);
	ltmatrix_test_validity(matrix_b);
	if(matrix_a->access_type == LTMATRIX_FULL){
		if(matrix_b->access_type != LTMATRIX_FULL){
			return 0;	
		}
	}else if (matrix_b->access_type == LTMATRIX_FULL){
		if(matrix_a->access_type != LTMATRIX_FULL){
			return 0;
		}
	}
	if(matrix_a->dimension != matrix_b->dimension){
		return 0;
	}
	if(matrix_a->len != matrix_b->len){
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
int ltmatrix_compare_element_wise(ltmatrix *matrix_a,ltmatrix *matrix_b){
	unsigned i,j;
	for (i=0;i<matrix_a->dimension;i++){
		for (j=0;j<matrix_a->dimension;j++){
			if(ltmatrix_get_element(matrix_a,i,j)!=ltmatrix_get_element(matrix_b,i,j)){
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
int ltmatrix_compare_array(ltmatrix* matrix_a, double* array_b)
{
	ltmatrix_test_validity(matrix_a);
	asc_assert(array_b!=NULL);
	return ltmatrix_compare_with_array(matrix_a,array_b);
}

/**
	Does element wise comparison of matrix with array.
	@param matrix_a is the matrix
	@param array_b is the array 
	@return 0 when no match

	@Note care is not taken to ensure array length is lesser than
          matrix length
*/
int ltmatrix_compare_with_array(ltmatrix* matrix_a, double* array_b){
	unsigned long i;
	for (i=0;i<matrix_a->len;i++){
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
double* ltmatrix_get_row_pointer(ltmatrix* matrix, unsigned long row){
	ltmatrix_test_validity(matrix);
	ASC_ASSERT_RANGE(row,0,matrix->dimension);
	switch(matrix->access_type){
	case LTMATRIX_LOWER:
	case LTMATRIX_FULL:
		return ((matrix->h) + ltmatrix_access(matrix,row,0));
		break;
	case LTMATRIX_UPPER:
		return ((matrix->h) + ltmatrix_access(matrix,row,row));
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
	@param row is the row whos length is needed (1..len)
	@return the length of the row
*/
unsigned long ltmatrix_get_row_length(ltmatrix* matrix, unsigned long row){
	ltmatrix_test_validity(matrix);
	ASC_ASSERT_RANGE(row,0,matrix->dimension);
	switch(matrix->access_type){
	case LTMATRIX_LOWER:
		return (row+1);
		break;
	case LTMATRIX_UPPER:
		return (matrix->dimension - row);
		break;
	case LTMATRIX_FULL:
		return (matrix->dimension);
		break;
	}
	return 0;
}

