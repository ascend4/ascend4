/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//**
	@file
	Unit test functions for compiler. Nothing here yet.
*/
#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/func.h>

#include <test/common.h>

static void test_test1(void){
	gl_init_pool();
	gl_init();
	InitDimenList();

	const struct Func *f = LookupFunc("lnxxxx");
	CU_ASSERT(NULL == f);
	
#define EPS 1e-10

	f = LookupFunc("ln");
	CU_ASSERT(NULL != f);	
	CU_ASSERT(FuncEval(f,1)==0);
	CU_ASSERT(FuncDeriv(f,1)==1);
	CU_ASSERT(FuncEval(f,exp(1))==1);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,exp(1)),1/exp(1),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,exp(1)),-1/exp(1)/exp(1),EPS);
	CU_ASSERT(FuncEval(f,exp(2))==2);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,exp(2)),1/exp(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,5),-1./25.,EPS);
	CU_ASSERT(0==strcmp(FuncName(f),"ln"));
	CU_ASSERT(0==strcmp(FuncCName(f),"log"));
	CU_ASSERT(0==strcmp(FuncDeriv1CName(f),"dln"));
	CU_ASSERT(0==strcmp(FuncDeriv2CName(f),"dln2"));
	CU_ASSERT_FATAL(NULL!=FuncDimens(f));
	CU_ASSERT(0==CmpDimen(FuncDimens(f),Dimensionless()));
	CU_ASSERT(F_LN==FuncId(f));

	f = LookupFunc("abs");
	CU_ASSERT(NULL != f);	
	CU_ASSERT(FuncEval(f,1.5)==1.5);
	CU_ASSERT(FuncEval(f,-1.5)==1.5);
	CU_ASSERT(FuncEval(f,0)==0);
	CU_ASSERT(FuncDeriv(f,1.5)==1);
	CU_ASSERT(FuncDeriv(f,-1.5)==-1);
	CU_ASSERT(FuncDeriv2(f,1.5)==0);
	CU_ASSERT(FuncDeriv2(f,-1.5)==0);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));
	CU_ASSERT(F_ABS==FuncId(f));

	f = LookupFunc("sqrt");
	CU_ASSERT(NULL != f);	
	CU_ASSERT(FuncEval(f,15*15)==15);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2),1.41421356237309504880168872,EPS);
	CU_ASSERT(FuncEval(f,1)==1);
	CU_ASSERT(FuncEval(f,0)==0);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,2),1./2./1.41421356237309504880168872,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,2),-1.41421356237309504880168872/16.,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));
	CU_ASSERT(F_SQRT==FuncId(f));

	f = LookupFunc("sqrt");
	CU_ASSERT(NULL != f);	
	CU_ASSERT(FuncEval(f,15*15)==15);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2),1.41421356237309504880168872,EPS);
	CU_ASSERT(FuncEval(f,1)==1);
	CU_ASSERT(FuncEval(f,0)==0);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,2),1./2./1.41421356237309504880168872,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,2),-1.41421356237309504880168872/16.,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));
	CU_ASSERT(F_SQRT==FuncId(f));

	f = LookupFunc("log10");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1000.),3.,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1e6),6.,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,100),1./100./log(10),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,1/log(10)),-log(10),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,1),-1/log(10),EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),Dimensionless()));

	// trig

	f = LookupFunc("sin");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2*M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI/2),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,3./2*M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1./2*M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,0),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,M_PI/2),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-M_PI/2),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI/2),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-M_PI/2),+1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI/4),1/sqrt(2),EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),TrigDimension()));

	f = LookupFunc("tan");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI/4),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,3./4*M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-3./4*M_PI),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1./4*M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,M_PI/4),2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,3*M_PI/4),2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-3*M_PI/4),2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI/4),4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,3*M_PI/4.),-4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-3*M_PI/4.),4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,0),0,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),TrigDimension()));

	f = LookupFunc("cos");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2*M_PI),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI/4),1./sqrt(2),EPS);
 	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI/2),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,3*M_PI/4),-1./sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,M_PI),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-3*M_PI/4),-1./sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-M_PI/2),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-M_PI/4),1./sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,M_PI/2),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-M_PI/2),+1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,M_PI),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,0),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI/4),-1/sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI/2),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,M_PI),1,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),TrigDimension()));

	// inv trig

	f = LookupFunc("arcsin");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),M_PI/2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1),-M_PI/2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1./sqrt(2)),M_PI/4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1./sqrt(2)),-M_PI/4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,0),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,1/sqrt(2)),sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-1/sqrt(2)),sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,1/sqrt(2)),2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-1/sqrt(2)),-2,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),Dimensionless()));

	f = LookupFunc("arccos");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),M_PI/2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1),M_PI,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1/sqrt(2)),M_PI/4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1/sqrt(2)),3*M_PI/4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,0),-1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,1/sqrt(2)),-sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-1/sqrt(2)),-sqrt(2),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,1/sqrt(2)),-2,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-1/sqrt(2)),2,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),Dimensionless()));


	/* TODO test undefined values, exceptions/FPEs */

	// hyp trig

	f = LookupFunc("sinh");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1.),1.1752011936438,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1./2),0.521095305493747,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2),3.62686040784702,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-2),-3.62686040784702,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-2),3.76219569108363,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-2),-3.62686040784702,EPS);

	f = LookupFunc("cosh");
	CU_ASSERT(NULL != f);	
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),1.54308063481524,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1./2),1.12762596520638,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2),3.76219569108363,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-2),3.76219569108363,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-2),-3.62686040784702,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-2),3.76219569108363,EPS);

	f = LookupFunc("cosh");
	CU_ASSERT(NULL != f);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),1.54308063481524,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1./2),1.12762596520638,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,2),3.76219569108363,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-2),3.76219569108363,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-2),-3.62686040784702,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-2),3.76219569108363,EPS);

	f = LookupFunc("tanh");
	CU_ASSERT(NULL != f);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),0.761594155955765,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,-1),-0.761594155955765,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,-1),0.419974341614026,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,-1),0.639700008449225,EPS);

	f = LookupFunc("sqr");
	CU_ASSERT(NULL != f);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,9),81,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,9),18,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,9),2,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));

	f = LookupFunc("cube");
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,4),64,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,4),48,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,4),24,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));

	f = LookupFunc("cbrt");
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,0),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,1),1,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,64),4,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,64),1./48,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,64),-1./48./96.,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));

	f = LookupFunc("hold");
	CU_ASSERT(NULL != f);
	CU_ASSERT_DOUBLE_EQUAL(FuncEval(f,37),37,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,37),0,EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,37),0,EPS);
	CU_ASSERT(0==CmpDimen(FuncDimens(f),WildDimension()));

	DestroyDimenList();
	gl_destroy_pool();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_func, TESTS)

