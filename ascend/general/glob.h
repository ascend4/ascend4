/*
 * ASCEND glob matching (limited).
 * Supports '*' only. If unsupported tokens are present, returns error.
 */
#ifndef ASCEND_GENERAL_GLOB_H
#define ASCEND_GENERAL_GLOB_H

#include <ascend/general/platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Match a pattern against text.
 * Only '*' is supported as a wildcard.
 * Returns 1 for match, 0 for no match.
 * If err is non-NULL, *err is set to 1 when unsupported tokens are found.
 */
ASC_DLLSPEC int asc_glob_match(const char *pattern, const char *text, int *err);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_GENERAL_GLOB_H */
