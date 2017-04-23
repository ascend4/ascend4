/*	ASCEND modelling environment
	Copyright (C) 2017 John Pye

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
*/
#include <ascend/general/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>

#include <test/common.h>
#include "test/assertimpl.h"

//#define TESTERROR_DEBUG
#ifdef TESTERROR_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

#define MESSAGEMAX 1000
#define NMESSAGES 1000
char messages[NMESSAGES][MESSAGEMAX];
const char *filenames[NMESSAGES];
const char *funcnames[NMESSAGES];
error_severity_t sevs[NMESSAGES];
int lines[NMESSAGES];
int currmessage = 0;

int reporter_test(const error_severity_t sev, const char *filename
  , const int line, const char *funcname, const char *fmt, va_list args
){
	assert(currmessage >= 0);
	assert(currmessage < NMESSAGES);
	int n = snprintf(messages[currmessage],MESSAGEMAX,fmt,args);
	MSG(fmt,args);
	if(n > MESSAGEMAX - 10){
		assert(0);
	}
	filenames[currmessage] = filename;
	lines[currmessage] = line;
	funcnames[currmessage] = funcname;
	sevs[currmessage] = sev;
	currmessage++;
	return currmessage;
}

error_reporter_tree_t *TREECURRENT(){
	return error_reporter_get_tree_current();
}

static void test_error(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();

	currmessage = 0;
	error_reporter_set_callback(reporter_test);
	CU_TEST(currmessage == 0);

	error_reporter(ASC_USER_ERROR,"thisfile",987,NULL,"hello");

	CU_TEST(currmessage == 1);
	CU_TEST(strcmp(filenames[0],"thisfile")==0);
	CU_TEST(lines[0] == 987);
	CU_TEST(strcmp(messages[0],"hello")==0);
	CU_TEST(sevs[0] == ASC_USER_ERROR);

	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam");

	CU_TEST(currmessage == 2);
	CU_TEST(strcmp(filenames[0],"thisfile")==0);
	CU_TEST(lines[0] == 987);
	CU_TEST(strcmp(messages[0],"hello")==0);
	CU_TEST(sevs[0] == ASC_USER_ERROR);
	CU_TEST(strcmp(filenames[1], "otherfile")==0);
	CU_TEST(lines[1] == 123);
	CU_TEST(strcmp(messages[1],"salaam")==0);
	CU_TEST(sevs[1] == ASC_USER_NOTE);

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"bonjour");
	CU_TEST(currmessage == 3);
	CU_TEST(strcmp(filenames[1], "otherfile")==0);
	CU_TEST(lines[1] == 123);
	CU_TEST(sevs[1] == ASC_USER_NOTE);
	CU_TEST(strcmp(messages[1],"salaam")==0);
	CU_TEST(strcmp(messages[2],"bonjour")==0);
	CU_TEST(sevs[2] == ASC_PROG_ERR);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree1(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a non-caching tree with one message
	currmessage = 0;
	error_reporter_tree_t *T1 = error_reporter_tree_start(0);

	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam");
	CU_TEST(currmessage == 1);

	CU_TEST(NULL != TREECURRENT());		

	CU_TEST(T1 == TREECURRENT());
	error_reporter_tree_end(T1);

	CU_TEST(NULL == TREECURRENT());

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}


static void test_errortree2(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a caching tree with three messages
	currmessage = 0;
	error_reporter_tree_t *T2 = error_reporter_tree_start(1);
	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam1");
	error_reporter(ASC_USER_NOTE,"otherfile",111,"funcname","salaam2");
	error_reporter(ASC_USER_NOTE,"otherfile",333,"funcname","salaam3");
	CU_TEST(currmessage == 0);
	CU_TEST(T2 == TREECURRENT());
	error_reporter_tree_end(T2);
	CU_TEST(currmessage == 3);
	CU_TEST(strcmp(messages[0],"salaam1")==0);
	CU_TEST(strcmp(messages[1],"salaam2")==0);
	CU_TEST(strcmp(messages[2],"salaam3")==0);
	CU_TEST(TREECURRENT()==NULL);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree3(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a non-caching tree with three messages
	currmessage = 0;
	error_reporter_tree_t *T3 = error_reporter_tree_start(0);
	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","hola1");
	error_reporter(ASC_USER_NOTE,"otherfile",111,"funcname","hola2");
	error_reporter(ASC_USER_NOTE,"otherfile",333,"funcname","hola3");
	CU_TEST(currmessage == 3);
	CU_TEST(strcmp(messages[0],"hola1")==0);
	CU_TEST(strcmp(messages[1],"hola2")==0);
	CU_TEST(strcmp(messages[2],"hola3")==0);

	CU_TEST(T3 == TREECURRENT());
	error_reporter_tree_end(T3);
	CU_TEST(currmessage == 3);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree4(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a message, then a non caching tree with a non-caching subtree
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_t *T4 = error_reporter_tree_start(0);
	error_reporter(ASC_USER_NOTE,"f2",222,"fn1","allo1");
	error_reporter(ASC_USER_NOTE,"f2",333,"fn2","allo2");
	error_reporter(ASC_USER_NOTE,"f2",444,"fn3","allo3");
	CU_TEST(strcmp(messages[1],"allo1")==0);
	CU_TEST(strcmp(messages[2],"allo2")==0);
	CU_TEST(strcmp(messages[3],"allo3")==0);
	CU_TEST(lines[3]==444);
	CU_TEST(lines[2]==333);
	CU_TEST(lines[1]==222);
	CU_TEST(currmessage == 4);
	CU_TEST(T4 == TREECURRENT());
	error_reporter_tree_t *t1 = TREECURRENT();
	error_reporter_tree_t *T5 = error_reporter_tree_start(0);
	CU_TEST(T5 == TREECURRENT());
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_USER_NOTE,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 5);
	error_reporter_tree_end(T5);
	CU_TEST(T4 == TREECURRENT());
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==t1);
	CU_TEST(!error_reporter_tree_has_error(T4));
	error_reporter_tree_end(T4);
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==NULL);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree5(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a message, then a caching tree with a non-caching subtree
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_t *T6 = error_reporter_tree_start(1); // caching
	error_reporter(ASC_USER_NOTE,"f2",222,"fn1","allo1");
	error_reporter(ASC_USER_NOTE,"f2",333,"fn2","allo2");
	error_reporter(ASC_USER_NOTE,"f2",444,"fn3","allo3");
	CU_TEST(currmessage == 1);
	error_reporter_tree_t *t1 = TREECURRENT();
	error_reporter_tree_t *T7 = error_reporter_tree_start(0);
	CU_TEST(currmessage == 1);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_PROG_ERR,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 1);
	error_reporter_tree_end(T7);
	CU_TEST(T6 == TREECURRENT());
	CU_TEST(currmessage == 1);
	CU_TEST(TREECURRENT()==t1);
	CU_TEST(error_reporter_tree_has_error(T6));
	error_reporter_tree_end(T6);
	CU_TEST(lines[1]==222);
	CU_TEST(lines[2]==333);
	CU_TEST(lines[3]==444);
	CU_TEST(lines[4]==555);
	CU_TEST(strcmp(messages[1],"allo1")==0);
	CU_TEST(strcmp(messages[2],"allo2")==0);
	CU_TEST(strcmp(messages[3],"allo3")==0);
	CU_TEST(strcmp(messages[4],"buongiorno")==0);
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==NULL);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree6(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a message, then a non-caching tree with a caching subtree
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_t *T8 = error_reporter_tree_start(0); // non-caching
	error_reporter(ASC_USER_NOTE,"f2",222,"fn1","allo1");
	error_reporter(ASC_USER_NOTE,"f2",333,"fn2","allo2");
	error_reporter(ASC_USER_NOTE,"f2",444,"fn3","allo3");
	CU_TEST(currmessage == 4);
	error_reporter_tree_t *t1 = TREECURRENT();
	error_reporter_tree_t *T9 = error_reporter_tree_start(1); // caching
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_USER_ERROR,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 4);
	CU_TEST(T9 == TREECURRENT());
	error_reporter_tree_end(T9);
	CU_TEST(T8 == TREECURRENT());
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==t1);
	CU_TEST(error_reporter_tree_has_error(T8));
	error_reporter_tree_end(T8);
	CU_TEST(lines[1]==222);
	CU_TEST(lines[2]==333);
	CU_TEST(lines[3]==444);
	CU_TEST(lines[4]==555);
	CU_TEST(strcmp(messages[0],"allo0")==0);
	CU_TEST(strcmp(messages[1],"allo1")==0);
	CU_TEST(strcmp(messages[2],"allo2")==0);
	CU_TEST(strcmp(messages[3],"allo3")==0);
	CU_TEST(strcmp(messages[4],"buongiorno")==0);
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==NULL);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

static void test_errortree7(void){
	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse();
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test clearing away a nested error
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_t *T10 = error_reporter_tree_start(0); // non-caching
	error_reporter(ASC_USER_NOTE,"f2",222,"fn1","allo1");
	error_reporter(ASC_USER_NOTE,"f2",333,"fn2","allo2");
	error_reporter(ASC_USER_NOTE,"f2",444,"fn3","allo3");
	CU_TEST(currmessage == 4);
	error_reporter_tree_t *t1 = TREECURRENT();

	error_reporter_tree_t *T11 = error_reporter_tree_start(1); // caching
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_USER_ERROR,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 4);
	CU_TEST(T11 == TREECURRENT());
	error_reporter_tree_end_clear(T11);

	CU_TEST(T10 == TREECURRENT());
	CU_TEST(T11 != T10);
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()==t1);
	CU_TEST(!error_reporter_tree_has_error(T10));
	error_reporter_tree_end(T10);
	CU_TEST(lines[0]==111);
	CU_TEST(lines[1]==222);
	CU_TEST(lines[2]==333);
	CU_TEST(lines[3]==444);
	CU_TEST(strcmp(messages[0],"allo0")==0);
	CU_TEST(strcmp(messages[1],"allo1")==0);
	CU_TEST(strcmp(messages[2],"allo2")==0);
	CU_TEST(strcmp(messages[3],"allo3")==0);
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()==NULL);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
	error_reporter_set_callback(NULL);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(error) \
	T(errortree1) \
	T(errortree2) \
	T(errortree3) \
	T(errortree4) \
	T(errortree5) \
	T(errortree6) \
	T(errortree7) \

REGISTER_TESTS_SIMPLE(utilities_error, TESTS)

