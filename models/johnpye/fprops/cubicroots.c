/* 
 * Copyright (C) 2022 John Pye
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
#include "cubicroots.h"
#include "common.h"

// uncomment the following if you want output from 'MSG' and 'ERRMSG' calls
//#define CUBICROOTS_DEBUG
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
#define SGN(X) (((X)>0)-((X)<0))
#ifndef M_PI
#error "No M_PI"
#endif

# define MAXP 1000
# define MAXQP 1000

/*
	select double (default), long double ('extra') or float ('low') precision
	using the #defines below. Note that the test suite will apply a different 
	TESTTOL depending on which machine precision you choose.
	
	Note that the internal precision does not change the function signature; 
	it always expects and returns `double` values.
*/
//#define CUBICROOTS_EXTRA_PRECISION
//#define CUBICROOTS_LOW_PRECISION
#ifdef CUBICROOTS_EXTRA_PRECISION
# define DOUBLE long double
# define FJOIN(FN) FN##l
# define PDBL "%0.16Le"
# define TESTTOL 1e-14
# define TOLD 1e-12L

#else
# ifdef CUBICROOTS_LOW_PRECISION
#  define DOUBLE float
#  define FJOIN(FN) FN##f
#  define PDBL "%0.9e"
#  define TESTTOL 3e-4
#  define TOLD 1e-6L
# else
   // standard ('double') precision
#  define DOUBLE double
#  define FJOIN(FN) FN
#  define PDBL "%g"
#  define TESTTOL 1e-13
#  define TOLD 1e-10 // needed to reduce this for calc of toluene at 5 bar, 260 K
# endif
#endif
#define COS FJOIN(cos)
#define ACOS FJOIN(acos)
#define COSH FJOIN(cosh)
#define ACOSH FJOIN(acosh)
#define SINH FJOIN(sinh)
#define ASINH FJOIN(asinh)
#define FABS FJOIN(fabs)
#define SQRT FJOIN(sqrt)
#define POW FJOIN(pow)

# define TOLQ TOLD
# define TOLP TOLD

#ifndef TEST

typedef DOUBLE TrigFunction(DOUBLE);

/*
	Generalised form of the solution to y^3 - 3P·y -2Q = 0. Requires
	evaluation of the discrimant D first, in order to determine which trig
	functions `trig` and `invtrig` are needed, as well as the sign `S`.
	The variable `k` is used for the case of three real roots only.
	The function returns one root at a time (indexed by `k` where needed).
*/
static DOUBLE ysol(TrigFunction *trig,TrigFunction *invtrig,int S, int k, DOUBLE P, DOUBLE Q){
	DOUBLE absP = FABS(P);
	DOUBLE sqrtabsP = SQRT(absP);
	MSG("trig = %s",trig==&COS ? "cos" : (trig==&SINH ? "sinh" : (trig==&COSH ? "cosh" : "???")));
	MSG("k = %d, S = %d", k, S);
	MSG("|P| = " PDBL " , √|P| = " PDBL,absP,sqrtabsP);
	MSG("Q*S/√|P|³ = " PDBL,Q*S/absP/sqrtabsP);
	if(trig==&COSH){
		if(Q*S/absP/sqrtabsP < 1){
			ERRMSG("invalid P,Q");
		}
	}
	DOUBLE res = 2*S*sqrtabsP*(*trig)((*invtrig)(Q*S/absP/sqrtabsP)/3 + 2./3*M_PI*k);
	MSG("res = " PDBL,res);
	return res;
}

int cubicroots(double a, double b, double c, double x[3]){
	MSG("Solving x³ + ax² + bx + c = 0, with a = %g, b = %g, c = %g",a,b,c);

	DOUBLE a3 = a/3;
	DOUBLE P = SQ(a3) - b/3;
	DOUBLE Q = -(a3)*(SQ(a3) - (DOUBLE)b/2) - (DOUBLE)c/2;
	
	MSG("P = " PDBL ", Q = " PDBL,P,Q);
	MSG("Depressed cubic: x³ + " PDBL " x + " PDBL " = 0",-3*P,-2*Q);
	// large coefficients; scale before and after solving
	// FIXME what to do about small coefficients? is that a problem?
	
	double F = 1.;
	if(FABS(P) > 1e-9){
		DOUBLE absQoverP = FABS(Q/P);
		if(absQoverP > MAXQP){
			F = POW(absQoverP,1./2);
			MSG("Reducing unknown variable x by factor F = %g = |Q/P|",F);
		}
	}
	if(FABS(P) > MAXP){
		F = SQRT(FABS(P));
		MSG("Reducing unknown variable x by factor F = %g = √|P|",F);
	}
	if(F != 1.){
		double w[3];
		double a1 = a/F, b1 = b/SQ(F),c1 = c/CUBE(F);
		a1 = a/F;
		int nr = cubicroots(a1,b1,c1,w);
		for(int i=0;i<3;++i){
			x[i] = w[i] * F;
		}
		if(nr==1){
			MSG("Reinflated value of x is x₀ = %.14e",x[0]);
		}else{
			MSG("Reinflated values of x are x₀ = %.14e, x₁ = %.14e, x₂ = %.14e, ",x[0],x[1],x[2]);
		}
		return nr;
	}


	if(FABS(P) < TOLP && FABS(Q) < TOLQ){
		x[0] = x[1] = x[2] = (double)-a3;																
		MSG("Triple real root x = %f (P==0, Q==0)",x[0]);
		return 3;
	}else if(FABS(P) < TOLP){
		x[0] = (double)(-a3 + POW(2*Q, 1./3));
		MSG("Single real root x = %f (P==0)",x[0]);
		return 1;
	}else if(fabsl(Q) < TOLQ){
		if(P > 0){
			x[0] = (double)(-a3 - SQRT(3*P));
			x[1] = (double)(-a3);
			x[2] = (double)(-a3 + SQRT(3*P));
			MSG("Three real roots, x₀ = %.14e, x1 = %.14e, x2 = %.14e (Q==0, P>0)",x[0],x[1],x[2]);
			return 3;
		}else{
			x[0] = (double)(-a3);
			MSG("One real root x₀ = %.14e (Q==0, P<0)",x[0]);
			return 1;
		}
	}
	
	DOUBLE D = SQ(Q) - CUBE(P);
	MSG("D = " PDBL,D);
	
	if(D < 0 || fabsl(D) < TOLD){
		if(fabsl(D) < TOLD){
			if(D == 0){
				MSG("D == 0"); // three real roots, including a double
			}else{
				MSG("D ≈ 0 (D = " PDBL ")", D);
				
				Q = SGN(Q)*P*SQRT(P);
			}
			x[0] = (double)(-a3 + ysol(&COS,&ACOS,1,0,P,Q));
			x[1] = (double)(-a3 + ysol(&COS,&ACOS,1,1,P,Q));
			if(Q < TOLQ){
				MSG("Single root is x₁");
				x[2] = x[0];
			}else{
				MSG("Single root is x₀");
				x[2] = x[1];
			}
			MSG("Unsorted roots: x₀ = %g, x₁ = %g, x₂ = %g",x[0],x[1],x[2]);
		}else{
			MSG("D <= 0");
			for(int k=0;k<3;++k){ //ysol(*trig,*invtrig,S,k,P,Q)
				MSG("Root with k = %d",k);
				x[k] = (double)(-a3 + ysol(&COS,&ACOS,1,k,P,Q));
			}
		}
		if(x[1]<x[0])SWAP(x[0],x[1]);
		if(x[2]<x[1])SWAP(x[1],x[2]);
		if(x[1]<x[0])SWAP(x[0],x[1]);
		
		MSG("Three real roots%s, x₀ = %.14e, x₁ = %.14e, x₂ = %.14e (D<0)",fabs(D)<1e-12?" (including a double root)":"",x[0],x[1],x[2]);
		return 3;
	}else{
		MSG("D > 0");
		if(P>0){
			MSG("P > 0");
			x[0] = (double)(-a3 + ysol(&COSH,&ACOSH,SGN(Q),0,P,Q));
			MSG("One real roots, x₀ = %.14e (D>0, P>0, %s)",x[0],(Q>0)?"Q>0":"Q<0");		
			return 1;
		}else{
			MSG("P < 0");
			x[0] = (double)(-a3 + ysol(&SINH,&ASINH,1,0,P,Q));
			MSG("One real roots, x₀ = %.14e (D>0, P<0)",x[0]);
			return 1;
		}
	}
}

#else // TEST

#include <assert.h>

#ifndef TESTTOL
# error TESTTOL
#endif

#define CTOL(X,V,TOL) \
	/*fprintf(stderr,"checking %.14e against %.14e\n",X,(double)V);*/ \
	if(fabs(V)<1)assert(fabs((X)-(double)(V))<TOL); \
	else assert(fabs((X)-(double)(V))/fabs((double)V)<TOL)

#define RTEST(A,B,C,NR,X0,X1,X2) \
	a = (A); b = (B); c = (C); \
	MSG("-----");\
	assert(NR==1||NR==3); \
	nr = cubicroots(a,b,c,x);\
	if(NR==3){\
		MSG("Expecting 3 roots (tol = %g): x₀ = %.14e, x₁ = %.14e, x₂ = %.14e",TESTTOL,(double)X0,(double)X1,(double)X2); \
	}else{\
		MSG("Expecting 1 root (tol = %g): x₀ = %.14e",TESTTOL, (double)X0); \
	}\
	assert(nr==(NR)); \
	CTOL(x[0],X0,TESTTOL);\
	if(NR==3){ \
		CTOL(x[1],X1,TESTTOL);\
		CTOL(x[2],X2,TESTTOL);\
	}\
	MSG("OK");

int main(void){

	double a = 3, b = 3, c = 1;
	double x[3];
	int nr;
	
	// cases of triple real roots
	MSG("====== P = Q = 0 CASES");
	RTEST(3,3,1,       3, -1, -1, -1);
	RTEST(-3,3,-1,     3, 1, 1, 1);
	RTEST(6,12,8,      3, -2, -2, -2);
	RTEST(-12,48,-64,  3, 4, 4, 4);
	RTEST(-12,48,-64,  3, 4, 4, 4);
	RTEST(-3e6,3e12,-1e18, 3, 1e6,1e6,1e6);
	//RTEST(-3e9,3e18,-1e24, 3, 1e6,1e6,1e6); // fails due to machine precision... but maybe we can normalise?

	// cases with P=0
	MSG("====== P = 0 CASES");
	RTEST(-12,48,-66,  1, 4+pow(2.,1./3),0, 0);
	RTEST(-9,27,-29,   1, 3+pow(2.,1./3),0,0);

	// cases with Q=0
	MSG("====== Q = 0 CASES");
	RTEST(0,-3,0,      3, -sqrt(3), 0, +sqrt(3));
	RTEST(6,6,-4,      3, -2-sqrt(6), -2, -2+sqrt(6));
	RTEST(6,6,-4,      3, -2-sqrt(6), -2, -2+sqrt(6));
	RTEST(-15,79,-145, 1, 5, 0, 0 );
	RTEST(-6,11,-6,    3, 1, 2, 3);
	
	// cases with three real roots
	MSG("====== D < 0 CASES"); // cos
	RTEST(1,-10,8,        3, -4, 1, 2);
	RTEST(0,-6,-4,     3, -2, 1-sqrt(3.),1+sqrt(3.));
	RTEST(-19,+118,-240, 3, 5, 6, 8);
	RTEST(51,-4300,60000, 3, -100, 24, 25);
	RTEST(-1001,-1000000,+1001000000, 3, -1000,1000,1001);
	//RTEST(-1e6-1,-1e12,+1000001e12, 3,1e6,1e6+1,-1e6); // fails due to numerical precision
	
	// cases with one real root
	MSG("====== D > 0 CASES");
	RTEST(-1,4,-4,     1, 1, 0,0); // P < 0: sinh
	RTEST(1,4,4,     1, -1, 0,0); // P < 0: sinh
	RTEST(0,-6,-9,    1, 3,0,0);
	RTEST(0,-6,+9,    1, -3,0,0);
	
	// cases with a double real root
	MSG("======= D == 0 CASES");
	RTEST(-9,24,-20,   3, 2,2,5);
	RTEST(9,-21,-245,  3, -7,-7,5);
	RTEST(-10,32,-32,  3, 2,4,4); // this case gives tol>1e-8, worst case
	RTEST(1e3,-5e6,+3e9,  3, -3e3,1e3,1e3);
	RTEST(-1e3,-1e6,+1e9,  3, -1e3,1e3,1e3);
	
	// cases that are tested in GSL
	MSG("======== GSL CASES");
	RTEST(0,0,-27,    1, 3, 0,0);
	RTEST(-51,867,-4913,    3, 17, 17, 17);
	RTEST(-57,1071,-6647,   3, 17,17,23);
	RTEST(-143,5087,-50065, 3, 17,31,95);
	RTEST(-109, 803,50065,  3,-17,31,95);
	

	return 0;
}

#endif

