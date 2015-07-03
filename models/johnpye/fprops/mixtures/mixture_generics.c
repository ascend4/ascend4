/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, June 25-, 2015

	Generic functions that are used in modeling mixtures, but that do not have 
	anything specifically to do with mixtures.
 */

#include "mixture_generics.h"
#include "mixture_struct.h"

double min_element(unsigned nelems, double *nums){
	unsigned i;
	double min=nums[0];
	for(i=1;i<nelems;i++){
		if(nums[i]<min){
			min = nums[i];
		}
	}
	return min;
}

double max_element(unsigned nelems, double *nums){
	unsigned i;
	double max=nums[0];
	for(i=1;i<nelems;i++){
		if(nums[i]>max){
			max = nums[i];
		}
	}
	return max;
}

double sum_elements(unsigned nelems, double *nums){
	unsigned i;
	double sum=0.0;
	for(i=0;i<nelems;i++){
		sum += nums[i];
	}
	return sum;
}

/*
	Find index of the minimum value in the array `nums', with maximum index 
	`nelems'
 */
unsigned index_of_min(unsigned nelems, double *nums){
	unsigned i;
	unsigned min_ix=0;  /* the index of the minimum element */
	double min=nums[0]; /* the minimum element */

	for(i=1;i<nelems;i++){
		if(nums[i]<min){
			min_ix = i;    /* update both `min' and `min_ix' */
			min = nums[i];
		}
	}
	return min_ix;
}

/*
	Find index of the maximum value in the array `nums', with maximum index 
	`nelems'
 */
unsigned index_of_max(unsigned nelems, double *nums){
	unsigned i;
	unsigned max_ix=0;  /* the index of the minimum element */
	double max=nums[0]; /* the minimum element */

	for(i=1;i<nelems;i++){
		if(nums[i]>max){
			max_ix = i;    /* update both `min' and `min_ix' */
			max = nums[i];
		}
	}
	return max_ix;
}

/*
	Generic root-finding function that uses the secant method, starting from 
	the positions in `x' and setting the first element of `x' to the position 
	at which `func' equals zero within the given tolerance `tol'
 */
void secant_solve(SecantSubjectFunction *func, void *user_data, double x[2], double tol){
#define MAX_ITER 30
	unsigned i;
	double y[2];
	double delta_x;

	y[1] = (*func)(x[1], user_data);

	for(i=0;i<MAX_ITER;i++){
		y[0] = (*func)(x[0], user_data);
		if(fabs(y[0])<tol){
			printf("\n\n\tRoot-finding SUCCEEDED after %u iterations;"
					"\n\t  zeroed function has value %.6g at postion %.6g\n", i, y[0], x[0]);
			break;
		}
		if(x[0]==x[1]){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  independent variables equal at %.6g,"
					"\n\t  function is not zero, but %.6g",
					i, x[0], y[0]);
			break;
		}

		/* update independent variable x[0] */
		delta_x = -y[0] * (x[0] - x[1])/(y[0] - y[1]);
		x[1] = x[0];     /* reassign second position to first position */
		y[1] = y[0];
		x[0] += delta_x; /* shift first position to more accurate value */
	}
	/* puts("\n\tLeaving secant root-finding routine now"); */
#undef MAX_ITER
}

