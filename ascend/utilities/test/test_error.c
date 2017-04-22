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
#include <stdio.h>
#define ASC_BUILDING_INTERFACE
#include <ascend/general/platform.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <ascend/utilities/error.h>
#include <test/common.h>

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
	CONSOLE_DEBUG(fmt,args);
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
	currmessage = 0;
	error_reporter_set_callback(reporter_test);
	CU_TEST(currmessage == 0);

	error_reporter(ASC_USER_ERROR,"thisfile",987,NULL,"hello");

	CU_TEST(currmessage == 1);
	CU_TEST(filenames[0] == "thisfile");
	CU_TEST(lines[0] == 987);
	CU_TEST(strcmp(messages[0],"hello")==0);
	CU_TEST(sevs[0] == ASC_USER_ERROR);

	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam");

	CU_TEST(currmessage == 2);
	CU_TEST(filenames[0] == "thisfile");
	CU_TEST(lines[0] == 987);
	CU_TEST(strcmp(messages[0],"hello")==0);
	CU_TEST(sevs[0] == ASC_USER_ERROR);
	CU_TEST(filenames[1] == "otherfile");
	CU_TEST(lines[1] == 123);
	CU_TEST(strcmp(messages[1],"salaam")==0);
	CU_TEST(sevs[1] == ASC_USER_NOTE);

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"bonjour");
	CU_TEST(currmessage == 3);
	CU_TEST(filenames[1] == "otherfile");
	CU_TEST(lines[1] == 123);
	CU_TEST(sevs[1] == ASC_USER_NOTE);
	CU_TEST(strcmp(messages[1],"salaam")==0);
	CU_TEST(strcmp(messages[2],"bonjour")==0);
	CU_TEST(sevs[2] == ASC_PROG_ERR);
}

static void test_errortree(void){
	CU_TEST(NULL == TREECURRENT());		
	error_reporter_set_callback(reporter_test);

	// test a non-caching tree with one message
	currmessage = 0;
	error_reporter_tree_start(0);

	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam");
	CU_TEST(currmessage == 1);

	CU_TEST(NULL != TREECURRENT());		

	error_reporter_tree_end();

	CU_TEST(NULL == TREECURRENT());

	// test a caching tree with three messages
	currmessage = 0;
	error_reporter_tree_start(1);
	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","salaam1");
	error_reporter(ASC_USER_NOTE,"otherfile",111,"funcname","salaam2");
	error_reporter(ASC_USER_NOTE,"otherfile",333,"funcname","salaam3");
	CU_TEST(currmessage == 0);
	error_reporter_tree_end();
	CU_TEST(currmessage == 3);

	CU_TEST(strcmp(messages[0],"salaam1")==0);
	CU_TEST(strcmp(messages[1],"salaam2")==0);
	CU_TEST(strcmp(messages[2],"salaam3")==0);
	CU_TEST(TREECURRENT()==NULL);

	CONSOLE_DEBUG("----");

	// test a non-caching tree with three messages
	currmessage = 0;
	error_reporter_tree_start(0);
	error_reporter(ASC_USER_NOTE,"otherfile",123,"funcname","hola1");
	error_reporter(ASC_USER_NOTE,"otherfile",111,"funcname","hola2");
	error_reporter(ASC_USER_NOTE,"otherfile",333,"funcname","hola3");
	CU_TEST(currmessage == 3);
	CU_TEST(strcmp(messages[0],"hola1")==0);
	CU_TEST(strcmp(messages[1],"hola2")==0);
	CU_TEST(strcmp(messages[2],"hola3")==0);

	error_reporter_tree_end();
	CU_TEST(currmessage == 3);

	// test a message, then a non caching tree with a non-caching subtree
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_start(0);
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
	error_reporter_tree_t *t1 = TREECURRENT();
	error_reporter_tree_start(0);
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_USER_NOTE,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 5);
	error_reporter_tree_end();
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==t1);
	error_reporter_tree_end();
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==NULL);

#if 0 
	// test a message, then a caching tree with a non-caching subtree
	currmessage = 0;
	error_reporter(ASC_USER_NOTE,"f1",111,NULL,"allo0");
	CU_TEST(currmessage == 1)
	CU_TEST(strcmp(messages[0],"allo0")==0);
	error_reporter_tree_start(1);
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
	error_reporter_tree_t *t1 = TREECURRENT();
	error_reporter_tree_start(0);
	CU_TEST(currmessage == 4);
	CU_TEST(TREECURRENT()!=t1);
	error_reporter(ASC_USER_NOTE,"f3",555,"fn4","buongiorno");
	CU_TEST(currmessage == 5);
	error_reporter_tree_end();
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==t1);
	error_reporter_tree_end();
	CU_TEST(currmessage == 5);
	CU_TEST(TREECURRENT()==NULL);
#endif
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(error) \
	T(errortree)

REGISTER_TESTS_SIMPLE(utilities_error, TESTS)

