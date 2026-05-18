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

#include <ascend/utilities/config.h>

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include <ascend/general/glob.h>
#endif

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

enum op_type {
	OP_ADD = 0,
	OP_REMOVE,
	OP_FILE
};

struct op {
	enum op_type type;
	char *arg;
};

struct oplist {
	struct op *items;
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

static void strlist_remove_value(struct strlist *list, const char *s){
	for(int i = 0; i < list->len; ){
		if(strcmp(list->items[i], s) == 0){
			free(list->items[i]);
			for(int j = i + 1; j < list->len; ++j){
				list->items[j - 1] = list->items[j];
			}
			list->len--;
			continue;
		}
		i++;
	}
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

static void oplist_append(struct oplist *list, enum op_type type, const char *arg){
	if(list->len == list->cap){
		int next = list->cap ? list->cap * 2 : 16;
		struct op *items = realloc(list->items, sizeof(struct op) * next);
		if(items == NULL){
			ascshutdown("Out of memory while expanding op list");
			return;
		}
		list->items = items;
		list->cap = next;
	}
	list->items[list->len].type = type;
	list->items[list->len].arg = strdup(arg);
	if(list->items[list->len].arg == NULL){
		ascshutdown("Out of memory while recording op argument");
		return;
	}
	list->len++;
}

static void oplist_free(struct oplist *list){
	for(int i = 0; i < list->len; ++i){
		free(list->items[i].arg);
	}
	free(list->items);
	list->items = NULL;
	list->len = 0;
	list->cap = 0;
}

#ifndef HAVE_FNMATCH
static int has_unsupported_glob(const char *s){
	return strpbrk(s, "?[") != NULL;
}
#endif

static int match_pattern(const char *pattern, const char *text){
#ifdef HAVE_FNMATCH
	return fnmatch(pattern, text, 0) == 0;
#else
	return asc_glob_match(pattern, text, NULL);
#endif
}

static void expand_pattern(const char *pattern, struct strlist *out){
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg ? reg->pSuite : NULL;
	int matched = 0;
#ifndef HAVE_FNMATCH
	if(has_unsupported_glob(pattern)){
		fprintf(stderr, "Glob pattern '%s' uses unsupported characters (only '*' is available without fnmatch)\n", pattern);
		strlist_append_unique(out, pattern);
		return;
	}
#endif
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

static void expand_pattern_suites_only(const char *pattern, struct strlist *out, int allow_unmatched){
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg ? reg->pSuite : NULL;
	int matched = 0;
#ifndef HAVE_FNMATCH
	if(has_unsupported_glob(pattern)){
		fprintf(stderr, "Glob pattern '%s' uses unsupported characters (only '*' is available without fnmatch)\n", pattern);
		return;
	}
#endif
	if(strchr(pattern, '.') != NULL){
		return;
	}
	for(; suite != NULL; suite = suite->pNext){
		if(match_pattern(pattern, suite->pName)){
			strlist_append_unique(out, suite->pName);
			matched = 1;
		}
	}
	if(!matched && allow_unmatched){
		strlist_append_unique(out, pattern);
	}
}

static void apply_add_pattern(const char *pattern, struct strlist *out){
	struct strlist expanded = {0};
	expand_pattern(pattern, &expanded);
	for(int i = 0; i < expanded.len; ++i){
		strlist_append_unique(out, expanded.items[i]);
	}
	strlist_free(&expanded);
}

static void apply_remove_pattern(const char *pattern, struct strlist *out){
	struct strlist expanded = {0};
	expand_pattern(pattern, &expanded);
	for(int i = 0; i < expanded.len; ++i){
		strlist_remove_value(out, expanded.items[i]);
	}
	strlist_free(&expanded);
}

static void apply_add_pattern_suites_only(const char *pattern, struct strlist *out){
	struct strlist expanded = {0};
	expand_pattern_suites_only(pattern, &expanded, 0);
	for(int i = 0; i < expanded.len; ++i){
		strlist_append_unique(out, expanded.items[i]);
	}
	strlist_free(&expanded);
}

static void apply_remove_pattern_suites_only(const char *pattern, struct strlist *out){
	struct strlist expanded = {0};
	expand_pattern_suites_only(pattern, &expanded, 0);
	for(int i = 0; i < expanded.len; ++i){
		strlist_remove_value(out, expanded.items[i]);
	}
	strlist_free(&expanded);
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

static int parse_except_line(const char *line, char *out, size_t outsz){
	const char *p = line;
	if(strncmp(p, "--except", 8) == 0){
		p += 8;
	}else if(strncmp(p, "-e", 2) == 0){
		p += 2;
	}else{
		return 0;
	}
	while(isspace((unsigned char)*p)) p++;
	if(*p == '\0'){
		return -1;
	}
	strncpy(out, p, outsz - 1);
	out[outsz - 1] = '\0';
	return 1;
}

static int read_tests_file(const char *path, struct strlist *out){
	FILE *fp = fopen(path, "r");
	if(!fp){
		fprintf(stderr, "Unable to open test list file '%s'\n", path);
		return 0;
	}
	char line[1024];
	while(fgets(line, sizeof(line), fp) != NULL){
		size_t len = strlen(line);
		if(len == sizeof(line) - 1 && line[len - 1] != '\n'){
			fprintf(stderr, "Test list line too long in '%s' (max %zu chars)\n", path, sizeof(line) - 2);
			int ch;
			while((ch = fgetc(fp)) != '\n' && ch != EOF){;}
			continue;
		}
		char *t = trim_line(line);
		if(t[0] == '\0' || t[0] == '#'){
			continue;
		}
		char exceptbuf[1024];
		int except = parse_except_line(t, exceptbuf, sizeof(exceptbuf));
		if(except == -1){
			fprintf(stderr, "Invalid --except line in '%s'\n", path);
			fclose(fp);
			return 0;
		}
		if(except == 1){
			char *p = trim_line(exceptbuf);
			if(p[0] == '-'){
				fprintf(stderr, "Invalid excluded test name '%s' in '%s'\n", p, path);
				fclose(fp);
				return 0;
			}
			apply_remove_pattern(p, out);
			continue;
		}
		if(t[0] == '-'){
			fprintf(stderr, "Invalid test entry '%s' in '%s'\n", t, path);
			fclose(fp);
			return 0;
		}
		apply_add_pattern(t, out);
	}
	fclose(fp);
	return 1;
}

static int expand_tests_file(const char *name, struct strlist *out){
	char filename[PATH_MAX];
	if(has_tests_ext(name)){
		snprintf(filename, sizeof(filename), "%s", name);
	}else{
		snprintf(filename, sizeof(filename), "%s.tests", name);
	}

	if(access(filename, R_OK) == 0){
		return read_tests_file(filename, out);
	}

	char altpath[PATH_MAX];
	int needed = snprintf(altpath, sizeof(altpath), "%s/%s", ASC_TEST_PATH, filename);
	if(needed < 0 || (size_t)needed >= sizeof(altpath)){
		fprintf(stderr, "Test list path too long: '%s/%s'\n", ASC_TEST_PATH, filename);
		return 0;
	}
	if(access(altpath, R_OK) == 0){
		return read_tests_file(altpath, out);
	}

	fprintf(stderr, "Unable to locate test list file '%s' (tried '%s' and '%s')\n", name, filename, altpath);
	return 0;
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

int list_suites_filtered(struct strlist *patterns){
	struct CU_TestRegistry *reg = CU_get_registry();
	struct CU_Suite *suite = reg->pSuite;
	fprintf(stderr,"Test suites found in registry:\n");
	while(suite!=NULL){
		int match = 0;
		for(int i = 0; i < patterns->len; ++i){
			if(match_pattern(patterns->items[i], suite->pName)){
				match = 1;
				break;
			}
		}
		if(match){
			fprintf(stderr,"\t%s\n", suite->pName);
		}
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

static void list_failed_tests(void){
	CU_pFailureRecord failure = CU_get_failure_list();
	unsigned int i = 1;

	if(failure == NULL){
		fprintf(stdout,"\nNo failures.\n");
		return;
	}

	fprintf(stdout,"\n--------------- Test Run Failures -------------------------\n");
	fprintf(stdout,"   src_file:line# : (suite:test) : failure_condition\n");
	for(; failure != NULL; failure = failure->pNext, ++i){
		const char *file = failure->strFileName != NULL ? failure->strFileName : "";
		const char *suite = (failure->pSuite != NULL && failure->pSuite->pName != NULL) ? failure->pSuite->pName : "";
		const char *test = (failure->pTest != NULL && failure->pTest->pName != NULL) ? failure->pTest->pName : "";
		const char *condition = failure->strCondition != NULL ? failure->strCondition : "";
		fprintf(stdout,"\n%u. %s:%u : (%s : %s) : %s",i,file,failure->uiLineNumber,suite,test,condition);
	}
	fprintf(stdout,"\n-----------------------------------------------------------\n");
	fprintf(stdout,"Total Number of Failures : %-u\n",i - 1);
}

static void list_skipped_tests(void){
	CU_pSkipRecord skip = CU_get_skip_list();
	unsigned int i = 1;

	if(skip == NULL){
		fprintf(stdout,"\nNo skipped tests.\n");
		return;
	}

	fprintf(stdout,"\n--------------- Test Run Skips ----------------------------\n");
	fprintf(stdout,"   src_file:line# : (suite:test) : skip_reason\n");
	for(; skip != NULL; skip = skip->pNext, ++i){
		const char *file = skip->strFileName != NULL ? skip->strFileName : "";
		const char *suite = (skip->pSuite != NULL && skip->pSuite->pName != NULL) ? skip->pSuite->pName : "";
		const char *test = (skip->pTest != NULL && skip->pTest->pName != NULL) ? skip->pTest->pName : "";
		const char *reason = skip->strReason != NULL ? skip->strReason : "";
		fprintf(stdout,"\n%u. %s:%u : (%s : %s) : %s",i,file,skip->uiLineNumber,suite,test,reason);
	}
	fprintf(stdout,"\n-----------------------------------------------------------\n");
	fprintf(stdout,"Total Number of Skips : %-u\n",i - 1);
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
	int list_failures = 0;
	int list_skipped = 0;
	int ran_tests = 0;

#ifdef __WIN32__
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif

	struct FilePath *test_executable = ospath_new(argv[0]);
	struct FilePath *test_dir = ospath_getdir(test_executable); /** Global Variable containing Path information about the test directory */
	ospath_strncpy(test_dir,ASC_TEST_PATH,PATH_MAX);
	ospath_free(test_dir);
	ospath_free(test_executable);

	static struct option long_options[] = {
		{"on-error",   required_argument, 0, 0},
		{"verbose",    no_argument,       0, 'v'},
		{"silent",     no_argument,       0, 's'},
		{"normal",     no_argument,       0, 'n'},
		{"except",     required_argument, 0, 'e'},
		{"run",        required_argument, 0, 'r'},
		{"help",       no_argument,       0, '?'},
		{"usage",      no_argument,       0, '?'},
		{"list-suites", no_argument,      0, 'l'},
		{"list-tests",  required_argument,0, 't'},
		{"list-failures",no_argument,     0, 0},
		{"list-skipped",no_argument,      0, 0},
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
		"    --except, -e   remove tests from the selection (can be used multiple times)\n"
		"    --on-error=[fail|abort|ignore]\n"
		"    --help\n"
		"    --list-suites, -l\n"
		"    --list-tests=SUITENAME, -tSUITENAME\n"
		"    --list-failures show failure records after the run\n"
		"    --list-skipped  show skipped-test records after the run\n"
	;

	int c;
	struct oplist ops = {0};
	int op_error = 0;
	while(-1 != (c = getopt_long (argc, argv, "-vsnr:e:t:l", long_options, &option_index))){
		switch(c){
		case 'v':
			mode = CU_BRM_VERBOSE;
			g_capture_enabled = 0;
			break;
		case 's':
			mode = CU_BRM_SILENT;
			break;
		case 'n':
			mode = CU_BRM_NORMAL;
			break;
		case 'r':
			oplist_append(&ops, OP_FILE, optarg);
			break;
		case 'e':
			if(optarg[0] == '-'){
				fprintf(stderr, "Invalid excluded test name '%s'\n", optarg);
				op_error = 1;
				break;
			}
			oplist_append(&ops, OP_REMOVE, optarg);
			break;
		case 0:
			if(strcmp(long_options[option_index].name, "on-error") == 0){
				if(0==strcmp(optarg,"fail")){
					fprintf(stderr,"on error FAIL\n");
					error_action = CUEA_FAIL;
				}else if(0==strcmp(optarg,"abort")){
					fprintf(stderr,"on error ABORT\n");
					error_action = CUEA_ABORT;
				}else if(0==strcmp(optarg,"ignore")){
					error_action = CUEA_IGNORE;
				}else{
					fprintf(stderr,"Invalid argument for --on-error option!\n");
					result = 1;
					goto cleanup;
				}
			}else if(strcmp(long_options[option_index].name, "list-failures") == 0){
				list_failures = 1;
			}else if(strcmp(long_options[option_index].name, "list-skipped") == 0){
				list_skipped = 1;
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
		case 1:
			if(optarg[0] == '-'){
				fprintf(stderr, "Invalid test name '%s'\n", optarg);
				op_error = 1;
				break;
			}
			oplist_append(&ops, OP_ADD, optarg);
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

	if(op_error){
		result = 1;
		goto cleanup;
	}

	if(ops.len > 0){
		struct strlist expanded = {0};
		for(int i = 0; i < ops.len; ++i){
			struct op *op = &ops.items[i];
			if(op->type == OP_FILE){
				if(!expand_tests_file(op->arg, &expanded)){
					result = 1;
					goto cleanup_ops;
				}
			}else if(op->type == OP_ADD){
				if(list){
					apply_add_pattern_suites_only(op->arg, &expanded);
				}else{
					apply_add_pattern(op->arg, &expanded);
				}
			}else if(op->type == OP_REMOVE){
				if(list){
					apply_remove_pattern_suites_only(op->arg, &expanded);
				}else{
					apply_remove_pattern(op->arg, &expanded);
				}
			}
		}
		if(list){
			if(expanded.len > 0){
				list_suites_filtered(&expanded);
			}else{
				list_suites();
			}
			goto cleanup_ops;
		}
		if(expanded.len > 0){
			result = CU_basic_run_selected_tests(expanded.len, expanded.items);
		}else{
			result = CU_basic_run_selected_tests(0, NULL);
		}
		ran_tests = 1;
cleanup_ops:
		strlist_free(&expanded);
	}else{
		if(list){
			if(strlen(suitename)){
				list_tests(suitename);
			}else{
				list_suites();
			}
			goto cleanup;
		}
		result = CU_basic_run_tests();
		ran_tests = 1;
	}
	if(ran_tests && list_failures){
		list_failed_tests();
	}
	if(ran_tests && list_skipped){
		list_skipped_tests();
	}

cleanup:
	oplist_free(&ops);
	if(mode == CU_BRM_VERBOSE){
		ascshutdown("Testing completed.");/* shut down memory manager */
	}
	CU_cleanup_registry();
	return result;
}
