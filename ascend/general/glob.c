/*
 * ASCEND glob matching (limited).
 * Supports '*' only.
 */
#include "glob.h"
#include <string.h>

static int has_unsupported(const char *pattern){
	return strpbrk(pattern, "?[") != NULL;
}

ASC_DLLSPEC int asc_glob_match(const char *pattern, const char *text, int *err){
	if(err){
		*err = 0;
	}
	if(pattern == NULL || text == NULL){
		return 0;
	}
	if(has_unsupported(pattern)){
		if(err){
			*err = 1;
		}
		return 0;
	}

	const char *p = pattern;
	const char *t = text;
	const char *star = NULL;
	const char *star_text = NULL;

	while(*t){
		if(*p == '*'){
			star = p++;
			star_text = t;
			continue;
		}
		if(*p == *t){
			p++;
			t++;
			continue;
		}
		if(star){
			p = star + 1;
			star_text++;
			t = star_text;
			continue;
		}
		return 0;
	}

	while(*p == '*'){
		p++;
	}
	return *p == '\0';
}
