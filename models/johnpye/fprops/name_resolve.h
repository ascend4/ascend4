#ifndef FPROPS_NAME_RESOLVE_H
#define FPROPS_NAME_RESOLVE_H

#include "name_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	FPROPS_NAME_RESOLVE_OK = 0,
	FPROPS_NAME_RESOLVE_NOT_FOUND = 1,
	FPROPS_NAME_RESOLVE_AMBIGUOUS = 2,
	FPROPS_NAME_RESOLVE_INVALID = 3
} FpropsNameResolveStatus;

typedef struct{
	const FpropsNameCanonical *canonical;
	const FpropsNameAlias *alias;
} FpropsResolvedName;

FpropsNameResolveStatus fprops_name_resolve(
	const char *token,
	unsigned domains,
	const char *source,
	FpropsResolvedName *out
);

int fprops_name_collect_matches(
	const char *token,
	unsigned domains,
	const char *source,
	const FpropsNameCanonical **out,
	int out_cap
);

#ifdef __cplusplus
}
#endif

#endif
