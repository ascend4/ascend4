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

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <compiler/redirectFile.h>
#include <utilities/ascMalloc.h>
#include <printutil.h>

#include <CUnit/Basic.h>

ASC_IMPORT int register_cunit_tests();

/*
	The following allows the CUnit tests to be run using a standalone executable
	using the CUnit 'basic' interface.
*/
int run_suite_or_test(char *name){
	char suitename[1000];
	char *s,*n;
	for(s=suitename,n=name; *n!='.' && *n!='\0' && s < suitename+999; *s++=*n++);
	*s='\0';
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg->pSuite;
	struct CU_Test *test;
	if(suite==NULL){
		fprintf(stderr,"No suites present in registry!\n");
		return CUE_NO_SUITENAME;
	}

	CU_ErrorCode result;
	while(suite!=NULL){
		fprintf(stderr,"Looking at suite %s\n", suite->pName);
		if(0==strcmp(suite->pName,suitename)){
			fprintf(stderr,"Found suite %s\n", suitename);
			if(*n=='.'){
				++n;
				fprintf(stderr,"Looking for test %s\n", n);
				test = suite->pTest;
				while(test!=NULL){
					fprintf(stderr,"Found test %s\n", test->pName);
					if(0==strcmp(test->pName,n)){
						fprintf(stderr,"Running test %s (%p, %p)\n", n,suite,test);
						result = CU_basic_run_test(suite,test);
						fprintf(stderr,"Result: %s\n",CU_get_error_msg());
						return result;
					}
					test = test->pNext;
				}
				return CUE_NO_TESTNAME;
			}else{
				fprintf(stderr,"Running suite %s (%p)\n",suitename,suite);
				result = CU_basic_run_suite(suite);
				fprintf(stderr,"Result: %s\n",CU_get_error_msg());
				return result;
			}
		}
		suite = suite->pNext;
	}
	return CUE_NO_SUITENAME;
};

/**
	Main routine, handles command line options
*/
int main(int argc, char* argv[]){
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_IGNORE;
	CU_ErrorCode result;

	static struct option long_options[] = {
		{"on-error", required_argument, 0, 'e'},
		{"verbose",  no_argument,       0, 'v'},
		{"silent",   no_argument,       0, 's'},
		{"normal",   no_argument,       0, 'n'},
		{"help",     no_argument,       0, '?'},
		{"usage",    no_argument,       0, '?'},
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
		"    --help\n";

	char c;
	while(-1 != (c = getopt_long (argc, argv, "vsne:", long_options, &option_index))){
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
					exit(1);
				}
				break;
			case '?':
			case 'h':
				fprintf(stderr,usage,argv[0]);
				exit(1);
			default:
				fprintf(stderr,"Unknown option -- '%c'", c);
				fprintf(stderr,usage,argv[0]);
				exit(2);
		}
	}

	CU_initialize_registry();
	register_cunit_tests();
	CU_basic_set_mode(mode);
	CU_set_error_action(error_action);

	/* any remaining command-line arguments will be specific test suites and/or tests to run */
	if(optind < argc){
		while(optind < argc){
			result = run_suite_or_test(argv[optind]);
			if(result==CUE_NO_SUITENAME){
				fprintf(stderr,"Invalid suite name '%s'\n", argv[optind]);
				exit(1);
			}else if(result==CUE_NO_TESTNAME){
				fprintf(stderr,"Invalid test name '%s'\n", argv[optind]);
				exit(1);
			}
			optind++;
		}
	}else{
		result = CU_basic_run_tests();
	}

	CU_cleanup_registry();

	if(mode == CU_BRM_VERBOSE)ascshutdown("Testing completed.");/* shut down memory manager */

	return result;
}
