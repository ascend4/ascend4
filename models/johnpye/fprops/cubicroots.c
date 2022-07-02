#include "cubicroots.h"
#include "common.h"

// uncomment the following if you want output from 'MSG' and 'ERRMSG' calls
#define CUBICROOTS_DEBUG
#ifdef CUBICROOTS_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

#include <math.h>

#define SQ(X) ((X)*(X))
#define CUBE(X) ((X)*SQ(X))
#define SWAP(a,b) do { double tmp = b ; b = a ; a = tmp ; } while(0)

#ifndef M_PI
#error "No M_PI"
#endif

#ifndef TEST

typedef double TrigFunction(double);

int sgn(double x){
	return (x>0)-(x<0);
}

double csol(TrigFunction *trig,TrigFunction *invtrig,int S, int k, double P, double Q){
	double absP = fabs(P);
	double sqrtabsP = sqrt(absP);
	MSG("trig = %s",trig==&cos ? "cos" : (trig==&sinh ? "sinh" : (trig==&cosh ? "cosh" : "???")));
	MSG("k = %d, S = %d", k, S);
	MSG("|P| = %f, √|P| = %f",absP,sqrtabsP);
	if(trig==&cosh){
		MSG("Q*S/√|P|³ = %f",Q*S/absP/sqrtabsP);
		if(Q*S/absP/sqrtabsP < 1){
			ERRMSG("invalid P,Q");
		}
	}
	return 2*S*sqrtabsP*(*trig)((*invtrig)(Q*S/absP/sqrtabsP)/3 + 2./3*M_PI*k);
}

int cubicroots(double a, double b, double c, double x[3]){
	MSG("Solving x³ + ax² + bx + c = 0, with a = %f, b = %f, c = %f",a,b,c);

	double a3 = a/3;
	double P = SQ(a3) - b/3;
	double Q = -(a3)*(SQ(a3) - b/2) - c/2;
	MSG("P = %f, Q = %f",P,Q);
	
	if(P == 0 && Q==0){ // TODO apply some precision here?
		x[0] = x[1] = x[2] = -a3;
		MSG("Triple real root x = %f",x[0]);
		return 3;
	}else if(P == 0){
		x[0] = -a3 + pow(2*Q, 1./3);
		MSG("Single real root x = %f (P==0)",x[0]);
		return 1;
	}else if(Q == 0){
		if(P > 0){
			x[0] = -a3 - sqrt(3*P);
			x[1] = -a3;
			x[2] = -a3 + sqrt(3*P);
			MSG("Three real roots, x0 = %f, x1 = %f, x2 = %f (Q==0, P>0)",x[0],x[1],x[2]);
			return 3;
		}else{
			x[0] = -a3;
			MSG("One real root x0 = %f (Q==0, P<0)",x[0]);
			return 1;
		}
	}
	
	double D = SQ(P) - CUBE(P);
	MSG("D = %f",D);
	
	if(D == 0){
		ERRMSG("CASE D==0 NOT HANDLED");
		exit(1);
	}else if(D < 0){
		MSG("D < 0");
		for(int k=0;k<3;++k){ //csol(*trig,*invtrig,S,k,P,Q)
			MSG("Root with k = %d",k);
			x[k] = -a3 + csol(&cos,&acos,1,k,P,Q);
		}
		if(x[1]<x[0])SWAP(x[0],x[1]);
		if(x[2]<x[1])SWAP(x[1],x[2]);
		if(x[1]<x[0])SWAP(x[0],x[1]);
		
		MSG("Three real roots, x0 = %f, x1 = %f, x2 = %f (D<0)",x[0],x[1],x[2]);
		return 3;
	}else{
		MSG("D > 0");
		if(P>0){
			MSG("P > 0");
			x[0] = -a3 + csol(&cosh,&acosh,1,0,P,Q);
			MSG("One real roots, x0 = %f (D>0,P>0)",x[0]);		
			return 1;
		}else{
			MSG("P < 0");
			x[0] = -a3 + csol(&sinh,&asinh,sgn(Q),0,P,Q);
			MSG("One real roots, x0 = %f (D>0,P<0)",x[0]);
			return 1;
		}
	}
}




#if 0
// for SQ(x)...
#include "common.h"

#include <complex.h>
#include <math.h>

#define SWAP(a,b) do { double tmp = b ; b = a ; a = tmp ; } while(0)

#if __STDC_VERSION__ < 199901L
double complex fprops_ccbrt(double complex z){
	double r = cabs(z);
	double th1 = carg(z) / 3.;
	return pow(r,1./3) * (cos(th1) + _Complex_I *sin(th1));
}
# define CCBRT(Z) fprops_ccbrt((Z))
#else
# define CCBRT(Z) cpow((Z), 1./3)
#endif

/*
This function was originally taken from the Ankit branch.  I have modified it so that the equation is not "deflated" by dividing by the pressure.
Because we are using specific volume, and in some cases large pressure, we may need a more numerically sound method that won't return nan's
Should probably be put somewhere else if used.
In this case, coefficients are AZ^3+BZ^2+CZ+D.  SPM

John Pye has modifed further to assume A = 1

Somehow this is messed up for the case of complex roots. Needs to be fixed.
This link might help: https://www.shsu.edu/kws006/professional/Concepts_files/SolvingCubics.pdf
*/
int cubicroots(double B, double C, double D, double *Z0, double *Z1, double *Z2){ // This function will give value of liquid and vapour volumes at P using PR, and store it in reference variables
	MSG("Solving x³ + ax² + bx + c = 0, with a = %f, b = %f, c = %f",B,C,D);

	double discriminant = 18*B*C*D - 4*B*B*B*D + C*C*B*B - 4*C*C*C - 27*D*D;
	double term1 = (2*B*B*B)-(9*B*C)+(27*D);
	double _Complex term2;
	if(discriminant>0)term2 = _Complex_I * sqrt(27*discriminant);
	else term2 = sqrt(-27*discriminant);

	double _Complex cuberootpos = CCBRT(.5*(term1+term2));
	double _Complex cuberootneg = CCBRT(.5*(term1-term2));
	double _Complex complextermpos = (1. + _Complex_I*sqrt(3.))/6.;
	double _Complex complextermneg = (1. - _Complex_I*sqrt(3.))/6.;

	*Z0 = -B/3 - 1./3 * cuberootpos - 1./3 * cuberootneg;
	*Z1 = -B/3 + complextermpos * cuberootpos + complextermneg * cuberootneg;
	*Z2 = -B/3 + complextermneg * cuberootpos + complextermpos * cuberootneg;

#define EVALC(X) ((X)*SQ(X) + B*SQ(X) + C*(X) + D)
	MSG("check Z0 = %f → %f",*Z0, EVALC(*Z0));
	MSG("check Z1 = %f → %f",*Z1, EVALC(*Z1));
	MSG("check Z2 = %f → %f",*Z2, EVALC(*Z2));
	
	if(discriminant>=0){
		MSG("three roots");
		return 3;
	}else{
		MSG("one root");
		return 1;
	}
}

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

#else // TEST

#include <assert.h>

#define CTOL(X,V) assert(fabs((X)-(V))<1e-11)

#define RTEST(A,B,C,NR,X0,X1,X2) \
	a = (A); b = (B); c = (C); \
	MSG("-----");\
	assert(NR==1||NR==3); \
	nr = cubicroots(a,b,c,x);\
	if(NR==3){\
		MSG("Expecting x₀ = %f, x₁ = %f, x₂ = %f",(double)X0,(double)X1,(double)X2); \
	}else{\
		MSG("Expecting x₀ = %f",(double)X0); \
	}\
	assert(nr==(NR)); \
	CTOL(x[0],X0);\
	if(NR==3){ \
		CTOL(x[1],X1);\
		CTOL(x[2],X2);\
	}\
	MSG("OK");

int main(void){

	double a = 3, b = 3, c = 1;
	double x[3];
	int nr;
	
	// cases of triple real roots
	RTEST(3,3,1,       3, -1, -1, -1);
	RTEST(-3,3,-1,     3, 1, 1, 1);
	RTEST(6,12,8,      3, -2, -2, -2);
	RTEST(-12,48,-64,  3, 4, 4, 4);

	// cases with P=0
	RTEST(-12,48,-66,  1, 4+pow(2.,1./3),0, 0);
	RTEST(-9,27,-29,   1, 3+pow(2.,1./3),0,0);

	// cases with Q=0
	MSG("====== P = 0 CASES");
	RTEST(0,-3,0,      3, -sqrt(3), 0, +sqrt(3));
	RTEST(6,6,-4,      3, -2-sqrt(6), -2, -2+sqrt(6));
	RTEST(6,6,-4,      3, -2-sqrt(6), -2, -2+sqrt(6));
	RTEST(-15,79,-145, 1, 5, 0, 0 );
	RTEST(-6,11,-6,    3, 1, 2, 3);
	
	// cases with three real roots
	MSG("====== D < 0 CASES"); // cos
	RTEST(1,-10,8,        3, -4, 1, 2);
	RTEST(0,-6,-4,     3, -2, 1-sqrt(3.),1+sqrt(3.));
	RTEST(51,-4300,60000, 3, -100, 24, 25);
	RTEST(-1001,-1000000,+1001000000, 3, -1000,1000,1001);
	// RTEST(-1e6-1,-1e12,+1000001e12, 3,1e6,1e6+1,-1e6); // fails due to numerical precision
	
	// cases with one real root
	MSG("====== D > 0 CASES");
	RTEST(-1,4,-4,     1, 1, 0,0); // P < 0: sinh
	RTEST(1,4,4,     1, -1, 0,0); // P < 0: sinh
	RTEST(-19,+118,-240, 3, 5, 6, 8); // P > 0: cosh
	
	
	
	return 0;
}

#endif

