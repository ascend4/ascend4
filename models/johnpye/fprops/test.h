#ifndef FPROPS_TEST_H
#define FPROPS_TEST_H

#include "rundata.h"
#include "color.h"
//#include "sat.h"

/* Macros and type declarations for running simple test cases */

/**
	Set up test environment, if required.
*/
void test_init();

typedef struct{double T,p,rho,u,h,s,cv,cp,cp0,a;} TestData;

typedef struct{double T,p,rhof,rhog,hf,hg,sf,sg;} TestDataSat;

/*, {Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cp, Cp0, Helmholtz} */

/**
	Run tests for p, u, h, s, a against values from a user-provided TestData array
	Tolerances are specified in the cdoe, in test.c.

	@param temp_unit Set to 'C' for celsius or 'K' for Kelvin.

	@return 1 if any failures occurred.
*/
int helm_run_test_cases(const PureFluid *d, unsigned ntd, const TestData *td, int temp_unit);

/**
	Run tests data in the saturation region from a user-provided TestData array
	Tolerances are specified in the code, in test.c.

	@param temp_unit Set to 'C' for celsius or 'K' for Kelvin.

	@return 1 if any failures occurred.
*/
int helm_run_saturation_tests(const PureFluid *d, unsigned nsd, const TestDataSat *td, int temp_unit);

/**
	Calculate values of 'c' and 'm' for the IdealData, based on expected values of h, s at reference
	state.

	TODO incorporate this with *_prepare functions somehow? Move to fprops.h?
*/
int helm_calc_offsets(double Tref, double rhoref, double href, double sref, const PureFluid *fluid);


/**
	Check 'u' values and output discrepancy for plotting.
*/
int helm_check_u(const PureFluid *d, unsigned ntd, const TestData *td);

/**
	Check dp/dT values (rho constant) by comparison with finite
	difference estimates (check that helmholtz_p is working first!)
*/
int helm_check_p_T(const PureFluid *d, unsigned ntd, const TestData *td);


int helm_check_d2pdrho2_T(const PureFluid *d, unsigned ntd, const TestData *td);

/*
	Check the value of critical pressure given in the species data.
*/
int helm_check_p_c(const HelmholtzData *d);

#define TEST_VERBOSE

#ifdef TEST_VERBOSE
# define TEST_SUCCESS(FN,PARAM1,PARAM2,PARAM3,VAL) \
		fprintf(stderr,"    ");\
		color_on(stderr,ASC_FG_GREEN);\
		fprintf(stderr,"OK");\
		color_off(stderr);\
		fprintf(stderr,", %s(%f,%f,%s) = %8.2e with %.6f%% err.\n"\
		,FN,PARAM1,PARAM2,#PARAM3,VAL,relerrpc\
	)
# define TEST_SUCCESS_PROP(PROP,STATE,VAL) \
		fprintf(stderr,"    ");\
		color_on(stderr,ASC_FG_GREEN);\
		fprintf(stderr,"OK");\
		color_off(stderr);\
		fprintf(stderr,", %s(T=%f,rho=%f) = %8.2e with %.6f%% err.\n"\
		,PROP,STATE.T,STATE.rho,VAL,relerrpc\
	)
#else
# define TEST_SUCCESS
# define TEST_SUCCESS_PROP
#endif

#define ASSERT(FACT) {\
		if(!(FACT)){\
			color_on(stderr,ASC_BG_RED|ASC_FG_BLACK);\
			fprintf(stderr,"ERROR");\
			color_off(stderr);\
			fprintf(stderr," %s:%d: failed assertion '%s'\n",__FILE__,__LINE__,#FACT);\
			exit(1);\
		}\
	}
/* a simple macro to actually do the testing */
#define ASSERT_TOL(FN,PARAM1,PARAM2,PARAM3,PARAM4,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3,PARAM4);\
		double x_err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(x_err)>fabs(TOL)){\
			fprintf(stderr,"    ");\
			color_on(stderr,ASC_FG_BRIGHTRED);\
			fprintf(stderr,"ERROR");\
			color_off(stderr);\
			fprintf(stderr," %s:%d: value of '%s(%f,%f,%s)' = %.5e,"\
				" should be %.5e, error is %.10e (%.2f%%, %1e)!\n"\
				, __FILE__,__LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc,relerrpc/100\
			);\
			exit(1);\
		}else{\
			TEST_SUCCESS(#FN,PARAM1,PARAM2,PARAM3,VAL);\
		}\
	}

#define ASSERT_PROP(PROP,STATE,ERR1,VAL,TOL){\
		double cval = fprops_##PROP(STATE,ERR1);\
		double x_err = cval - (double)VAL;\
		double relerrpc	= (cval - (VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(x_err)>fabs(TOL)){\
			fprintf(stderr,"    ");\
			color_on(stderr,ASC_FG_BRIGHTRED);\
			fprintf(stderr,"ERROR");\
			color_off(stderr);\
			fprintf(stderr," %s:%d: value of '%s(T=%f,rho=%f)' = %.5e,"\
				" should be %.5e, error is %.10e (%.2f%%, %1e)!\n"\
				, __FILE__,__LINE__, #PROP,STATE.T,STATE.rho, cval, VAL,x_err\
				,relerrpc,relerrpc/100\
			);\
			exit(1);\
		}else{\
			TEST_SUCCESS_PROP(#PROP,STATE,VAL);\
		}\
	}

/* a simple macro to actually do the testing */
#define ASSERT_TOL_3(FN,PARAM1,PARAM2,PARAM3,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3);\
		double x_err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(x_err)>fabs(TOL)){\
			fprintf(stderr,"    ");\
			color_on(stderr,ASC_FG_BRIGHTRED);\
			fprintf(stderr,"ERROR");\
			color_off(stderr);\
			fprintf(stderr," %s:%d: value of '%s(%f,%f,%s)' = %.5e,"\
				" should be %.5e, error is %.10e (%.2f%%, %1e)!\n"\
				, __FILE__,__LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc,relerrpc/100\
			);\
			exit(1);\
		}else{\
			TEST_SUCCESS(#FN,PARAM1,PARAM2,PARAM3,VAL);\
		}\
	}

/* even simpler testing of an assertion */
#define ASSERT_TOL_VAL(CALC,REF,TOL) {\
		double cval = (CALC);\
		double rval = (REF);\
		double x_err = cval - rval;\
		double relerr = (cval-rval)/rval;\
		if(fabs(relerr*100)>maxerr)maxerr=fabs(relerr*100);\
		if(fabs(x_err)>fabs(TOL)){\
			fprintf(stderr,"    ");\
			color_on(stderr,ASC_FG_BRIGHTRED);\
			fprintf(stderr,"ERROR");\
			color_off(stderr);\
			fprintf(stderr," %s:%d: value of '%s' = %.5e,"\
				" should be %.5e, error is %.10e (%.2f%%, %1e)!\n"\
				,__FILE__,__LINE__,#CALC,cval,rval,cval-rval,relerr*100,relerr\
			);\
			exit(1);\
		}else{\
			fprintf(stderr,"    ");\
			color_on(stderr,ASC_FG_GREEN);\
			fprintf(stderr,"OK");\
			color_off(stderr);\
			fprintf(stderr,", %s = %8.2e with %0.6f%% err.\n"\
				,#CALC,rval,relerr*100\
			);\
		}\
	}

#endif
