#include <complex.h>
#include <math.h>

#define SWAP(a,b) do { double tmp = b ; b = a ; a = tmp ; } while(0)

/* note that MinGW32 doesn't provide cpow function, so some changes made here: */
#ifndef HAVE_CPOW
double complex fprops_ccbrt(double complex z){
	double r = cabs(z);
	double th1 = carg(z) / 3.;
	return pow(r,1./3) * (cos(th1) + _Complex_I *sin(th1));
}
# define CCBRT(Z) fprops_ccbrt((Z))
#else
# define CCBRT(Z) cpow((Z), 1./3)
#endif

#if 1

/*
This function was originally taken from the Ankit branch.  I have modified it so that the equation is not "deflated" by dividing by the pressure.
Because we are using specific volume, and in some cases large pressure, we may need a more numerically sound method that won't return nan's
Should probably be put somewhere else if used.
In this case, coefficients are AZ^3+BZ^2+CZ+D.  SPM

John Pye has modifed further to assume A = 1
*/
int cubicroots(double B, double C, double D, double *Z0, double *Z1, double *Z2){ // This function will give value of liquid and vapour volumes at P using PR, and store it in reference variables
    #define PI 3.14159265
    double discriminant = 18*B*C*D - 4*B*B*B*D + C*C*B*B - 4*C*C*C - 27*D*D;
	double term1 = (2*B*B*B)-(9*B*C)+(27*D);
	double complex term2;
	if(discriminant>0)term2 = _Complex_I * sqrt(27*discriminant);
	else term2 = sqrt(-27*discriminant);

	double complex cuberootpos = CCBRT(.5*(term1+term2));
	double complex cuberootneg = CCBRT(.5*(term1-term2));
	double complex complextermpos = (1 + _Complex_I*sqrt(3))/6;
	double complex complextermneg = (1 - _Complex_I*sqrt(3))/6;
    
	*Z0 = -B/3 - 1./3 * cuberootpos - 1./3 * cuberootneg;
	*Z1 = -B/3 + complextermpos * cuberootpos + complextermneg * cuberootneg;
    *Z2 = -B/3 + complextermneg * cuberootpos + complextermpos * cuberootneg;
    
    if(discriminant>0) return 3;
    else return 1;
}

#else
// We are seeing weird numerical behaviour from the following function. We will
// try a different approach instead (and instead adopt Ankit's GSOC code)
/* 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007, 2009 Brian Gough
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
int  cubicroots(double a, double b, double c, double *x0, double *x1, double *x2){
	double q = (a * a - 3 * b);
	double r = (2 * a * a * a - 9 * a * b + 27 * c);

	double Q = q / 9;
	double R = r / 54;

	double Q3 = Q * Q * Q;
	double R2 = R * R;

	double CR2 = 729 * r * r;
	double CQ3 = 2916 * q * q * q;

	if (R == 0 && Q == 0){
		*x0 = - a / 3 ;
		*x1 = - a / 3 ;
		*x2 = - a / 3 ;
		return 3 ;
	}else if (CR2 == CQ3){
		/* this test is actually R2 == Q3, written in a form suitable
		 for exact computation with integers */

		/* Due to finite precision some double roots may be missed, and
		 considered to be a pair of complex roots z = x +/- epsilon i
		 close to the real axis. */

		double sqrtQ = sqrt (Q);

		if (R > 0){
			*x0 = -2 * sqrtQ  - a / 3;
			*x1 = sqrtQ - a / 3;
			*x2 = sqrtQ - a / 3;
		}else{
			*x0 = - sqrtQ  - a / 3;
			*x1 = - sqrtQ - a / 3;
			*x2 = 2 * sqrtQ - a / 3;
		}
		return 3 ;
	}else if (R2 < Q3){
		// TODO is the trigonometric way of doing this really the fastest??
		double sgnR = (R >= 0 ? 1 : -1);
		double ratio = sgnR * sqrt (R2 / Q3);
		double theta = acos (ratio);
		double norm = -2 * sqrt (Q);
		*x0 = norm * cos (theta / 3) - a / 3;
		*x1 = norm * cos ((theta + 2.0 * M_PI) / 3) - a / 3;
		*x2 = norm * cos ((theta - 2.0 * M_PI) / 3) - a / 3;
	  
		/* Sort *x0, *x1, *x2 into increasing order */
		if (*x0 > *x1)SWAP(*x0, *x1);
		if (*x1 > *x2){
			SWAP(*x1, *x2);
			if (*x0 > *x1)SWAP(*x0, *x1);
		}
		return 3;
	}else{
	  double sgnR = (R >= 0 ? 1 : -1);
	  double A = -sgnR * pow (fabs (R) + sqrt (R2 - Q3), 1.0/3.0);
	  double B = Q / A ;
	  *x0 = A + B - a / 3;
	  return 1;
	}
}
#endif

