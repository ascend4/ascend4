/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
	CUnit tests for the ospath.c module.
	by John Pye.
*/

#include <ascend/general/ospath.h>

#include <test/common.h>

FilePathTestFn ospath_searchpath_testexists;

#define NDEBUG

#ifndef NDEBUG
# include <assert.h>
# define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG);fflush(stderr);fflush(stderr)
# define MC(CLR,MSG) color_on(stderr,CLR);fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG);color_off(stderr);fflush(stderr)
# define MM(MSG) MC(ASC_FG_MAGENTA,MSG)
# define X(VAR) fprintf(stderr,"%s:%d: (%s) %s=%s\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR);fflush(stderr)
# define C(VAR) fprintf(stderr,"%s:%d: (%s) %s=%c\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR);fflush(stderr)
# define V(VAR) fprintf(stderr,"%s:%d: (%s) %s=%d\n",__FILE__,__LINE__,__FUNCTION__,#VAR,(VAR));fflush(stderr)
# define D(VAR) fprintf(stderr,"%s:%d: (%s) %s=",__FILE__,__LINE__,__FUNCTION__,#VAR);ospath_debug(VAR);fflush(stderr)
# define DD(VAR) color_on(stderr,ASC_FG_BRIGHTBLUE);fprintf(stderr,"%s:%d: (%s)",__FILE__, __LINE__,__FUNCTION__);color_off(stderr);fprintf(stderr,"%s=",#VAR);ospath_debug(VAR);fflush(stderr)
#else
# include <assert.h>
# define M(MSG) ((void)0)
# define MC(CLR,MSG) ((void)0)
# define X(VAR) ((void)0)
# define C(VAR) ((void)0)
# define V(VAR) ((void)0)
# define D(VAR) ((void)0)
# define DD(VAR) ((void)0)
# define MM(VAR) ((void)0)
#endif

#ifndef MEMUSED
# define MEMUSED(N) CU_TEST(ascmeminuse()==(N))
#endif

/**
	This is a sample searchpath test function. Assumes the 'userdata' is a
	relative FilePath which is appended to path, and then if it matches
	the path \GTK\bin\johnpye\extfn, returns true. This is of
	course a fairly useless test function, so it's just for testing.
*/
int ospath_searchpath_testexists(struct FilePath *path,void *file){
	struct FilePath *fp, *fp1, *fp2;
	fp = (struct FilePath *)file;
	D(fp);
	fp1 = ospath_concat(path,fp);
	D(fp1);

#ifdef WINPATHS
	fp2 = ospath_new("c:\\GTK\\bin\\johnpye\\extfn");
#else
	fp2 = ospath_new("/GTK/bin/johnpye/extfn");
#endif

	char *t=ospath_str(fp1);
	MC(ASC_FG_BRIGHT,t);
	FREE(t);

	t=ospath_str(fp2);
	MC(ASC_FG_BRIGHTRED,t);
	FREE(t);

	if(ospath_cmp(fp1,fp2)==0){
		MC(ASC_FG_GREEN,"MATCH");
		ospath_free(fp1);
		ospath_free(fp2);
		return 1;
	}
	MC(ASC_FG_RED,"NO MATCH");
	ospath_free(fp1);
	ospath_free(fp2);
	return 0;
}


// switch to boldface for messages in 'main'
#undef D
#define D DD
#undef M
#define M MM

static void test_getparent(void){
	struct FilePath *fp1, *fp2, *fp3;

	//------------------------

	fp1 = ospath_new_from_posix("/usr/local/hello/");
	fp2 = ospath_getparent(fp1);
	fp3 = ospath_new_from_posix("/usr/local");

	D(fp1);
	D(fp2);
	D(fp3);
	CU_TEST(ospath_cmp(fp2,fp3)==0);
	if(ospath_cmp(fp2,fp3)==0)M("Passed 'getparent' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3);

	MEMUSED(0);
}
	//------------------------

static void test_cleanup(void){
	struct FilePath *fp1, *fp2;

	fp1 = ospath_new_from_posix("/usr/include/../local");
	fp2 = ospath_new_from_posix("/usr/local");

	D(fp1);
	D(fp2);
	CU_TEST(ospath_cmp(fp1,fp2)==0);
	M("Passed 'cleanup' test\n");

	ospath_free(fp1); ospath_free(fp2);
	MEMUSED(0);
}
	//------------------------

static void test_newfromposix(void){

	struct FilePath *fp1, *fp2;

	fp1 = ospath_new_from_posix("models/johnpye/extfn/extfntest");
	D(fp1);
	fp2 = ospath_new("models\\johnpye\\extfn\\extfntest");
	D(fp2);
	D(fp1);
	CU_TEST(ospath_cmp(fp1,fp2)==0);
	M("Passed 'new_from_posix' test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	MEMUSED(0);
}
	//------------------------

static void test_secondcleanup(void){
	struct FilePath *fp1, *fp2, *fp3;
	fp1 = ospath_new(".\\src/.\\images\\..\\\\movies\\");
	fp2 = ospath_new(".\\src\\movies");

	D(fp1);
	D(fp2);

	CU_TEST(ospath_cmp(fp1,fp2)==0);
	M("Passed mid-path '..' cleanup test\n");

	ospath_free(fp2);

	fp2 = ospath_new("./src/movies\\kubrick");
	fp3 = ospath_getparent(fp2);

	D(fp2);
	D(fp3);

	CU_TEST(ospath_cmp(fp1,fp3)==0);
	M("Passed 'second cleanup' test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);
	MEMUSED(0);
}
	//------------------------

static void test_append(void){

	struct FilePath *fp2, *fp3, *fp4;

	fp2 = ospath_new("\\home\\john");
	fp3 = ospath_new("where\\mojo");

	D(fp2);
	D(fp3);

	ospath_append(fp2,fp3);

	D(fp2);

	fp4 = ospath_new("\\home\\john\\where\\mojo\\");

	D(fp2);
	CU_TEST(ospath_cmp(fp2,fp4)==0);
	M("Passed 'append' test\n");

	ospath_free(fp3);
	ospath_free(fp2);
	ospath_free(fp4);
	MEMUSED(0);
}
	//---------------------------

static void test_appendupup(void){

	struct FilePath *fp2, *fp3, *fp4;

	fp3 = ospath_new_noclean("../..");
	D(fp3);

	// test with appending ../.. to an existing path
	fp2 = ospath_new("\\home\\john");
	M("ORIGINAL PATH");
	D(fp2);
	ospath_append(fp2,fp3);
	M("AFTER APPENDING ../..");
	D(fp2);

	M("GETTING ROOT");
	fp4 = ospath_root(fp2);
	M("ROOT FOUND:");
	D(fp4);

	CU_TEST(ospath_cmp(fp2,fp4)==0);
	M("Passed 'append ../..' test\n");

	ospath_free(fp2);
	ospath_free(fp3);
	ospath_free(fp4);
	MEMUSED(0);
}
	//-------------------------

static void test_up(void){

	struct FilePath *fp1, *fp2;

	fp1 = ospath_new("~\\somewhere\\..");
	fp2 = ospath_new("~/.");

	CU_TEST(ospath_cmp(fp1,fp2)==0);

	D(fp2);

	ospath_free(fp1);
	ospath_free(fp2);

	fp1 = ospath_new("/usr/local/include");
	fp2 = ospath_new("/usr/include/../local/include");

	D(fp1);
	D(fp2);

	CU_TEST(ospath_cmp(fp1,fp2)==0);
	M("Passed another mid-path '..' test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	MEMUSED(0);
}
	//---------------------------

static void test_concat(void){

	struct FilePath *fp1, *fp2, *fp3, *fp4;

	fp1 = ospath_new("/home");
	fp2 = ospath_new("john");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("/home/john");

	CU_TEST(ospath_cmp(fp3,fp4)==0);
	M("Passed 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);

	fp1 = ospath_new("c:/Program Files");
	fp2 = ospath_new("GnuWin32\\bin");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin");

	CU_TEST(ospath_cmp(fp3,fp4)==0);
	M("Passed 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);
	MEMUSED(0);
}
	//---------------------------

static void test_concatmixedslash(void){

	struct FilePath *fp1, *fp2, *fp3, *fp4;

	fp1 = ospath_new("c:/Program Files/");
	fp2 = ospath_new("GnuWin32\\bin");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin");

	CU_TEST(ospath_cmp(fp3,fp4)==0);
	M("Passed trailing-slash 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);
	MEMUSED(0);
}
	//---------------------------

static void test_trailingslash(void){

	struct FilePath *fp1, *fp2, *fp3, *fp4;

	fp1 = ospath_new("c:/Program Files/GnuWin32/bin");
	fp2 = ospath_new("johnpye/extfn");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin/johnpye/extfn");

	CU_TEST(ospath_cmp(fp3,fp4)==0);
	M("Passed trailing-slash 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);
	MEMUSED(0);
}
	//---------------------------

static void test_searchpath(void){

	struct FilePath *fp1, *fp2, *fp3;
	struct FilePath **pp, **p1;// will be returned null-terminated
#ifdef WINPATHS
	char pathtext[]="c:\\Program Files\\GnuWin32\\bin;c:\\GTK\\bin;e:\\ascend\\;..\\..\\pygtk";
#else
	char pathtext[]="\\Program Files\\GnuWin32\\bin:\\GTK\\bin:\\ascend\\:..\\..\\pygtk";
#endif

	pp = ospath_searchpath_new(pathtext);

	for(p1=pp; *p1!=NULL; ++p1){
		D(*p1);
	}

#ifdef WINPATHS
	fp1 = ospath_new("c:\\program files\\GnuWin32\\bin");
#else
	fp1 = ospath_new("\\Program Files\\GnuWin32\\bin");
#endif

	CU_TEST(pp[0]!=NULL);
	CU_TEST(fp1!=NULL);

	D(fp1);
	D(pp[0]);

	CU_TEST(ospath_cmp(pp[0],fp1)==0);


	fp2 = ospath_new_noclean("johnpye/extfn");
	D(fp2);

	fflush(stderr);

	fp3 = ospath_searchpath_iterate(pp,&ospath_searchpath_testexists,(void*)fp2);

	CU_TEST(fp3!=NULL);
	D(fp3);
	D(pp[1]);

	CU_TEST(ospath_cmp(fp3,pp[1])==0);
	M("Passed path-search test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_searchpath_free(pp);
	MEMUSED(0);
}
	//-------------------------------

static void test_searchpath2(void){

	struct FilePath *fp2, *fp3;
	struct FilePath **pp, **p1;// will be returned null-terminated
#ifdef WINPATHS
	char pathtext2[]="c:\\Program Files\\ASCEND\\models";
#else
	char pathtext2[]="/usr/local/ascend/models";
#endif

	M("Path-search test 2...");

	pp = ospath_searchpath_new(pathtext2);

	CU_TEST(pp!=NULL);

	for (p1=pp; *p1!=NULL; ++p1){
		D(*p1);
	}

	fp2 = ospath_new_noclean("johnpye/extfn/extfntest");
	D(fp2);

	fp3 = ospath_searchpath_iterate(pp,&ospath_searchpath_testexists,(void*)fp2);
	CU_TEST(fp3==NULL);

	M("Passed path-search test 2\n");

	ospath_free(fp2);
	ospath_free(fp3);
	ospath_searchpath_free(pp);
	MEMUSED(0);
}
	//-------------------------------

static void test_basefilename(void){

	struct FilePath *fp1;
	char *s1;

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test 2\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("/here/is/my/path.dir/");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	CU_TEST(NULL==s1);
	M("Passed getbasefilename test 3\n");

	ospath_free(fp1);
	if(s1)FREE(s1);

#ifdef WINPATHS
	fp1 = ospath_new("c:extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test WINPATHS\n");

	ospath_free(fp1);
	FREE(s1);
#endif
	MEMUSED(0);
}
	//-------------------------------

static void test_getfilestem(void){

	struct FilePath *fp1;
	char *s1;

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest.a4c");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"extfntest")==0);
	M("Passed getfilestem test\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"extfntest")==0);
	M("Passed getfilestem test 2\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("/usr/share/data/ascend/.ascend.ini");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(strcmp(s1,".ascend")==0);
	M("Passed getfilestem test 3\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("~/.vimrc");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(strcmp(s1,".vimrc")==0);
	M("Passed getfilestem test 3\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("~/src/ascend-0.9.5-1.jdpipe.src.rpm");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(strcmp(s1,"ascend-0.9.5-1.jdpipe.src")==0);
	M("Passed getfilestem test 4\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("~/dir1/dir2/");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	CU_TEST(NULL==s1);
	M("Passed getfilestem test 5\n");

	ospath_free(fp1);
	if(s1)FREE(s1);
	MEMUSED(0);
}
	//-------------------------------

static void test_getbasefileext(void){

	struct FilePath *fp1;
	char *s1;

	fp1 = ospath_new("~/src/ascend-0.9.5-1.jdpipe.src.rpm");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	CU_TEST(strcmp(s1,".rpm")==0);
	M("Passed getbasefileext test\n");

	ospath_free(fp1);
	FREE(s1);

	fp1 = ospath_new("~/.vimrc");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	CU_TEST(s1==NULL);
	M("Passed getbasefileext test 2\n");

	ospath_free(fp1);
	if(s1)FREE(s1);

	fp1 = ospath_new("./ascend4");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	CU_TEST(s1==NULL);
	M("Passed getbasefileext test 3\n");

	ospath_free(fp1);
	if(s1)FREE(s1);
	MEMUSED(0);
}
	//-------------------------------

static void test_getdir(void){

	struct FilePath *fp1, *fp2, *fp3;

	fp1 = ospath_new("/home/myfile");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home");
	CU_TEST(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);

	fp1 = ospath_new("/home/myfile.ext");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home");
	CU_TEST(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test 2\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);

	fp1 = ospath_new("/home/mydir/");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home/mydir");
	CU_TEST(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test 3\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);
	MEMUSED(0);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(getparent) \
	T(cleanup) \
	T(newfromposix) \
	T(secondcleanup) \
	T(append) \
	T(appendupup) \
	T(up) \
	T(concat) \
	T(concatmixedslash) \
	T(trailingslash) \
	T(searchpath) \
	T(searchpath2) \
	T(basefilename) \
	T(getfilestem) \
	T(getbasefileext) \
	T(getdir)

REGISTER_TESTS_SIMPLE(general_ospath, TESTS);

