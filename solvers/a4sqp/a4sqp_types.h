/*
 * A4SQP core numeric and allocation definitions.
 *
 * Keep this header independent of ASCEND so liba4sqp.so can be used by
 * callback/SIFDecode frontends without importing libascend.so.
 */

#ifndef ASC_A4SQP_TYPES_H
#define ASC_A4SQP_TYPES_H

#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double real64;
typedef int int32;
typedef unsigned uint32;

#define A4SQP_NO_UPPER_BOUND DBL_MAX
#define A4SQP_NO_LOWER_BOUND (-DBL_MAX / 2.0)

#define A4SQP_NEW_ARRAY(TYPE,COUNT) ((TYPE *)malloc(sizeof(TYPE) * (size_t)(COUNT)))
#define A4SQP_NEW_ARRAY_CLEAR(TYPE,COUNT) ((TYPE *)calloc((size_t)(COUNT),sizeof(TYPE)))
#define A4SQP_NEW_ARRAY_OR_NULL(TYPE,COUNT) ((COUNT) > 0 ? A4SQP_NEW_ARRAY(TYPE,COUNT) : NULL)
#define A4SQP_NEW_ARRAY_OR_NULL_CLEAR(TYPE,COUNT) ((COUNT) > 0 ? A4SQP_NEW_ARRAY_CLEAR(TYPE,COUNT) : NULL)
#define A4SQP_NEW_CLEAR(TYPE) ((TYPE *)calloc(1,sizeof(TYPE)))
#define A4SQP_FREE(PTR) free(PTR)

#if defined(__GNUC__) || defined(__clang__)
# define A4SQP_CORE_EXPORT __attribute__((visibility("default")))
#else
# define A4SQP_CORE_EXPORT
#endif

#ifdef __cplusplus
}
#endif

#endif
