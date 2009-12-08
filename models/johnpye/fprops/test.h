#ifndef HELMHOLTZ_TEST_H
#define HELMHOLTZ_TEST_H

#include "helmholtz.h"

/* Macros and type declarations for running simple test cases */

typedef struct{double T,p,rho,u,h,s,cv,cp,cp0,a;} TestData;

/*, {Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cp, Cp0, Helmholtz}

/**
	Run tests for p, u, h, s, a against values from a user-provided TestData array
	Tolerances are specified in the cdoe, in test.c.

	@param temp_unit Set to 'C' for celsius or 'K' for Kelvin.

	@return 1 if any failures occurred.
*/
int helm_run_test_cases(const HelmholtzData *d, unsigned ntd, const TestData *td, int temp_unit);

/**
	Check 'u' values and output discrepancy for plotting.
*/
int helm_check_u(const HelmholtzData *d, unsigned ntd, const TestData *td);

/**
	Check dp/dT values (rho constant) by comparison with finite
	difference estimates (check that helmholtz_p is working first!)
*/
int helm_check_p_T(const HelmholtzData *d, unsigned ntd, const TestData *td);

#define TEST_VERBOSE

#ifdef TEST_VERBOSE
# define TEST_SUCCESS(FN,PARAM1,PARAM2,PARAM3,VAL) \
	fprintf(stderr,"    OK, %s(%f,%f,%s) = %8.2e with %.6f%% err.\n"\
		,FN,PARAM1,PARAM2,#PARAM3,VAL,relerrpc\
	)
#else
# define TEST_SUCCESS
#endif

/* a simple macro to actually do the testing */
#define ASSERT_TOL(FN,PARAM1,PARAM2,PARAM3,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>fabs(TOL)){\
			fprintf(stderr,"ERROR in line %d: value of '%s(%f,%f,%s)' = %.5e,"\
				" should be %.5e, error is %.10e (%.2f%%)!\n"\
				, __LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc\
			);\
			exit(1);\
		}else{\
			TEST_SUCCESS(#FN,PARAM1,PARAM2,PARAM3,VAL);\
		}\
	}

#endif
