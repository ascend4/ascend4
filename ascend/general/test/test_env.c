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

#include <ascend/general/env.h>
#include <test/common.h>

/*--------------------------------
	some simple test routines...
*/

# define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG);fflush(stderr)

/* 
	return NULL for unfound env vars, else point to a string that must not be
	modified by the caller, and may later be changed by a later call to getenv.
*/
static char *my_getenv(const char *name){
	if(strcmp(name,"MYHOME")==0){
		return "/home/john";
	}else if(strcmp(name,"MYBIN")==0){
		return "/usr/local/bin";
	}
	return NULL;
}

// simple 'env' implementation for the purpose of testing

typedef struct MyEnvList_struct{
	char *key;
	char *val;
	struct MyEnvList_struct *next;
} MyEnvList;

MyEnvList *myenv2 = NULL;

static char *my2_getenv(const char *name){
	MyEnvList *m = myenv2;
	while(m!=NULL){
		if(strcmp(m->key,name)==0 && m->val!=NULL)
			return m->val;
		m = m->next;
	}
	return NULL;
}

static int my2_putenv(const char *s){
	char *eq = strchr(s,'=');
	if(eq==NULL)return 1;
	//M(s);
	//M(eq);
	char *k = ASC_NEW_ARRAY(char,(eq-s+1));
	if(!k)return 2;
	strncpy(k, s, eq-s);
	k[eq-s]='\0';
	char *v = ASC_NEW_ARRAY(char,(size_t)(strlen(s) - (eq-s)));
	if(!v){
		ASC_FREE(k);
		return 3;
	}
	strncpy(v, eq+1, strlen(s) - (eq-s));
	//M(k);
	//M(v);
	MyEnvList **m = &myenv2;
	while(*m!=NULL){
		if(strcmp((*m)->key,k)==0){
			// replace existing value in list
			ASC_FREE(k);
			if((*m)->val!=NULL)ASC_FREE((*m)->val);
			(*m)->val = v;
			return 0;
		}
		m = &((*m)->next);
	}
	*m = ASC_NEW(MyEnvList);
	// add to end of list
	(*m)->key = k;
	(*m)->val = v;
	(*m)->next = NULL;
	return 0;
}

void my2_envclean(void){
	MyEnvList *m = myenv2;
	while(m){
		char *k = m->key; 
		char *v = m->val;
		MyEnvList *n = m->next; 
		if(k)ASC_FREE(k);
		if(v)ASC_FREE(v);
		ASC_FREE(m);
		m = n;
	}
	myenv2 = NULL;
}

void test_subst(void){
	char s1[]="$MYHOME/bitmaps";
	char *r;

	M(s1);

	r = env_subst(s1,NULL);
	CU_TEST(strcmp(r,s1)==0);
	ASC_FREE(r);

	r = env_subst(s1,my_getenv);
	M(r);
	CU_TEST(strcmp(r,"/home/john/bitmaps")==0);
	ASC_FREE(r);

	r = env_subst("$MYHOME",my_getenv);
	M(r);
	CU_TEST(0==strcmp(r,"/home/john"));
	ASC_FREE(r);

	r = env_subst("$MYHOME.",my_getenv);
	M(r);
	CU_TEST(0==strcmp(r,"/home/john."));
	ASC_FREE(r);

	r = env_subst("$MISSING",my_getenv);
	M(r);
	CU_TEST(0==strcmp(r,""));
	ASC_FREE(r);

	r = env_subst("$MISSING#",my_getenv);
	M(r);
	CU_TEST(0==strcmp(r,"#"));
	ASC_FREE(r);

	r = env_subst("$MISSING:$MYHOME",my_getenv);
	M(r);
	CU_TEST(0==strcmp(r,":/home/john"));
	ASC_FREE(r);

	env_import("MYHOME",my_getenv,my2_putenv);
	my2_putenv("MYEXE=$MYHOME/myexe");
	r = env_subst("$MYEXE --version",my2_getenv);
	M(r);
	CU_TEST(0==strcmp(r,"/home/john/myexe --version"));
	ASC_FREE(r);
	my2_envclean();

	/* test where nested substitution is null */
	my2_putenv("MYEXE=$MYHOME/myexe"); // MYHOME not set
	r = env_subst("$MYEXE --version",my2_getenv);
	M(r);
	CU_TEST(0==strcmp(r,"/myexe --version"));
	ASC_FREE(r);
	my2_envclean();
}

void test_putenv(void){
	CU_TEST(my2_getenv("MYHOME")==NULL);
	my2_putenv("MYHOME=aabbcc");
	CU_TEST(my2_getenv("MYHOME")!=NULL);
	if(my2_getenv("MYHOME")!=NULL){
		CU_TEST(strcmp(my2_getenv("MYHOME"),"aabbcc")==0);
	}
	CU_TEST(my2_getenv("MYBIN")==NULL);
	my2_putenv("MYBIN=/usr/local/bin");
	my2_putenv("CMD=export NAME=VALUE");
	CU_TEST(my2_getenv("MYHOME")!=NULL && strcmp(my2_getenv("MYHOME"),"aabbcc")==0);
	CU_TEST(my2_getenv("MISSING")==NULL);
	my2_putenv("MYHOME=ccc");
	CU_TEST(my2_getenv("MYHOME")!=NULL && strcmp(my2_getenv("MYHOME"),"ccc")==0);
	CU_TEST(my2_getenv("CMD")!=NULL && strcmp(my2_getenv("CMD"),"export NAME=VALUE")==0);

	my2_envclean();
}	

void test_import(void){
	char *h = my_getenv("MYHOME");
	CU_TEST(h != NULL);
	CU_TEST(0==env_import("MYHOME",my_getenv,my2_putenv));
	CU_TEST(0!=env_import("MISSING",my_getenv,my2_putenv));

	CU_TEST(my2_getenv("MYHOME")!=NULL && strcmp(my2_getenv("MYHOME"),h)==0);

	my2_envclean();
}

void test_import_default(void){
	char *h1 = my_getenv("MYHOME");
	CU_TEST(h1 != NULL);
	char *h2 = my_getenv("MISSING");
	CU_TEST(h2 == NULL);

	/* test env_import_default */

	CU_TEST(0==env_import_default("MYHOME",my_getenv,my2_putenv,"UNUSEDSTRING"));
	CU_TEST(0==env_import_default("MISSING",my_getenv,my2_putenv,"DEFAULTVAL"));

	CU_TEST(my2_getenv("MYHOME")!=NULL && 0==strcmp(my2_getenv("MYHOME"),h1));
	CU_TEST(my2_getenv("MISSING")!=NULL)
	CU_TEST(0==strcmp(my2_getenv("MISSING"),"DEFAULTVAL"));

	/* env_import_default, overwriting values in the my2 env */
	CU_TEST(0==my2_putenv("MYHOME=SOMETHING"));
	CU_TEST(my2_getenv("MYHOME")!=NULL && 0==strcmp(my2_getenv("MYHOME"),"SOMETHING"));

	CU_TEST(0==env_import_default("MYHOME",my_getenv,my2_putenv,"ALSOUNUSED"));
	CU_TEST(0==env_import_default("MISSING2",my_getenv,my2_putenv,"SECONDDEFAULT"));

	CU_TEST(my2_getenv("MYHOME")!=NULL && 0==strcmp(my2_getenv("MYHOME"),h1));
	CU_TEST(my2_getenv("MISSING2")!=NULL)
	CU_TEST(0==strcmp(my2_getenv("MISSING2"),"SECONDDEFAULT"));

	my2_envclean();
}
/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(subst) \
	T(putenv) \
	T(import) \
	T(import_default)

REGISTER_TESTS_SIMPLE(general_env, TESTS);

