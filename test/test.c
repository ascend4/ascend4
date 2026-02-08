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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	Test runner for the 'base/generic' routines in ASCEND
*/
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#endif

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/compiler/redirectFile.h>
#include <ascend/general/ascMalloc.h>

#include "printutil.h"
#include "test_globals.h"
#include <ascend/general/ospath.h>

#include <CUnit/Basic.h>
#include <CUnit/TestRun.h>

#ifdef __WIN32__
# include <windows.h>
#endif

extern int register_cunit_tests();

static int g_capture_enabled = 1;

struct strlist {
	char **items;
	int len;
	int cap;
};

static void strlist_append_unique(struct strlist *list, const char *s){
	for(int i = 0; i < list->len; ++i){
		if(strcmp(list->items[i], s) == 0){
			return;
		}
	}
	if(list->len == list->cap){
		int next = list->cap ? list->cap * 2 : 16;
		char **items = realloc(list->items, sizeof(char *) * next);
		if(items == NULL){
			ascshutdown("Out of memory while expanding test list");
			return;
		}
		list->items = items;
		list->cap = next;
	}
	list->items[list->len] = strdup(s);
	if(list->items[list->len] == NULL){
		ascshutdown("Out of memory while recording test name");
		return;
	}
	list->len++;
}

static void strlist_free(struct strlist *list){
	for(int i = 0; i < list->len; ++i){
		free(list->items[i]);
	}
	free(list->items);
	list->items = NULL;
	list->len = 0;
	list->cap = 0;
}

static int has_glob_chars(const char *s){
	return strpbrk(s, "*?[") != NULL;
}

static int match_pattern(const char *pattern, const char *text){
#ifdef HAVE_FNMATCH
	return fnmatch(pattern, text, 0) == 0;
#else
	return strcmp(pattern, text) == 0;
#endif
}

static void expand_pattern(const char *pattern, struct strlist *out){
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg ? reg->pSuite : NULL;
	int matched = 0;
	if(strchr(pattern, '.') != NULL){
		for(; suite != NULL; suite = suite->pNext){
			struct CU_Test *test = suite->pTest;
			for(; test != NULL; test = test->pNext){
				char namebuf[1024];
				snprintf(namebuf, sizeof(namebuf), "%s.%s", suite->pName, test->pName);
				if(match_pattern(pattern, namebuf)){
					strlist_append_unique(out, namebuf);
					matched = 1;
				}
			}
		}
	} else {
		for(; suite != NULL; suite = suite->pNext){
			if(match_pattern(pattern, suite->pName)){
				strlist_append_unique(out, suite->pName);
				matched = 1;
			}
		}
	}
	if(!matched){
		strlist_append_unique(out, pattern);
	}
}

static char *trim_line(char *s){
	while(isspace((unsigned char)*s)) s++;
	char *end = s + strlen(s);
	while(end > s && isspace((unsigned char)end[-1])) end--;
	*end = '\0';
	return s;
}

static int has_tests_ext(const char *name){
	const char *dot = strrchr(name, '.');
	return dot && strcmp(dot, ".tests") == 0;
}

static void read_tests_file(const char *path, struct strlist *out){
	FILE *fp = fopen(path, "r");
	if(!fp){
		fprintf(stderr, "Unable to open test list file '%s'\n", path);
		return;
	}
	char *line = NULL;
	size_t cap = 0;
	while(getline(&line, &cap, fp) != -1){
		char *t = trim_line(line);
		if(t[0] == '\0' || t[0] == '#'){
			continue;
		}
		if(has_glob_chars(t)){
			expand_pattern(t, out);
		}else{
			strlist_append_unique(out, t);
		}
	}
	free(line);
	fclose(fp);
}

static void expand_tests_file(const char *name, struct strlist *out){
	char filename[PATH_MAX];
	if(has_tests_ext(name)){
		snprintf(filename, sizeof(filename), "%s", name);
	}else{
		snprintf(filename, sizeof(filename), "%s.tests", name);
	}

	if(access(filename, R_OK) == 0){
		read_tests_file(filename, out);
		return;
	}

	char altpath[PATH_MAX];
	snprintf(altpath, sizeof(altpath), "%s/%s", ASC_TEST_PATH, filename);
	if(access(altpath, R_OK) == 0){
		read_tests_file(altpath, out);
		return;
	}

	fprintf(stderr, "Unable to locate test list file '%s' (tried '%s' and '%s')\n", name, filename, altpath);
}


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

char ASC_TEST_PATH[PATH_MAX];

/**
	Main routine, handles command line options
*/
int main(int argc, char* argv[]){
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_IGNORE;
	CU_ErrorCode result = 0;
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
		{"run",        required_argument, 0, 'r'},
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
		"    --verbose, -v   full output (no suppression)\n"
		"    --silent, -s\n"
		"    --normal, -n\n"
		"    --run=FILE, -r FILE  run tests listed in FILE (.tests extension optional)\n"
		"    --on-error=[fail|abort|ignore], -e\n"
		"    --help\n"
		"    --list-suites, -l\n"
		"    --list-tests=SUITENAME, -tSUITENAME\n"
	;

	char c;
	const char *runfile = NULL;
	while(-1 != (c = getopt_long (argc, argv, "vsnr:e:t:l", long_options, &option_index))){
		switch(c){
			case 'v': mode = CU_BRM_VERBOSE; g_capture_enabled = 0; break;
			case 's': mode = CU_BRM_SILENT; break;
			case 'n': mode = CU_BRM_NORMAL; break;
			case 'r': runfile = optarg; break;
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
	CU_set_test_output_capture(g_capture_enabled);

	if(list){
		if(strlen(suitename)){
			list_tests(suitename);
		}else{
			list_suites();
		}
		goto cleanup;
	}

	if(runfile != NULL){
		struct strlist expanded = {0};
		expand_tests_file(runfile, &expanded);
		if(expanded.len > 0){
			result = CU_basic_run_selected_tests(expanded.len, expanded.items);
		}else{
			result = CU_basic_run_selected_tests(0, NULL);
		}
		strlist_free(&expanded);
	}else if(optind < argc){
		int selected = argc - optind;
		char **selected_argv = &argv[optind];
		int needs_expand = 0;
		for(int i = 0; i < selected; ++i){
			if(has_glob_chars(selected_argv[i])){
				needs_expand = 1;
				break;
			}
		}

		if(needs_expand){
			struct strlist expanded = {0};
			for(int i = 0; i < selected; ++i){
				if(has_glob_chars(selected_argv[i])){
					expand_pattern(selected_argv[i], &expanded);
				} else {
					strlist_append_unique(&expanded, selected_argv[i]);
				}
			}
			if(expanded.len > 0){
				result = CU_basic_run_selected_tests(expanded.len, expanded.items);
			} else {
				result = CU_basic_run_selected_tests(selected, selected_argv);
			}
			strlist_free(&expanded);
		} else {
			result = CU_basic_run_selected_tests(selected, selected_argv);
		}
	}else{
		result = CU_basic_run_tests();
	}

cleanup:
	if(mode == CU_BRM_VERBOSE){
		ascshutdown("Testing completed.");/* shut down memory manager */
	}
	CU_cleanup_registry();
	return result;
}
