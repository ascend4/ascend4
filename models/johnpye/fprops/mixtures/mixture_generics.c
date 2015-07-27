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
	by Jacob Shealy, June 25-July 13, 2015

	Generic functions that are used in modeling mixtures, but that do not have 
	anything specifically to do with mixtures.
 */

#include "mixture_generics.h"
#include "mixture_struct.h"
#include <math.h>

/*
	Find minimum element from an array of doubles.

	Sets minimum value to the first element in the array initially; then it is 
	re-sets it to any element smaller than its current value.
 */
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

/*
	Find minimum *positive* (meaning greater than one, not just non-zero) 
	element from an array of doubles.

	This works differently from 'min_element' above: the variable to hold the 
	minimum is passed in by reference, and the function returns whether the 
	search for a positive element was a success or failure.

	Initially, minimum value is set to -1, so that if no elements are positive, 
	the function can return 1 (failure: no minimum value was found).  Otherwise, 
	the function returns 0 (success).  Minimum value is iteratively set to any 
	positive element that is smaller than its current value, with the exception 
	of the first assignment -- first assignment (minimum is still negative) only 
	requires a positive number.
 */
int min_positive_elem(double *min, unsigned nelems, double *nums){
	unsigned i;
	*min = -1;

	for(i=0;i<nelems;i++){
		if(nums[i]>0 && (nums[i]<*min || *min<0)){
			*min = nums[i];
		}
	}
	if(*min<0){
		return 1; /* no positive minimum element found -- failure */
	}else{
		return 0;
	}
}

/*
	Find maximum element from an array of doubles.

	Initially, maximum value is set equal to the first element in the array; 
	then it is re-assigned to equal any element larger than its current value.
 */
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

/*
	Calculate sum of the elements of an array of doubles.

	The sum is initially set to zero, then elements are added in turn.
 */
double sum_elements(unsigned nelems, double *nums){
	unsigned i;
	double sum=0.0;

	for(i=0;i<nelems;i++){
		sum += nums[i];
	}
	return sum;
}

/*
	Find index of the minimum element in an array of doubles.

	Sets index of minimum element to zero initially and minimum value to first 
	element; then re-sets index to the index of any element smaller than the 
	minimum, and re-sets minimum value also.  Note that the loop indexes from 1, 
	since the initial (0) element has already been treated.
 */
unsigned index_of_min(unsigned nelems, double *nums){
	unsigned i;
	unsigned min_ix=0;  /* index of the minimum value */
	double min=nums[0]; /* minimum value */

	for(i=1;i<nelems;i++){
		if(nums[i]<min){
			min_ix = i;    /* update both `min' and `min_ix' */
			min = nums[i];
		}
	}
	return min_ix;
}

/*
	Find index of the maximum value in an array of doubles.

	Sets index of maximum element to zero initially and maximum value to first 
	element; then re-sets index to the index of any element larger than the 
	maximum, and re-sets maximum value also.  Note that the loop indexes from 1, 
	since the initial (0) element has already been treated.
 */
unsigned index_of_max(unsigned nelems, double *nums){
	unsigned i;
	unsigned max_ix=0;  /* index of the maximum element */
	double max=nums[0]; /* maximum element */

	for(i=1;i<nelems;i++){
		if(nums[i]>max){
			max_ix = i;    /* update both 'max' and 'max_ix' */
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
	unsigned i
		/* , ix_low */ /* index of point (x,y) with smallest y-error */
		;
	double y[2];
	double delta_x;

	y[1] = (*func)(x[1], user_data);

	for(i=0;i<MAX_ITER;i++){
		y[0] = (*func)(x[0], user_data);
		/* ix_low = (fabs(y[0]) < fabs(y[1])) ? 0 : 1; */ /* find point with smallest y-error */
		if(fabs(y[0])<tol){
			MSG("Root-finding SUCCEEDED after %u iterations;"
					"\n\t  zeroed function has value %.6g at postion %.6g\n", i, y[0], x[0]);
			break;
		}
		if(fabs(x[0] - x[1])<tol){
			MSG("Root-finding FAILED after %u iterations;"
					"\n\t  independent variables equal at %.6g,"
					"\n\t  function is not zero, but %.6g",
					i, x[0], y[0]);
			break;
		}
		/* Break if variables are infinity or NaN */
		if(x[0]==INFINITY || y[0]==INFINITY 
				|| x[0]!=x[0] || y[0]!=y[0]){
			MSG("Root-finding FAILED after %u iterations;"
					"\n\t  independent variable equals %.6g,"
					"\n\t  function output equals %.6g"
					, i, x[0], y[0]);
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

/*
	Find mole fractions of a mixture from the mass fractions and identities of 
	the fluids that form the solution.  This implements the relation

	Mole Fraction of component 'i' = (Mass Fraction of 'i'/Molar Mass of 'i') /
	                                 (Sum over components 'j' (Mass Fraction of 'j'/Molar Mass of 'j'))

	x_i^{mole} = \frac{\frac{x_i^{mass}}{M_i}}{\sum\limits_j \frac{x_j^{mass}}{M_j}}

	Unfortunately, we can't use 'sum_elements' to find the sum 'XM_sum', so it 
	is calculated in a loop.
 */
void mole_fractions(unsigned n_pure, double *x_mole, double *X_mass, PureFluid **PF){
#define TITLE "<mole_fractions>: "
#define D PF[i]->data
	unsigned i;
	double XM_sum=0.0; /* sum of mass fraction over molar mass terms */

	for(i=0;i<n_pure;i++){
		XM_sum += X_mass[i] / D->M;
	}
	for(i=0;i<n_pure;i++){
		x_mole[i] = X_mass[i] / D->M / XM_sum;
	}
#undef D
}

/*
	Find all real roots of a cubic equation of the form 
		a*x^3 + b*x^2 + c*x + d = 0
	There will be at least one, and possibly two or three, real roots.
 */
int cubic_solution(double coef[4], double *roots){
	double p, q, r,
		   a, b,
		   cond;

	p = coef[1]/coef[0];
	q = coef[2]/coef[0];
	r = coef[3]/coef[0];
	a = (3*q - pow(p, 2)) / 3;
	b = (2*pow(p, 3) - (9*p*q) + (27*r)) / 27;
	cond = (pow(b, 2) / 4) + (pow(a, 3) / 27);

	if(cond>0){
		roots[0] = cbrt(-b/2 + sqrt(cond)) + cbrt(-b/2 - sqrt(cond)) - p/3;
		return 1;
	}else if(cond<0){
		double R, phi;

		R = pow(b, 2) / (4 * (-pow(a, 3)/27));
		phi = acos(sqrt(R));

		int i;
		for(i=0;i<3;i++){
			if(b>0){
				roots[i] = -2 * sqrt(-a/3) * cos(phi/3 + (MIX_PI*2*i)/3) - p/3;
			}else{
				roots[i] = 2 * sqrt(-a/3) * cos(phi/3 + (MIX_PI*2*i)/3) - p/3;
			}
		}
		return 3;
	}else{ /* cond equals zero */
		double A;

		A = sqrt(a/3) * ((b>0) ? -1 : 1);
		roots[0] = 2*A - p/3;
		roots[1] = -A - p/3;

		return 2;
	}
	return 0; /* error -- no roots returned */
}

