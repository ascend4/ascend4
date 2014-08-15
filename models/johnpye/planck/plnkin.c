/*
Function to evaluate the Planck integral, following the method described in
http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19680008986.pdf

Thomas Michels, 1968, "Planck Functions and Integrals; Methods of Computation"
NASA Technical Note NASA TN D-4446, Goddard Space Flight Center, Greenbelt, Md.

The appendix of that report contains a Fortran function, which has been 
loosely converted to C below.

To test this code,
gcc -DTEST plnkin.c -lm && ./a.out
*/

#ifdef TEST
# include <stdio.h>
# define MSG(FMT,...) fprintf(stderr,FMT "\n",##__VA_ARGS__);
#else
# define MSG(...) ((void)0)
#endif

#include <math.h>
#define SQ(X) ((X)*(X))


double plnkin(double lambda, double T){
	//if(lambda*100*T > 1000.)return 1.;
	//if(lambda*100*T < 1./30)return 0.;
	double c2 = 1.43879;
	double y = 0.01/(lambda*T);
	//MSG("y=%f",y);
	double z = c2*y;
	double a2 = exp(z);
	double s = 0;
	//MSG("z=%f",z);
	int x = 0;
	if(y >= 7.4)x = 4;
	else if(y >= 4.2)x = 2;
	else if(y >= 0.45)x = (unsigned)(-2.3*y + 12.96);
	else x = (unsigned)(-46.66* y + 33.);

	//MSG("x=%d",x);
	double a1 = exp(-x*z);
	while(--x>0){
		a1 *= a2;
		double arg = x*z;
		s += a1* (6. + arg*(6. + arg*(3. + arg))) / SQ(SQ(x));
		//MSG("x=%d, a1 = %f, s = %f",x,a1,s);
	}
	
	return 0.15399*s;
}

#ifdef TEST
#include <stdio.h>



typedef struct{
	double recip_lamT;
	double I_norm;
} TestPoint;

int main(void){
	TestPoint pts[] = {
		{0.001, 1.}
		,{0.5, 9.8555e-1}
		,{2.0, 6.3372e-1}
		,{4.0, 1.6135e-1}
		,{6.0, 2.5374e-2}
		,{8.0, 3.0840e-3}
		,{10., 3.2074e-4}
		,{15., 7.5600e-7}
		,{19., 4.7196e-9}
		,{22., 9.6303e-11}
		,{26., 4.9586e-13}
		,{28., 3.4646e-14}
		,{30., 2.3857e-15}
	};
	unsigned npt = sizeof(pts)/sizeof(TestPoint);

	int i;
	int err = 0;
	for(i=0; i<npt; ++i){
		double ind = pts[i].recip_lamT;
		double val = plnkin(0.01,1/pts[i].recip_lamT);
		double ref = pts[i].I_norm;

		if(fabs((val - ref)/ref)>0.0001){\
			err++;\
			fprintf(stderr,"ERROR: for 1/lamT = %f, computed value %e != expected value %e (rel err %f%%\n",ind,val,ref,(val-ref)/ref*100);\
		}else{\
			fprintf(stderr,"OK at %f, computed value %e = expected value %e\n",ind,val,ref);\
		}
	}

	if(err){
		fprintf(stderr,"\n%d ERRORS ENCOUNTERED.\n",err);
	}else{
		fprintf(stderr,"\nSUCCESS, NO ERRORS.\n");
	}
	return err;
}

#endif



