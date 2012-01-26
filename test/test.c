/*	ASCEND modelling environment
	Copyright (C) 2005 Jerry St.Clair
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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	Test runner for the 'base/generic' routines in ASCEND
*/
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/compiler/redirectFile.h>
#include <ascend/general/ascMalloc.h>

#include "printutil.h"
#include "test_globals.h"

#include <ascend/general/ospath.h>

#include <CUnit/Basic.h>

#ifdef __WIN32__
# include <windows.h>
#endif

extern int register_cunit_tests();

extern char ASC_TEST_PATH[PATH_MAX];

int list_suites(){
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg->pSuite;
	fprintf(stderr,"Test suites found in registry:\n");
	while(suite!=NULL){
		fprintf(stderr,"\t%s\n", suite->pName);
		suite = suite->pNext;
	}
	return CUE_NO_SUITENAME;
}

int list_tests(const char *suitename0){
	char suitename[1000];
	char *s;
	const char *n;
	/* locate the '.' separator and copy bits before that into suitename. */
	for(s=suitename,n=suitename0; *n!='.' && *n!='\0' && s < suitename+999; *s++=*n++);
	*s='\0';

	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg->pSuite;
	struct CU_Test *test;
	while(suite!=NULL){
		if(0==strcmp(suite->pName,suitename)){
			fprintf(stderr,"Tests found in suite '%s':\n",suitename);
			test = suite->pTest;
			while(test!=NULL){
				fprintf(stderr,"\t%s\n", test->pName);
				test = test->pNext;
			}
			return CUE_NO_TESTNAME;
		}
		suite = suite->pNext;
	}
	fprintf(stderr,"Test suite '%s' not found in registry.\n",suitename);
	return CUE_NO_SUITENAME;
}

/**
	Main routine, handles command line options
*/
int main(int argc, char* argv[]){
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_IGNORE;
	CU_ErrorCode result;
	char suitename[1000];
	char list = 0;

#ifdef __WIN32__
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif

	struct FilePath *test_executable = ospath_new(argv[0]);
	struct FilePath *test_dir = ospath_getdir(test_executable); /** Global Variable containing Path information about the test directory */
	ospath_strncpy(test_dir,ASC_TEST_PATH,PATH_MAX);
	ospath_free(test_dir);
	ospath_free(test_executable);

	static struct option long_options[] = {
		{"on-error",   required_argument, 0, 'e'},
		{"verbose",    no_argument,       0, 'v'},
		{"silent",     no_argument,       0, 's'},
		{"normal",     no_argument,       0, 'n'},
		{"help",       no_argument,       0, '?'},
		{"usage",      no_argument,       0, '?'},
		{"list-suites",no_argument,       0, 'l'},
		{"list-tests", required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	/* getopt_long stores the option index here. */
	int option_index = 0;

	const char *usage =
		"%s -vsne [SuiteName|SuiteName.testname] ...\n"
		"Test ASCEND base/generic routines\n"
		"options:\n"
		"    --verbose, -v   full output, including memory checking\n"
		"    --silent, -s\n"
		"    --normal, -n\n"
		"    --on-error=[fail|abort|ignore], -e\n"
		"    --help\n"
		"    --list-suites, -l\n"
		"    --list-tests=SUITENAME, -tSUITENAME\n"
	;

	char c;
	while(-1 != (c = getopt_long (argc, argv, "vsne:t:l", long_options, &option_index))){
		switch(c){
			case 'v': mode = CU_BRM_VERBOSE; break;
			case 's': mode = CU_BRM_SILENT; break;
			case 'n': mode = CU_BRM_NORMAL; break;
			case 'e':
				if(0==strcmp(optarg,"fail")){
					fprintf(stderr,"on error FAIL\n");
					error_action = CUEA_FAIL;
				}else if(0==strcmp(optarg,"abort")){
					fprintf(stderr,"on error ABORT\n");
					error_action = CUEA_ABORT;
					break;
				}else if(0==strcmp(optarg,"ignore")){
					error_action = CUEA_IGNORE;
				}
				else{
					fprintf(stderr,"Invalid argument for --on-error option!\n");
					result = 1;
					goto cleanup;
				}
				break;
			case 'l':
				list = 1;
				suitename[0] = '\0';
				break;
			case 't':
				list = 1;
				strncpy(suitename, optarg, 999);
				break;
			case '?':
			case 'h':
				fprintf(stderr,usage,argv[0]);
				result = 1;
				goto cleanup;
			default:
				fprintf(stderr,"Unknown option -- '%c'", c);
				fprintf(stderr,usage,argv[0]);
				result = 2;
				goto cleanup;
		}
	}

	CU_initialize_registry();
	register_cunit_tests();
	CU_basic_set_mode(mode);
	CU_set_error_action(error_action);

	if(list){
		if(strlen(suitename)){
			list_tests(suitename);
		}else{
			list_suites();
		}
		goto cleanup;
	}

	if(optind < argc){
		result = CU_basic_run_selected_tests(argc - optind, &argv[optind]);
	}else{
		result = CU_basic_run_tests();
	}

cleanup:
	if(mode == CU_BRM_VERBOSE)ascshutdown("Testing completed.");/* shut down memory manager */
	CU_cleanup_registry();
	return result;
}
